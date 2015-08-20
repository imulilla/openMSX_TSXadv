#include "PluggingController.hh"
#include "PlugException.hh"
#include "Connector.hh"
#include "Pluggable.hh"
#include "PluggableFactory.hh"
#include "TclObject.hh"
#include "CommandException.hh"
#include "MSXMotherBoard.hh"
#include "CliComm.hh"
#include "outer.hh"
#include "stl.hh"
#include <cassert>
#include <iostream>

using std::string;
using std::vector;

namespace openmsx {

PluggingController::PluggingController(MSXMotherBoard& motherBoard_)
	: motherBoard(motherBoard_)
	, plugCmd(
		motherBoard.getCommandController(),
		motherBoard.getStateChangeDistributor(),
		motherBoard.getScheduler())
	, unplugCmd(
		motherBoard.getCommandController(),
		motherBoard.getStateChangeDistributor(),
		motherBoard.getScheduler())
	, pluggableInfo      (motherBoard.getMachineInfoCommand())
	, connectorInfo      (motherBoard.getMachineInfoCommand())
	, connectionClassInfo(motherBoard.getMachineInfoCommand())
{
	PluggableFactory::createAll(*this, motherBoard);
}

PluggingController::~PluggingController()
{
#ifndef NDEBUG
	// This is similar to an assert: it should never print anything,
	// but if it does, it helps to catch an error.
	for (auto& c : connectors) {
		std::cerr << "ERROR: Connector still registered at shutdown: "
		          << c->getName() << std::endl;
	}
#endif
}

void PluggingController::registerConnector(Connector& connector)
{
	connectors.push_back(&connector);
	getCliComm().update(CliComm::CONNECTOR, connector.getName(), "add");
}

void PluggingController::unregisterConnector(Connector& connector)
{
	connector.unplug(motherBoard.getCurrentTime());
	move_pop_back(connectors, rfind_unguarded(connectors, &connector));
	getCliComm().update(CliComm::CONNECTOR, connector.getName(), "remove");
}


void PluggingController::registerPluggable(std::unique_ptr<Pluggable> pluggable)
{
	pluggables.push_back(std::move(pluggable));
}


// === Commands ===
//  plug command

PluggingController::PlugCmd::PlugCmd(
		CommandController& commandController_,
		StateChangeDistributor& stateChangeDistributor_,
		Scheduler& scheduler_)
	: RecordedCommand(commandController_, stateChangeDistributor_,
	                  scheduler_, "plug")
{
}

void PluggingController::PlugCmd::execute(
	span<const TclObject> tokens, TclObject& result_, EmuTime::param time)
{
	string result;
	auto& pluggingController = OUTER(PluggingController, plugCmd);
	switch (tokens.size()) {
	case 1:
		for (auto& c : pluggingController.connectors) {
			strAppend(result, c->getName(), ": ",
			          c->getPlugged().getName(), '\n');
		}
		break;
	case 2: {
		auto& connector = pluggingController.getConnector(tokens[1].getString());
		strAppend(result, connector.getName(), ": ",
		          connector.getPlugged().getName());
		break;
	}
	case 3: {
		string_view connName = tokens[1].getString();
		string_view plugName = tokens[2].getString();
		auto& connector = pluggingController.getConnector(connName);
		auto& pluggable = pluggingController.getPluggable(plugName);
		if (&connector.getPlugged() == &pluggable) {
			// already plugged, don't unplug/replug
			break;
		}
		if (connector.getClass() != pluggable.getClass()) {
			throw CommandException("plug: ", plugName,
			                       " doesn't fit in ", connName);
		}
		connector.unplug(time);
		try {
			connector.plug(pluggable, time);
			pluggingController.getCliComm().update(
				CliComm::PLUG, connName, plugName);
		} catch (PlugException& e) {
			throw CommandException("plug: plug failed: ", e.getMessage());
		}
		break;
	}
	default:
		throw SyntaxError();
	}
	result_.setString(result); // TODO return Tcl list
}

string PluggingController::PlugCmd::help(const vector<string>& /*tokens*/) const
{
	return "Plugs a plug into a connector\n"
	       " plug [connector] [plug]";
}

void PluggingController::PlugCmd::tabCompletion(vector<string>& tokens) const
{
	auto& pluggingController = OUTER(PluggingController, plugCmd);
	if (tokens.size() == 2) {
		// complete connector
		vector<string_view> connectorNames;
		for (auto& c : pluggingController.connectors) {
			connectorNames.emplace_back(c->getName());
		}
		completeString(tokens, connectorNames);
	} else if (tokens.size() == 3) {
		// complete pluggable
		vector<string_view> pluggableNames;
		auto* connector = pluggingController.findConnector(tokens[1]);
		string_view className = connector ? connector->getClass() : string_view{};
		for (auto& p : pluggingController.pluggables) {
			if (p->getClass() == className) {
				pluggableNames.emplace_back(p->getName());
			}
		}
		completeString(tokens, pluggableNames);
	}
}

bool PluggingController::PlugCmd::needRecord(span<const TclObject> tokens) const
{
	return tokens.size() == 3;
}


//  unplug command

PluggingController::UnplugCmd::UnplugCmd(
		CommandController& commandController_,
		StateChangeDistributor& stateChangeDistributor_,
		Scheduler& scheduler_)
	: RecordedCommand(commandController_, stateChangeDistributor_,
	                  scheduler_, "unplug")
{
}

void PluggingController::UnplugCmd::execute(
	span<const TclObject> tokens, TclObject& /*result*/, EmuTime::param time)
{
	if (tokens.size() != 2) {
		throw SyntaxError();
	}
	auto& pluggingController = OUTER(PluggingController, unplugCmd);
	string_view connName = tokens[1].getString();
	auto& connector = pluggingController.getConnector(connName);
	connector.unplug(time);
	pluggingController.getCliComm().update(CliComm::PLUG, connName, {});
}

string PluggingController::UnplugCmd::help(const vector<string>& /*tokens*/) const
{
	return "Unplugs a plug from a connector\n"
	       " unplug [connector]";
}

void PluggingController::UnplugCmd::tabCompletion(vector<string>& tokens) const
{
	if (tokens.size() == 2) {
		// complete connector
		vector<string_view> connectorNames;
		auto& pluggingController = OUTER(PluggingController, unplugCmd);
		for (auto& c : pluggingController.connectors) {
			connectorNames.emplace_back(c->getName());
		}
		completeString(tokens, connectorNames);
	}
}

Connector* PluggingController::findConnector(string_view name) const
{
	for (auto& c : connectors) {
		if (c->getName() == name) {
			return c;
		}
	}
	return nullptr;
}

Connector& PluggingController::getConnector(string_view name) const
{
	if (auto* result = findConnector(name)) {
		return *result;
	}
	throw CommandException("No such connector: ", name);
}

Pluggable* PluggingController::findPluggable(string_view name) const
{
	for (auto& p : pluggables) {
		if (p->getName() == name) {
			return p.get();
		}
	}
	return nullptr;
}

Pluggable& PluggingController::getPluggable(string_view name) const
{
	if (auto* result = findPluggable(name)) {
		return *result;
	}
	throw CommandException("No such pluggable: ", name);
}

CliComm& PluggingController::getCliComm()
{
	return motherBoard.getMSXCliComm();
}

EmuTime::param PluggingController::getCurrentTime() const
{
	return motherBoard.getCurrentTime();
}


// Pluggable info

PluggingController::PluggableInfo::PluggableInfo(
		InfoCommand& machineInfoCommand)
	: InfoTopic(machineInfoCommand, "pluggable")
{
}

void PluggingController::PluggableInfo::execute(
	span<const TclObject> tokens, TclObject& result) const
{
	auto& pluggingController = OUTER(PluggingController, pluggableInfo);
	switch (tokens.size()) {
	case 2:
		for (auto& p : pluggingController.pluggables) {
			result.addListElement(p->getName());
		}
		break;
	case 3: {
		auto& pluggable = pluggingController.getPluggable(
				tokens[2].getString());
		result.setString(pluggable.getDescription());
		break;
	}
	default:
		throw CommandException("Too many parameters");
	}
}

string PluggingController::PluggableInfo::help(const vector<string>& /*tokens*/) const
{
	return "Shows a list of available pluggables. "
	       "Or show info on a specific pluggable.";
}

void PluggingController::PluggableInfo::tabCompletion(vector<string>& tokens) const
{
	if (tokens.size() == 3) {
		vector<string_view> pluggableNames;
		auto& pluggingController = OUTER(PluggingController, pluggableInfo);
		for (auto& p : pluggingController.pluggables) {
			pluggableNames.emplace_back(p->getName());
		}
		completeString(tokens, pluggableNames);
	}
}

// Connector info

PluggingController::ConnectorInfo::ConnectorInfo(
		InfoCommand& machineInfoCommand)
	: InfoTopic(machineInfoCommand, "connector")
{
}

void PluggingController::ConnectorInfo::execute(
	span<const TclObject> tokens, TclObject& result) const
{
	auto& pluggingController = OUTER(PluggingController, connectorInfo);
	switch (tokens.size()) {
	case 2:
		for (auto& c : pluggingController.connectors) {
			result.addListElement(c->getName());
		}
		break;
	case 3: {
		auto& connector = pluggingController.getConnector(tokens[2].getString());
		result.setString(connector.getDescription());
		break;
	}
	default:
		throw CommandException("Too many parameters");
	}
}

string PluggingController::ConnectorInfo::help(const vector<string>& /*tokens*/) const
{
	return "Shows a list of available connectors.";
}

void PluggingController::ConnectorInfo::tabCompletion(vector<string>& tokens) const
{
	if (tokens.size() == 3) {
		vector<string_view> connectorNames;
		auto& pluggingController = OUTER(PluggingController, connectorInfo);
		for (auto& c : pluggingController.connectors) {
			connectorNames.emplace_back(c->getName());
		}
		completeString(tokens, connectorNames);
	}
}

// Connection Class info

PluggingController::ConnectionClassInfo::ConnectionClassInfo(
		InfoCommand& machineInfoCommand)
	: InfoTopic(machineInfoCommand, "connectionclass")
{
}

void PluggingController::ConnectionClassInfo::execute(
	span<const TclObject> tokens, TclObject& result) const
{
	auto& pluggingController = OUTER(PluggingController, connectionClassInfo);
	switch (tokens.size()) {
	case 2: {
		std::vector<string_view> classes;
		classes.reserve(pluggingController.connectors.size());
		for (auto& c : pluggingController.connectors) {
			classes.push_back(c->getClass());
		}
		for (auto& p : pluggingController.pluggables) {
			auto c = p->getClass();
			if (!contains(classes, c)) classes.push_back(c);
		}
		result.addListElements(classes);
		break;
	}
	case 3: {
		const auto& arg = tokens[2].getString();
		if (auto* connector = pluggingController.findConnector(arg)) {
			result.setString(connector->getClass());
			break;
		}
		if (auto* pluggable = pluggingController.findPluggable(arg)) {
			result.setString(pluggable->getClass());
			break;
		}
		throw CommandException("No such connector or pluggable");
	}
	default:
		throw CommandException("Too many parameters");
	}
}

string PluggingController::ConnectionClassInfo::help(const vector<string>& /*tokens*/) const
{
	return "Shows the class a connector or pluggable belongs to.";
}

void PluggingController::ConnectionClassInfo::tabCompletion(vector<string>& tokens) const
{
	if (tokens.size() == 3) {
		vector<string_view> names;
		auto& pluggingController = OUTER(PluggingController, connectionClassInfo);
		for (auto& c : pluggingController.connectors) {
			names.emplace_back(c->getName());
		}
		for (auto& p : pluggingController.pluggables) {
			names.emplace_back(p->getName());
		}
		completeString(tokens, names);
	}
}

} // namespace openmsx
