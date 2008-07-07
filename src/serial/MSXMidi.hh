// $Id$

#ifndef MSXMIDI_HH
#define MSXMIDI_HH

#include "MSXDevice.hh"
#include "IRQHelper.hh"
#include "MidiInConnector.hh"

namespace openmsx {

class MSXMidiCounter0;
class MSXMidiCounter2;
class MSXMidiI8251Interf;
class ClockPin;
class I8254;
class I8251;
class MidiOutConnector;

class MSXMidi: public MSXDevice, public MidiInConnector
{
public:
	MSXMidi(MSXMotherBoard& motherBoard, const XMLElement& config);
	virtual ~MSXMidi();

	virtual void reset(const EmuTime& time);
	virtual byte readIO(word port, const EmuTime& time);
	virtual byte peekIO(word port, const EmuTime& time) const;
	virtual void writeIO(word port, byte value, const EmuTime& time);

	// MidiInConnector
	virtual bool ready();
	virtual bool acceptsData();
	virtual void setDataBits(DataBits bits);
	virtual void setStopBits(StopBits bits);
	virtual void setParityBit(bool enable, ParityBit parity);
	virtual void recvByte(byte value, const EmuTime& time);

	template<typename Archive>
	void serialize(Archive& ar, unsigned version);

private:
	void setTimerIRQ(bool status, const EmuTime& time);
	void enableTimerIRQ(bool enabled, const EmuTime& time);
	void updateEdgeEvents(const EmuTime& time);
	void setRxRDYIRQ(bool status);
	void enableRxRDYIRQ(bool enabled);

	const std::auto_ptr<MSXMidiCounter0> cntr0; // counter 0 clock pin
	const std::auto_ptr<MSXMidiCounter2> cntr2; // counter 2 clock pin
	const std::auto_ptr<MSXMidiI8251Interf> interf;

	IRQHelper timerIRQ;
	IRQHelper rxrdyIRQ;
	bool timerIRQlatch;
	bool timerIRQenabled;
	bool rxrdyIRQlatch;
	bool rxrdyIRQenabled;

	// must come last
	const std::auto_ptr<MidiOutConnector> outConnector;
	const std::auto_ptr<I8251> i8251;
	const std::auto_ptr<I8254> i8254;

	friend class MSXMidiCounter0;
	friend class MSXMidiCounter2;
	friend class MSXMidiI8251Interf;
};

REGISTER_MSXDEVICE(MSXMidi, "MSX-Midi");

} // namespace openmsx

#endif
