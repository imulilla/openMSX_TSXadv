// $Id$

#include "Y8950KeyboardConnector.hh"
#include "PluggingController.hh"


Y8950KeyboardConnector::Y8950KeyboardConnector(const EmuTime &time)
{
	PRT_DEBUG("Creating a Y8950KeyboardConnector");

	dummy = new DummyY8950KeyboardDevice();
	PluggingController::instance()->registerConnector(this);
	unplug(time);	// TODO plug device as specified in config file
	write(255, time);
}

Y8950KeyboardConnector::~Y8950KeyboardConnector()
{
	PRT_DEBUG("Destroying a Y8950KeyboardConnector");
	//unplug(time)
	PluggingController::instance()->unregisterConnector(this);
	delete dummy;
}


void Y8950KeyboardConnector::write(byte newData, const EmuTime &time)
{
	if (newData != data) {
		data = newData;
		((Y8950KeyboardDevice*)pluggable)->write(data, time);
	}
}

byte Y8950KeyboardConnector::read(const EmuTime &time)
{
	return ((Y8950KeyboardDevice*)pluggable)->read(time);
}


const std::string &Y8950KeyboardConnector::getName()
{
	return name;
}
const std::string Y8950KeyboardConnector::name("audiokeyboardport");

const std::string &Y8950KeyboardConnector::getClass()
{
	return className;
}
const std::string Y8950KeyboardConnector::className("Y8950 Keyboard Port");

void Y8950KeyboardConnector::plug(Pluggable *dev, const EmuTime &time)
{
	Connector::plug(dev, time);
	((Y8950KeyboardDevice*)pluggable)->write(data, time);
}

void Y8950KeyboardConnector::unplug(const EmuTime &time)
{
	Connector::unplug(time);
	plug(dummy, time);
}


// --- DummyY8950KeyboardDevice ---

void DummyY8950KeyboardDevice::write(byte data, const EmuTime &time)
{
	// ignore data
}

byte DummyY8950KeyboardDevice::read(const EmuTime &time)
{
	return 255;
}

const std::string &DummyY8950KeyboardDevice::getName()
{
	return name;
}
const std::string DummyY8950KeyboardDevice::name("dummy");
