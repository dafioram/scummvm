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

#ifndef SCI_SOUND_DRIVERS_AMIGAMAC_H
#define SCI_SOUND_DRIVERS_AMIGAMAC_H

#include "common/ptr.h"
#include "sci/sound/drivers/driver.h"
#include "sci/sound/sound.h"

namespace Sci {

class AmigaMacDriver final : public SoundDriver {
public:
	AmigaMacDriver(ResourceManager &resMan, SciVersion version);
	~AmigaMacDriver() {}

	virtual int getNumVoices() const override { return 4; }

	virtual DeviceId getDeviceId() const override { return 6; }

	virtual void getChannelMasks(uint8 &instrumentMask, uint8 &percussionMask) const override {
		instrumentMask = 0x40;
		percussionMask = 0;
	}

	virtual void getRemapRange(uint8 &low, uint8 &high) const override {
		low = 0;
		high = 3;
	}

	virtual void service() override {}

	virtual void noteOn(const uint8 channelNo, const uint8 note, const uint8 velocity) override {}

	virtual void noteOff(const uint8 channelNo, const uint8 note, const uint8 velocity) override {}

	virtual void controllerChange(const uint8 channelNo, const uint8 controllerNo, const uint8 value) override {}

	virtual void programChange(const uint8 channelNo, const uint8 programNo) override {}

	virtual void pitchBend(const uint8 channelNo, const uint16 bend) override {}

	virtual void keyPressure(const uint8 channelNo, const uint8 note, const uint8 pressure) override {
		// no-op
	}

	virtual void channelPressure(const uint8 channelNo, const uint8 pressure) override {
		// no-op
	}

	virtual void setMasterVolume(const uint8 volume) override {}

	virtual void enable(const bool enable) override {}

	virtual void debugPrintState(Console &con) const override {}

private:
	enum {
		kNumVoices = 4
	};
};

SoundDriver *makeAmigaMacDriver(ResourceManager &resMan, SciVersion version);

} // End of namespace Sci

#endif

