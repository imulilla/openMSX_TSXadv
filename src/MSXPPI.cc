// $Id$

#include <cassert>
#include "MSXPPI.hh"
#include "Keyboard.hh"
#include "Leds.hh"
#include "MSXCPUInterface.hh"
#include "KeyClick.hh"
#include "CassettePort.hh"
#include "xmlx.hh"
#include "RenShaTurbo.hh"

namespace openmsx {

// MSXDevice

MSXPPI::MSXPPI(const XMLElement& config, const EmuTime& time)
	: MSXDevice(config, time), MSXIODevice(config, time),
	  cassettePort(CassettePortFactory::instance()),
	  cpuInterface(MSXCPUInterface::instance()),
	  leds(Leds::instance()),
	  renshaTurbo(RenShaTurbo::instance())
{
	bool keyGhosting = deviceConfig.getChildDataAsBool("key_ghosting", true);
	keyboard.reset(new Keyboard(keyGhosting));
	i8255.reset(new I8255(*this, time));
	click.reset(new KeyClick(config, time));

	reset(time);
}

MSXPPI::~MSXPPI()
{
}

void MSXPPI::reset(const EmuTime& time)
{
	i8255->reset(time);
	click->reset(time);
}

byte MSXPPI::readIO(byte port, const EmuTime& time)
{
	switch (port & 0x03) {
	case 0:
		return i8255->readPortA(time);
	case 1:
		return i8255->readPortB(time);
	case 2:
		return i8255->readPortC(time);
	case 3:
		return i8255->readControlPort(time);
	default: // unreachable, avoid warning
		assert(false);
		return 0;
	}
}

void MSXPPI::writeIO(byte port, byte value, const EmuTime& time)
{
	switch (port & 0x03) {
	case 0:
		i8255->writePortA(value, time);
		break;
	case 1:
		i8255->writePortB(value, time);
		break;
	case 2:
		i8255->writePortC(value, time);
		break;
	case 3:
		i8255->writeControlPort(value, time);
		break;
	default:
		assert(false);
	}
}


// I8255Interface

byte MSXPPI::readA(const EmuTime& /*time*/)
{
	// port A is normally an output on MSX, reading from an output port
	// is handled internally in the 8255
	return 255;	//TODO check this
}
void MSXPPI::writeA(byte value, const EmuTime& /*time*/)
{
	cpuInterface.setPrimarySlots(value);
}

byte MSXPPI::readB(const EmuTime& time)
{
       if (selectedRow != 8) {
               return keyboard->getKeys()[selectedRow];
       } else {
               return keyboard->getKeys()[8] | renshaTurbo.getSignal(time);
       }

}
void MSXPPI::writeB(byte /*value*/, const EmuTime& /*time*/)
{
	// probably nothing happens on a real MSX
}

nibble MSXPPI::readC1(const EmuTime& /*time*/)
{
	return 15;	// TODO check this
}
nibble MSXPPI::readC0(const EmuTime& /*time*/)
{
	return 15;	// TODO check this
}
void MSXPPI::writeC1(nibble value, const EmuTime& time)
{
	cassettePort.setMotor(!(value & 1), time);	// 0=0n, 1=Off
	cassettePort.cassetteOut(value & 2, time);

	Leds::LEDCommand caps = (value & 4) ? Leds::CAPS_OFF : Leds::CAPS_ON;
	leds.setLed(caps);

	click->setClick(value & 8, time);
}
void MSXPPI::writeC0(nibble value, const EmuTime& /*time*/)
{
	selectedRow = value;
}

} // namespace openmsx
