// $Id$

#include "Keys.hh"
#include "RealTime.hh"
#include "MSXCPU.hh"
#include "MSXConfig.hh"
#include "CommandController.hh"
#include "Scheduler.hh"
#include "RealTimeSDL.hh"

const int SYNC_INTERVAL = 50;


RealTime::RealTime()
	: throttleSetting("throttle", "controls speed throttling", true)
{
	// default values
	maxCatchUpFactor = 105; // %
	maxCatchUpTime = 2000;	// ms
	try {
		Config *config = MSXConfig::instance()->getConfigById("RealTime");
		if (config->hasParameter("max_catch_up_time")) {
			maxCatchUpTime =
				config->getParameterAsInt("max_catch_up_time");
		}
		if (config->hasParameter("max_catch_up_factor")) {
			maxCatchUpFactor =
				config->getParameterAsInt("max_catch_up_factor");
		}
	} catch (MSXException &e) {
		// no Realtime section
	}

	Scheduler::instance()->setSyncPoint(Scheduler::ASAP, this);
}

RealTime::~RealTime()
{
}

RealTime *RealTime::instance()
{
	static RealTime* oneInstance = NULL;
	if (oneInstance == NULL) {
		oneInstance = new RealTimeSDL();
	}
	return oneInstance;
}

const std::string &RealTime::schedName() const
{
	static const std::string name("RealTime");
	return name;
}

void RealTime::executeUntilEmuTime(const EmuTime &curEmu, int userData)
{
	internalSync(curEmu);
}

float RealTime::sync(const EmuTime &time)
{
	Scheduler::instance()->removeSyncPoint(this);
	return internalSync(time);
}

float RealTime::internalSync(const EmuTime &time)
{
	float speed;
	if (throttleSetting.getValue()) {
		speed = doSync(time);
	} else {
		speed = 1.0;
	}
	
	// Schedule again in future
	EmuTimeFreq<1000> time2(time);
	Scheduler::instance()->setSyncPoint(time2 + SYNC_INTERVAL, this);
	
	return speed;
}

float RealTime::getRealDuration(const EmuTime &time1, const EmuTime &time2)
{
	return (time2 - time1).toFloat() * 100.0 / speedSetting.getValue();
}

EmuDuration RealTime::getEmuDuration(float realDur)
{
	return EmuDuration(realDur * speedSetting.getValue() / 100.0);
}


RealTime::PauseSetting::PauseSetting()
	: BooleanSetting("pause", "pauses the emulation", false)
{
}

bool RealTime::PauseSetting::checkUpdate(bool newValue)
{
	if (newValue) {
		Scheduler::instance()->pause();
	} else {
		RealTime::instance()->resync();
		Scheduler::instance()->unpause();
	}
	return true;
}

RealTime::SpeedSetting::SpeedSetting()
	: IntegerSetting("speed",
	       "controls the emulation speed: higher is faster, 100 is normal",
	       100, 1, 1000000)
{
}

bool RealTime::SpeedSetting::checkUpdate(int newValue)
{
	RealTime::instance()->resync();
	return true;
}
