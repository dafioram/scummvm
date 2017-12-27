/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef SCI_SOUND_DRIVERS_ADLIB_H
#define SCI_SOUND_DRIVERS_ADLIB_H

#include "audio/fmopl.h"
#include "audio/rate.h"
#include "common/debug.h"
#include "common/ptr.h"
#include "sci/sound/drivers/driver.h"
#include "sci/sound/sound.h"

namespace Sci {

class AdLibDriverMixin {
public:
	AdLibDriverMixin() {
		_opl.reset(OPL::Config::create(OPL::Config::kDualOpl2));
		if (!_opl) {
			_opl.reset(OPL::Config::create(OPL::Config::kOpl2));
		}
		if (!_opl) {
			error("Could not create OPL");
		}
		if (!_opl->init()) {
			error("Could not init OPL");
		}

		_opl->start(nullptr);
	}

protected:
	Common::ScopedPtr<OPL::OPL> _opl;
};

class Sci0AdLibDriver final : public Sci0SoundDriver, AdLibDriverMixin {
public:
	Sci0AdLibDriver(ResourceManager &resMan, SciVersion version) :
		Sci0SoundDriver(resMan, version),
		AdLibDriverMixin() {}

	virtual int getNumVoices() const override { return 9; }

	virtual void service(Sci0Sound &) override {
		debug("TODO: Service");
	}

	virtual Sci0PlayStrategy play(Sci0Sound &) override {
		debug("TODO: Play");
		return kAbort;
	}

	virtual void fade(Sci0Sound &) override {
		debug("TODO: Fade");
	}

	virtual void stop(Sci0Sound &) override {
		debug("TODO: Stop");
	}

	virtual void pause(Sci0Sound &) override {
		debug("TODO: Pause");
	}

	virtual void restore(Sci0Sound &) override {
		debug("TODO: Restore");
	}

	virtual void setMasterVolume(Sci0Sound &sound) override {
		debug("TODO: Set master volume to %d", sound.volume);
	}
};

class Sci1AdLibDriver final : public Sci1SoundDriver, AdLibDriverMixin {
public:
	Sci1AdLibDriver(ResourceManager &resMan, SciVersion version) :
		Sci1SoundDriver(resMan, version),
		AdLibDriverMixin() {}

	virtual int getNumVoices() const override { return 9; }

	virtual SoundDriver::DeviceId getDeviceId() const override { return 0; }

	virtual void getValidChannelRange(int8 &low, int8 &high) const override { low = 0; high = 8; }

	virtual void service() override {
		debug("TODO: Service");
	}

	virtual void noteOn(const int8 channelIndex, const int8 note, const int8 velocity) override {
		debug("TODO: On  channel %d note %03d velocity %03d", channelIndex, note, velocity);
	}

	virtual void noteOff(const int8 channelIndex, const int8 note, const int8 velocity) override {
		debug("TODO: Off channel %d note %03d velocity %03d", channelIndex, note, velocity);
	}

	virtual void controllerChange(const int8 channelIndex, const int8 controllerNo, const int8 value) override {
		debug("TODO: Channel %d controller %d value %d", channelIndex, controllerNo, value);
	}

	virtual void programChange(const int8 channelIndex, const int8 programNo) override {
		debug("TODO: Channel %d program %d", channelIndex, programNo);
	}

	virtual void pitchBend(const int8 channelIndex, const int16 pitch) override {
		debug("TODO: Channel %d pitch %d", channelIndex, pitch);
	}

	virtual void keyPressure(const int8 channelIndex, const int8 note, const int8 pressure) override {
		debug("TODO: Channel %d note %d pressure %d", channelIndex, note, pressure);
	}

	virtual void channelPressure(const int8 channelIndex, const int8 pressure) override {
		debug("TODO: Channel %d pressure %d", channelIndex, pressure);
	}

	virtual uint8 getReverbMode() const override {
		debug("TODO: Get reverb mode");
		return 0;
	}

	virtual uint8 setReverbMode(const uint8 mode) override {
		debug("TODO: Set reverb mode to %d", mode);
		return 0;
	}

	virtual int8 getMasterVolume() const override {
		debug("TODO: Get master volume");
		return 15;
	}

	virtual int8 setMasterVolume(const int8 volume) override {
		debug("TODO: Set master volume to %d", volume);
		return 15;
	}
};

SoundDriver *makeAdLibDriver(ResourceManager &resMan, SciVersion version) {
	if (version <= SCI_VERSION_01) {
		return new Sci0AdLibDriver(resMan, version);
	} else {
		return new Sci1AdLibDriver(resMan, version);
	}
}

} // End of namespace Sci

#endif
