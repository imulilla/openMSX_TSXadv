// $Id$

#ifndef __MSXDEVICE_HH__
#define __MSXDEVICE_HH__

#include <iostream>
#include <fstream>
#include <string>
#include "MSXConfig.hh"
#include "openmsx.hh"

// forward declarations
class EmuTime;


/** An MSXDevice is an emulated hardware component connected to the bus
  * of the emulated MSX (located in MSXMotherBoard). There is no
  * communication among devices, only between devices and the CPU.
  * MSXMemDevice contains the interface for memory mapped communication.
  * MSXIODevice contains the interface for I/O mapped communication.
  */
class MSXDevice
{
	public:
		/**
		 * Destructor
		 */
		virtual ~MSXDevice();

		/**
		 * This method is called on reset.
		 * Default implementation resets internal IRQ flag (every
		 * re-implementation should also call this method).
		 */
		virtual void reset(const EmuTime &time);


		/**
		 * Save all state-information for this device
		 * Default implementation does nothing (nothing needs to be saved).
		 *
		 * Note: save mechanism not implemented yet
		 */
		virtual void saveState(std::ofstream &writestream);

		/**
		 * Restore all state-information for this device
		 * Default implementation does nothing (nothing needs to be restored).
		 *
		 * Note: save mechanism not implemented yet
		 */
		virtual void restoreState(std::string &devicestring, std::ifstream &readstream);


		/**
		 * Returns a human-readable name for this device. The name is set
		 * in the setConfigDevice() method. This method is mostly used to
		 * print debug info.
		 * Default implementation is normally ok.
		 */
		virtual const std::string &getName();

	protected:
		/** Every MSXDevice has a config entry; this constructor gets
		  * some device properties from that config entry.
		  * All subclasses must call this super-constructor.
		  * @param config config entry for this device.
		  * @param time the moment in emulated time this MSXDevice is
		  *   created (typically at time zero: power-up).
		  */
		MSXDevice(MSXConfig::Device *config, const EmuTime &time);

		/** Superclass constructor for DummyDevice, which is the only
		  * device that has no config entry.
		  * It has no sense of time either.
		  */
		MSXDevice();

		MSXConfig::Device *deviceConfig;
		const std::string* deviceName;
		static const std::string defaultName;
};

#endif //__MSXDEVICE_HH__

