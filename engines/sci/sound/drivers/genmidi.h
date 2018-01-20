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

#ifndef SCI_SOUND_DRIVERS_GENMIDI_H
#define SCI_SOUND_DRIVERS_GENMIDI_H

#include "audio/mpu401.h"
#include "common/array.h"
#include "common/debug.h"
#include "common/ptr.h"
#include "sci/sound/drivers/driver.h"
#include "sci/sound/sound.h"

namespace Sci {
static const byte defaultSci32GMPatchMidiData[] = {
	0x9B, 0x00, 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7, 0xB0, 0x65,
	0x00, 0x64, 0x00, 0x06, 0x0C, 0x26, 0x00, 0xB1, 0x65, 0x00, 0x64, 0x00, 0x06, 0x0C, 0x26, 0x00,
	0xB2, 0x65, 0x00, 0x64, 0x00, 0x06, 0x0C, 0x26, 0x00, 0xB3, 0x65, 0x00, 0x64, 0x00, 0x06, 0x0C,
	0x26, 0x00, 0xB4, 0x65, 0x00, 0x64, 0x00, 0x06, 0x0C, 0x26, 0x00, 0xB5, 0x65, 0x00, 0x64, 0x00,
	0x06, 0x0C, 0x26, 0x00, 0xB6, 0x65, 0x00, 0x64, 0x00, 0x06, 0x0C, 0x26, 0x00, 0xB7, 0x65, 0x00,
	0x64, 0x00, 0x06, 0x0C, 0x26, 0x00, 0xB8, 0x65, 0x00, 0x64, 0x00, 0x06, 0x0C, 0x26, 0x00, 0xB9,
	0x65, 0x00, 0x64, 0x00, 0x06, 0x0C, 0x26, 0x00, 0xBA, 0x65, 0x00, 0x64, 0x00, 0x06, 0x0C, 0x26,
	0x00, 0xBB, 0x65, 0x00, 0x64, 0x00, 0x06, 0x0C, 0x26, 0x00, 0xBC, 0x65, 0x00, 0x64, 0x00, 0x06,
	0x0C, 0x26, 0x00, 0xBD, 0x65, 0x00, 0x64, 0x00, 0x06, 0x0C, 0x26, 0x00, 0xBE, 0x65, 0x00, 0x64,
	0x00, 0x06, 0x0C, 0x26, 0x00, 0xBF, 0x65, 0x00, 0x64, 0x00, 0x06, 0x0C, 0x26, 0x00
};

class GeneralMidiDriver final : public SoundDriver {
public:
	enum {
		kMinChannel = 1
	};

	GeneralMidiDriver(ResourceManager &resMan, const SciVersion version, const bool isMt32);
	virtual ~GeneralMidiDriver();

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

	virtual void setReverbMode(const uint8 mode) override { _reverbMode = mode; }

	virtual uint8 getMasterVolume() const override { return _masterVolume; }

	virtual void setMasterVolume(const uint8 volume) override;

	virtual void enable(const bool enabled) override;

	virtual void debugPrintState(Console &con) const override;

private:
	enum {
		kNumPrograms = 128,
		kNumNotes = 128,
		kNumVelocities = 128,
		kNumVelocityMaps = 4,
		kNumChannels = 16,
		kNotesPerOctave = 12,
		kMaxVolume = 127
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
		/** The original program for the channel. */
		uint8 program;
		/** The pitch bend for this channel. */
		uint16 pitchBend;
		/** The vibrato modulation for this channel. */
		uint8 modulation;
		/** The original volume for this channel. */
		uint8 volume;
		/** The stereo pan for this channel. */
		uint8 pan;
		/** Whether or not the damper pedal is on for this channel. */
		bool damperPedalOn;
		/** Whether or not output of this channel is enabled. */
		bool enabled;
		/** The amount to shift notes played on this channel. */
		int8 noteShift;
		/** The amount to shift the volume for this channel. */
		int8 volumeShift;
		/** The velocity map index for this channel. */
		uint8 velocityMap;
		/** The remapped output program for this channel. */
		uint8 outProgram;
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
			noteShift(0),
			volumeShift(0),
			velocityMap(0),
			outProgram(0),
			hw(nullptr) {}
	};

	// TODO: Make common code let us send MIDI data like this, having to parse
	// the messages ourselves seems like it would cause a lot of code
	// redundancy.
	void sendBytes(SciSpan<const byte> data, const bool skipDelays) const;

	bool remapNote(const uint8 channelNo, uint8 &note) const;

	/**
	 * The ID used within Sound resources for identifying the correct tracks
	 * for this device.
	 */
	DeviceId _deviceId;

	/** The last reverb mode passed to the driver. This value is not used. */
	uint8 _reverbMode;

	/** The current master volume. */
	uint8 _masterVolume;

	/**
	 * Whether or not the driver is running in the MT-32 General MIDI emulation
	 * mode. In SSCI, this corresponds to the MT32.DRV in games with
	 * soundtracks composed for General MIDI only.
	 */
	bool _isMt32;

	/** The output channel state. */
	Common::FixedArray<Channel, 16> _channels;

	/** A map from input program # to output program #. */
	Common::FixedArray<uint8, kNumPrograms> _programMap;

	/** A map from program # to number of semitones to shift the output note. */
	Common::FixedArray<int8, kNumPrograms> _noteShift;

	/** A map from program # to amount to shift the output volume. */
	Common::FixedArray<int8, kNumPrograms> _volumeShift;

	/** A map from percussion note # to output percussion note #. */
	Common::FixedArray<uint8, kNumNotes> _percussionMap;

	/** An amount to shift the output volume of percussion. */
	int8 _percussionVolumeShift;

	/** A map from program # to the index of a velocity map in `_velocityMaps`. */
	Common::FixedArray<uint8, kNumPrograms> _programVelocityMap;

	/** A set of maps from input velocity to output velocity. */
	Common::FixedArray<Common::FixedArray<uint8, kNumVelocities>, kNumVelocityMaps> _velocityMaps;
};

SoundDriver *makeGeneralMidiDriver(ResourceManager &resMan, const SciVersion version, const bool isMt32);

} // End of namespace Sci

#endif
