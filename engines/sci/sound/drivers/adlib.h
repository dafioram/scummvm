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

	virtual void getRemapRange(uint8 &low, uint8 &high) const override {
		low = 0;
		high = _version >= SCI_VERSION_1_1 ? 8 : 15;
	}

	virtual void service() override;

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
		/**
		 * This register globally enables waveform selection for operators
		 * through the Waveform Select register.
		 */
		kWaveformSelectEnableRegister = 1,

		/**
		 * This register globally sets the bit used from the frequency number
		 * when determining which note in an octave is the split point for the
		 * octave.
		 */
		kKeySplitRegister = 8,

		/**
		 * This register enables tremolo, vibrato, sustain, & envelope scaling
		 * for an operator, and controls the frequency multiplication factor for
		 * the frequency number given in the frequency number register for the
		 * associated voice.
		 */
		kSavekRegister = 0x20,

		/**
		 * This register controls the attenuation of an operator (Output
		 * Level) as well as additional attenuation applied to every octave (Key
		 * Scale).
		 */
		kKeyScaleOutputLevelRegister = 0x40,

		/**
		 * This register controls the attack and decay rates for an operator's
		 * ADSR envelope.
		 */
		kAttackDecayRegister = 0x60,

		/**
		 * This register controls the sustain and release rates for an
		 * operator's ADSR envelope.
		 */
		kSustainReleaseRegister = 0x80,

		/**
		 * This register controls the frequency of a voice. It receives the low
		 * 8 bits of the frequency number.
		 */
		kLowFrequencyNumberRegister = 0xa0,

		/**
		 * This register controls the frequency of a voice and whether or not
		 * the voice is playing. It receives the high 2 bits of the frequency
		 * number, the frequency block number, and the playing bit.
		 */
		kHighFrequencyNumberRegister = 0xb0,

		/**
		 * This register globally controls the tremolo and vibrato depth,
		 * percussion mode, and controls whether or not each of the five
		 * percussion channels are playing when percussion mode is enabled.
		 */
		kPercussionRegister = 0xbd,

		/**
		 * This register controls the output channel (OPL3 only), synthesis
		 * type, and feedback level and of a voice.
		 */
		kSynthTypeRegister = 0xc0,

		/**
		 * This register selects the waveform for an operator.
		 */
		kWaveformSelectRegister = 0xe0
	};

	/** Sound resource channel state. */
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
		/** The number of inactive extra voices for this channel. */
		uint8 numInactiveExtraVoices;
		/** The number of active extra voices assigned to this channel. */
		uint8 numActiveExtraVoices;
		/** The number of non-extra active voices for this channel. */
		uint8 numActiveVoices;

		Channel() :
			damperPedalOn(false),
			pitchBend(0x2000),
			program(0),
			volume(63),
			pan(64),
			numInactiveExtraVoices(0),
			numActiveExtraVoices(0),
			numActiveVoices(0) {}
	};

	/** OPL voice state. SCI always uses 2-op voices. */
	struct Voice {
		/** Whether or not the damper pedal is on for this voice. */
		bool damperPedalOn;

		/** If true, this voice uses AM synthesis instead of FM synthesis. */
		bool isAM;

		/** The current operator states for this voice. */
		struct {
			/** The operator's key scale attenuation level. */
			uint16 keyScaleLevel;
			/** The operator's output attenuation level. */
			uint16 outputLevel;
		} operators[2];

		/**
		 * The explicitly assigned channel for this voice, or `kUnmapped` if
		 * there is no explicitly assigned channel.
		 */
		uint8 originalChannel;

		/**
		 * The extra channel assigned to this voice, or `kUnmapped` if no extra
		 * channel is assigned.
		 */
		uint8 extraChannel;

		/**
		 * The current note for this voice, or `kUnmapped` if no note is
		 * playing.
		 */
		uint8 note;

		/** The current velocity for this voice. */
		uint8 velocity;

		/** The current program for this voice. */
		uint8 program;

		Voice() :
			damperPedalOn(false),
			isAM(false),
			operators(),
			originalChannel(kUnmapped),
			note(kUnmapped),
			velocity(0),
			program(kUnmapped),
			extraChannel(kUnmapped) {}
	};

	/** OPL operator parameters. */
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

	/** 2-op OPL program. */
	typedef Common::FixedArray<Operator, 2> Program;

	// TODO: Get rid of these if all SSCI versions always set operators from
	// patch data
	static const Operator _defaultOperators[2];

	/**
	 * Reads a program from the patch data and advances the data to point to the
	 * next program or end of program data.
	 */
	void readProgramFromPatch(SciSpan<const byte> &data, Program &program);

	/**
	 * Changes the number of voices assigned to the given channel.
	 */
	void setChannelExtraVoices(const uint8 channelNo, const uint8 numVoices);

	/**
	 * Turns on a note for the given voice. This will not retrigger
	 * already-playing notes.
	 */
	void voiceOn(const uint8 voiceNo, const uint8 note, const uint8 velocity);

	/**
	 * Turns off the voice.
	 */
	void voiceOff(const uint8 voiceNo);

	/**
	 * Updates the least-recently-used voice list.
	 */
	void updateLru(const uint8 voiceNo);

	/**
	 * Finds the best free voice for playback of a new note for the given
	 * channel. If no free voice could be found, kUnmapped is returned.
	 */
	uint8 findFreeVoice(const uint8 channelNo);

	/**
	 * Tries to assign up to `numVoices` extra voices to the given channel.
	 */
	void assignExtraVoices(const uint8 channelNo, const uint8 numVoices);

	/**
	 * Releases up to `numVoices` extra voices from the given channel.
	 */
	void releaseExtraVoices(const uint8 channelNo, const uint8 numVoices);

	/**
	 * Sets the program for a voice.
	 */
	void setVoiceProgram(const uint8 voiceNo, const uint8 programNo);

	/**
	 * Sets the volume of a voice.
	 */
	void setVoiceVolume(const uint8 voiceNo, const uint8 volume);

	/**
	 * Sends a note for the given voice to hardware.
	 */
	void sendNote(const uint8 voiceNo, const bool noteOn);

	/**
	 * Programs the given operator number with the given Operator parameters.
	 */
	void sendOperator(const uint8 opNo, const Operator &op);

	/**
	 * Writes the given register to hardware.
	 */
	void sendToHardware(const uint8 registerNo, const uint8 value);

	/**
	 * Writes the given register to the hardware's right output channel.
	 */
	void sendLeft(const uint8 registerNo, const uint8 value);

	/**
	 * Writes the given register to the hardware's left output channel.
	 */
	void sendRight(const uint8 registerNo, const uint8 value);

	/** Resets all OPL registers. */
	void resetOpl();

	/** Information on input sound channels. */
	Common::FixedArray<Channel, kNumChannels> _channels;

	/** Information on OPL voices. */
	Common::FixedArray<Voice, kNumVoices> _voices;

	/**
	 * List of least recently used voices, where the 0 is the least recently
	 * used voice.
	 */
	Common::FixedArray<uint8, kNumVoices> _lruVoice;

	/** List of program patch data. */
	Common::FixedArray<Program, kNumPrograms> _programs;

	/**
	 * A note-to-note map for voices assigned to percussion. Used by SCI1.1+.
	 */
	Common::FixedArray<uint8, kRhythmMapSize> _rhythmMap;

	/** The output OPL device. */
	Common::ScopedPtr<OPL::OPL> _opl;

	/** Whether or not output should be in stereo. */
	bool _isStereo;
};

SoundDriver *makeAdLibDriver(ResourceManager &resMan, SciVersion version);

} // End of namespace Sci

#endif
