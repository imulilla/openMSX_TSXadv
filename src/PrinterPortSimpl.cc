// $Id$

#include "PrinterPortSimpl.hh"
#include "DACSound8U.hh"
#include "XMLElement.hh"

using std::auto_ptr;
using std::string;

namespace openmsx {

PrinterPortSimpl::PrinterPortSimpl()
{
}

PrinterPortSimpl::~PrinterPortSimpl()
{
}

bool PrinterPortSimpl::getStatus(const EmuTime& /*time*/)
{
	return true;	// TODO check
}

void PrinterPortSimpl::setStrobe(bool /*strobe*/, const EmuTime& /*time*/)
{
	// ignore strobe	TODO check
}

void PrinterPortSimpl::writeData(byte data, const EmuTime& time)
{
	dac->writeDAC(data, time);
}

void PrinterPortSimpl::plugHelper(Connector* /*connector*/, const EmuTime& time)
{
	// TODO get from config file
	static XMLElement simplConfig("simpl");
	static bool init = false;
	if (!init) {
		init = true;
		auto_ptr<XMLElement> soundElem(new XMLElement("sound"));
		soundElem->addChild(auto_ptr<XMLElement>(
			new XMLElement("volume", "12000")));
		soundElem->addChild(auto_ptr<XMLElement>(
			new XMLElement("mode", "mono")));
		simplConfig.addChild(soundElem);
	}
	dac.reset(new DACSound8U("simpl", getDescription(), simplConfig, time));
}

void PrinterPortSimpl::unplugHelper(const EmuTime& /*time*/)
{
	dac.reset();
}

const string& PrinterPortSimpl::getName() const
{
	static const string name("simpl");
	return name;
}

const string& PrinterPortSimpl::getDescription() const
{
	static const string desc("Play samples via your printer port.");
	return desc;
}

} // namespace openmsx
