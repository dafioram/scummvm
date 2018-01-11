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

#ifndef SCI_SOUND_DRIVERS_MT32_H
#define SCI_SOUND_DRIVERS_MT32_H

#include "audio/mpu401.h"
#include "common/array.h"
#include "common/debug.h"
#include "common/ptr.h"
#include "sci/sound/drivers/driver.h"
#include "sci/sound/sound.h"

namespace Sci {

class Sci1Mt32Driver final : public Sci1SoundDriver {
public:
	enum {
		kMinChannel = 1
	};

	Sci1Mt32Driver(ResourceManager &resMan, const SciVersion version);
	virtual ~Sci1Mt32Driver();

	virtual int getNumVoices() const override { return 32; }

	virtual DeviceId getDeviceId() const override { return _deviceId; }

	virtual void getRemapRange(uint8 &low, uint8 &high) const override { low = kMinChannel; high = kPercussionChannel - 1; }

	virtual void service() override {
		// no-op
	}

	virtual void noteOn(const uint8 channelNo, const uint8 note, const uint8 velocity) override;

	virtual void noteOff(const uint8 channelNo, const uint8 note, const uint8 velocity) override;

	virtual void controllerChange(const uint8 channelNo, const uint8 controllerNo, const uint8 value) override;

	virtual void programChange(const uint8 channelNo, const uint8 programNo) override;

	virtual void pitchBend(const uint8 channelNo, const uint16 bend) override;

	virtual void keyPressure(const uint8 channelNo, const uint8 note, const uint8 pressure) override {
		// no-op
	}

	virtual void channelPressure(const uint8 channelNo, const uint8 pressure) override {
		// no-op
	}

	virtual uint8 getReverbMode() const override { return _reverbMode; }

	virtual void setReverbMode(const uint8 mode) override;

	virtual uint8 getMasterVolume() const override { return _masterVolume; }

	virtual void setMasterVolume(const uint8 volume) override;

	virtual void enable(const bool enabled) override;

	virtual void debugPrintState(Console &con) const override;

private:
	enum {
		kNumChannels = 16,
		kNumPatchesPerBank = 48,
		kPatchSize = 8,
		kNumRhythmPatches = 64,
		kRhythmPatchSize = 4,
		kPartialReserveSize = 9,
		kShortTimbreSize = 14,
		kLongTimbreSize = 58,
		kNumLongTimbres = 4,
		kPatchDataSize = kPatchSize * kNumPatchesPerBank,
		kTimbreDataSize = kShortTimbreSize + kLongTimbreSize * kNumLongTimbres,
		kRhythmDataSize = kNumRhythmPatches * kRhythmPatchSize
	};

	enum {
		kRhythmKeyMapAddress = 0x30110,
		kPatchAddress = 0x50000,
		kTimbreAddress = 0x80000,
		kReverbModeAddress = 0x100001,
		kPartialReserveAddress = 0x100004,
		kMasterVolumeAddress = 0x100016,
		kDisplayAddress = 0x200000,
		kDisableCm32PAddress = 0x52000a
	};

	// TODO: These values are common to at least GM and MT-32 so should probably
	// go somewhere common
	enum {
		kPercussionChannel = 9,
		kUnmapped = 0xff,
		kMaxMasterVolume = 15
	};

	Common::ScopedPtr<MidiDriver> _device;

	/** The state of an output channel. */
	struct Channel {
		/** The program for the channel. */
		uint8 program;
		/** The pitch bend for this channel. */
		uint16 pitchBend;
		/** The vibrato modulation for this channel. */
		uint8 modulation;
		/** The volume for this channel. */
		uint8 volume;
		/** The stereo pan for this channel. */
		uint8 pan;
		/** Whether or not the damper pedal is on for this channel. */
		bool damperPedalOn;
		/** Whether or not output of this channel is enabled. */
		bool enabled;

		/**
		 * The system output channel for this channel. These objects are owned
		 * by the driver in `_device`.
		 */
		MidiChannel_MPU401 *hw;

		Channel() :
			program(kUnmapped),
			pitchBend(0xffff),
			modulation(kUnmapped),
			volume(kUnmapped),
			pan(kUnmapped),
			damperPedalOn(true),
			enabled(true),
			hw(nullptr) {}
	};

	typedef SciSpan<const byte> SysEx;

	/**
	 * Send a DT1 SysEx to the given address.
	 */
	void sendSysEx(const uint32 address, const SysEx &data, const bool skipDelays);

	/**
	 * Send a DT1 SysEx to the given address, incrementing the address and
	 * and SysEx positions automatically by the given size.
	 */
	void sendCountingSysEx(uint32 &address, SysEx &data, const uint8 size);

	/**
	 * Sends a patch set to the given device address, and increments the address
	 * to point to the next free address in the patch bank.
	 */
	void sendPatches(uint32 &address, SysEx data);

	/**
	 * Sends timbre data to the device.
	 */
	void sendTimbres(const uint8 numTimbres, SysEx data);

	/**
	 * Converts and sends the given master volume to the device.
	 */
	void sendMasterVolume(const uint8 volume);

	/**
	 * The ID used within Sound resources for identifying the correct tracks
	 * for this device.
	 */
	DeviceId _deviceId;

	/** Whether or not the MT-32 device is a softsynth. */
	bool _isEmulated;

	/** The last reverb mode passed to the driver. */
	uint8 _reverbMode;

	/** The current master volume. */
	uint8 _masterVolume;

	/** The message written to the MT-32 display on shutdown. */
	Common::FixedArray<byte, 20> _goodbyeSysEx;

	/** The list of preprogrammed reverb mode settings. */
	Common::FixedArray<Common::FixedArray<byte, 3>, 11> _reverbModes;

	/** The output channel state. */
	Common::FixedArray<Channel, kNumChannels> _channels;
};

SoundDriver *makeMt32Driver(ResourceManager &resMan, const SciVersion version);

} // End of namespace Sci

#endif
