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
	AdLibDriver(ResourceManager &resMan, SciVersion version);
	~AdLibDriver();

	virtual int getNumVoices() const override { return 9; }

	virtual DeviceId getDeviceId() const override { return 0; }

	virtual void getChannelMasks(uint8 &instrumentMask, uint8 &percussionMask) const override {
		instrumentMask = (_version == SCI_VERSION_0_EARLY) ? 1 : 4;
		percussionMask = 0;
	}

	virtual void getRemapRange(uint8 &low, uint8 &high) const override { low = 0; high = 8; }

	virtual void service() override;

	virtual void noteOn(const uint8 channelIndex, const uint8 note, const uint8 velocity) override;

	virtual void noteOff(const uint8 channelIndex, const uint8 note, const uint8 velocity) override;

	virtual void controllerChange(const uint8 channelIndex, const uint8 controllerNo, const uint8 value) override;

	virtual void programChange(const uint8 channelIndex, const uint8 programNo) override;

	virtual void pitchBend(const uint8 channelIndex, const uint16 bend) override;

	virtual void keyPressure(const uint8 channelIndex, const uint8 note, const uint8 pressure) override {
		// no-op
	}

	virtual void channelPressure(const uint8 channelIndex, const uint8 pressure) override {
		// no-op
	}

	virtual void setMasterVolume(const uint8 volume) override;

	virtual void enable(const bool enable) override;

	virtual void debugPrintState(Console &con) const override;

private:
	enum {
		kNumVoices = 9,
		kNumPrograms = 190,
		kRhythmMapSize = 62,
		kNumOperators = kNumVoices * 2,
		kMaxVolume = 63,
		kMaxPan = 127
	};

	enum OplRegister {
		kWaveformSelectEnableRegister = 1,
		kKeySplitRegister = 8,
		kSavekRegister = 0x20,
		kKeyScaleOutputLevelRegister = 0x40,
		kAttackDecayRegister = 0x60,
		kSustainReleaseRegister = 0x80,
		kLowFrequencyNumberRegister = 0xa0,
		kHighFrequencyNumberRegister = 0xb0,
		kPercussonRegister = 0xbd,
		kSpeakerEnableRegisterLow = 0xc0,
		kSpeakerEnableRegisterHigh = 0xc8,
		kWaveformSelectRegister = 0xe0
	};

	struct Channel {
		/** Whether or not the damper pedal is on for this channel. */
		bool damperPedalOn;
		/** The pitch bend for this channel. */
		uint16 pitchBend;
		/** The program for the channel. */
		uint8 program;
		/** The volume for this channel. */
		uint8 volume;
		/** The stereo pan for this channel. */
		uint8 pan;
		/** The number of voices reserved for this channel. TODO: Rename to 'unusable' or 'unused'? This is the number of requested voices still remaining unassigned after all voices have been assigned. */
		uint8 numReservedVoices;
		/** The number of voices assigned to this channel. */
		uint8 numAssignedVoices;
		/** The number of active voices for this channel. */
		uint8 numActiveVoices;

		Channel() :
			damperPedalOn(false),
			pitchBend(0x2000),
			program(0),
			volume(63),
			pan(64),
			numReservedVoices(0),
			numAssignedVoices(0),
			numActiveVoices(0) {}
	};

	Common::FixedArray<Channel, kNumChannels> _channels;

	struct Voice {
		/** Whether or not the damper pedal is on for this voice. */
		bool damperPedalOn;
		/** If true, this voice uses additive synthesis instead of FM synthesis. TODO: Should be two ops? Seems to be more about having information for a second operator, from the patch reading side of things. */
		bool isAdditive;
		/** The current operator states for this voice. */
		struct {
			/** The operator's key scale attenuation level. */
			uint16 keyScaleLevel;
			/** The operator's output attenuation level. */
			uint16 outputLevel;
		} operators[2];
		/** The originally assigned channel for this voice. */
		uint8 originalChannel;
		/** The currently assigned channel for this voice. */
		uint8 currentChannel;
		/** The current note for this voice. */
		uint8 note;
		/** The current velocity for this voice. */
		uint8 velocity;
		/** The current program for this voice. */
		uint8 program;

		Voice() :
			damperPedalOn(false),
			isAdditive(false),
			operators(),
			originalChannel(kUnmapped),
			note(kUnmapped),
			velocity(0),
			program(kUnmapped),
			currentChannel(kUnmapped) {}
	};

	struct Operator {
		uint8 keyScaleLevel;
		uint8 frequencyMultiplicationFactor;
		uint8 feedbackFactor;
		uint8 attackRate;
		uint8 sustainLevel;
		bool sustainOn;
		uint8 decayRate;
		uint8 releaseRate;
		uint8 outputLevel;
		bool tremoloOn;
		bool vibratoOn;
		bool envelopeScalingOn;
		bool isFrequencyModulation;
		uint8 waveform;
	};

	typedef Common::FixedArray<Operator, 2> Program;

	static Operator _defaultProgram[2];

	void voiceOn(const uint8 voiceNo, const uint8 note, const uint8 velocity);
	void voiceOff(const uint8 voiceNo);
	uint8 findFreeVoice(const uint8 channelNo);
	void setChannelNumVoices(const uint8 channelIndex, const uint8 numVoices);

	void assignVoices(const uint8 channelIndex, const uint8 numVoices);
	void releaseVoices(const uint8 channelIndex, const uint8 numVoices);

	void sendNote(const uint8 voiceNo, const bool noteOn);

	void sendToHardware(const uint8 registerNo, const uint8 value);
	void sendLeft(const uint8 registerNo, const uint8 value);
	void sendRight(const uint8 registerNo, const uint8 value);
	void resetOpl();

	void setVoiceProgram(const uint8 voiceNo, const uint8 programNo);

	void updateLru(const uint8 voiceNo);

	void sendOperator(const uint8 opNo, const Operator &op);
	void sendVolume(const uint8 voiceNo, const uint8 volume);

	Common::FixedArray<Voice, kNumVoices> _voices;

	Common::FixedArray<uint8, kNumVoices> _lruVoice;

	Common::FixedArray<Operator, kNumOperators> _operators;
	Common::FixedArray<Program, kNumPrograms> _programs;
	Common::FixedArray<uint8, kRhythmMapSize> _rhythmMap;

	Common::ScopedPtr<OPL::OPL> _opl;

	bool _isStereo;
};

SoundDriver *makeAdLibDriver(ResourceManager &resMan, SciVersion version);

} // End of namespace Sci

#endif
