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

class AdLibDriver final : public SoundDriver {
public:
	AdLibDriver(ResourceManager &resMan, SciVersion version) :
		SoundDriver(resMan, version) {

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

	virtual int getNumVoices() const override { return 9; }

	virtual DeviceId getDeviceId() const override { return 0; }

	virtual void getChannelMasks(uint8 &instrumentMask, uint8 &percussionMask) const override {
		instrumentMask = (_version == SCI_VERSION_0_EARLY) ? 1 : 4;
		percussionMask = 0;
	}

	virtual void getRemapRange(uint8 &low, uint8 &high) const override { low = 0; high = 8; }

	virtual void service() override {
		static int numServices = 0;
		if (numServices == 600) {
			debug("TODO: Serviced 10 seconds");
			numServices = 0;
		}
	}

	virtual void noteOn(const uint8 channelIndex, const uint8 note, const uint8 velocity) override {
		debug("TODO: On  channel %u note %03u velocity %03u", channelIndex, note, velocity);
	}

	virtual void noteOff(const uint8 channelIndex, const uint8 note, const uint8 velocity) override {
		debug("TODO: Off channel %u note %03u velocity %03u", channelIndex, note, velocity);
	}

	virtual void controllerChange(const uint8 channelIndex, const uint8 controllerNo, const uint8 value) override {
		debug("TODO: Channel %u controller %u value %u", channelIndex, controllerNo, value);
	}

	virtual void programChange(const uint8 channelIndex, const uint8 programNo) override {
		debug("TODO: Channel %u program %u", channelIndex, programNo);
	}

	virtual void pitchBend(const uint8 channelIndex, const uint16 bend) override {
		debug("TODO: Channel %u pitch %u", channelIndex, bend);
	}

	virtual void keyPressure(const uint8 channelIndex, const uint8 note, const uint8 pressure) override {
		debug("TODO: Channel %u note %u pressure %u", channelIndex, note, pressure);
	}

	virtual void channelPressure(const uint8 channelIndex, const uint8 pressure) override {
		debug("TODO: Channel %u pressure %u", channelIndex, pressure);
	}

	virtual uint8 getReverbMode() const override {
		debug("TODO: Get reverb mode");
		return 0;
	}

	virtual void setReverbMode(const uint8 mode) override {
		debug("TODO: Set reverb mode to %u", mode);
	}

	virtual uint8 getMasterVolume() const override {
		debug("TODO: Get master volume");
		return 15;
	}

	virtual void setMasterVolume(const uint8 volume) override {
		debug("TODO: Set master volume to %u", volume);
	}
protected:
	Common::ScopedPtr<OPL::OPL> _opl;
};

SoundDriver *makeAdLibDriver(ResourceManager &resMan, SciVersion version) {
	return new AdLibDriver(resMan, version);
}

} // End of namespace Sci

#endif
