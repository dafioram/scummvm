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

#ifndef SCI_SOUND_SOUND_H
#define SCI_SOUND_SOUND_H

#include "common/array.h"
#include "common/mutex.h"
#include "common/ptr.h"
#include "common/serializer.h"
#include "common/scummsys.h"
#include "sci/engine/vm_types.h"
#include "sci/resource.h"
#include "sci/util.h"

namespace Audio { class Mixer; }

namespace Sci {
class Sci1SoundDriver;

enum Sci0PlayStrategy {
	/**
	 * The caller should synchronously invoke `service` on the driver until
	 * `signal` is kFinished.
	 */
	kSync = 0,

	/**
	 * The caller should invoke `service` once on the driver and then
	 * return.
	 */
	kAsync = 1,

	/**
	 * Playback did not begin; the caller should clean up immediately.
	 */
	kAbort = 3
};

/**
 * A representation of the current state of a sound in SCI0.
 */
struct Sci0Sound {
	// TODO: In SSCI, the first field is a Node, is this relevant?

	/**
	 * The sound resource data.
	 *
	 * @note In SSCI this is a raw pointer to the resource data.
	 */
	Resource *data;

	int16 numLoops;

	/**
	 * The current position of playback in the sound resource, in bytes.
	 */
	int16 position;

	/**
	 * The priority of the sound. See `SciSound::priority`.
	 */
	int16 priority;

	/**
	 * The playback strategy for the sound.
	 */
	Sci0PlayStrategy strategy;

	// uint16 chain; // TODO: May be unused, replaced by priority

	int16 state;

	uint16 signal;

	/**
	 * The playback volume of the sound, TODO: from 0 to 15?
	 */
	int16 volume;

	// uint16 effect; // TODO: Not a clue what this is
};

/**
 * A representation of the current state of a sound in SCI1+.
 */
struct Sci1Sound {
	enum {
		kNumTracks = 16,
		kNumChannels = 15
	};

	enum ChannelFlags {
		/**
		 * Normal channel.
		 */
		kNoFlags = 0,

		/**
		 * Channel cannot be preempted or mapped to a different output channel.
		 */
		kLocked = 1,

		/**
		 * The channel is outside the range representable on a single MIDI port
		 * (>15).
		 */
		kOutOfRange = 2
	};

	enum Signal {
		/** No event. */
		kNoSignal = 0,

		/** A fade has finished. */
		kFadeEnded = 0xFE,

		/** The sound has finished playback. */
		kFinished = 0xFF
	};

	enum State {
		/** No sound is playing. */
		kNoSound = 0,

		/** A normal sound created by `play` is playing. */
		kNormalSound = 1,

		/**
		 * A sound created by `playBed` is playing. These differ from normal
		 * sounds by (1) allowing all channels, (2) having channels which
		 * are directly mapped onto the MIDI device instead of being dynamically
		 * allocated by the MIDI system, and (3) may not be preempted by another
		 * sound. TODO: No longer used by around SCI1mid?
		 */
		kBedSound = 2
	};

	/**
	 * The raw Sound resource being played.
	 */
	Resource *resource;

	struct Track {
		enum SpecialChannel {
			/** Track is the end-of-file marker. */
			kEndOfTracks = -1,

			/** Track contains a digital sample. */
			kSampleTrack = -2
		};

		/** The current playback position of the track, in bytes. */
		uint16 position;

		/**
		 * TODO: Something to do with delaying for N ticks to avoid polyphony
		 * overflow?
		 * TODO: Rename to 'delay'
		 */
		uint16 rest;

		/** TODO: The currently active operation?? for the track? */
		int8 command;

		/**
		 * The channel for the track, with extra bits used for properties.
		 * The bit layout is:
		 * 0-3 channel number
		 * 4   out of range
		 * 5   locked
		 * 6   muted
		 */
		int8 channelNo;

		/** The position of the start of the current sound loop, in bytes. */
		uint16 loopPosition;

		/** The state of `rest` at the loop point. */
		uint16 loopRest;

		/** The state of `command` at the loop point. */
		int8 loopCommand;
	} tracks[kNumTracks];

	struct Channel {
		// NOTE: In SSCI, damper pedal state & pitch bend are packed into a
		// single word where the pitch bend is mask 0x7f7f and the damper pedal
		// flag is mask 0x8000.

		/** Whether or not the damper pedal is on. */
		bool damperPedalOn;

		/** MIDI pitch bend value. */
		int16 pitchBend;

		// NOTE: In SSCI, polyphony & priority are packed into a single byte,
		// with priority in the high 4 bits and polyphony in the low 4 bits.

		/** Number of voices, 0-15. */
		int8 polyphony;

		/** Priority of the channel, 0-15. */
		int8 priority;

		/** MIDI amount of modulation. */
		int8 modulation;

		/** MIDI pan, L 0 C 64 R 127. */
		int8 pan;

		/** MIDI volume, 0-127. */
		int8 volume;

		/** MIDI program, 0-127. */
		int8 program;

		/**
		 * The currently playing note for this channel, 0-127. If no note is
		 * played, the value is -1.
		 */
		int8 currentNote;

		ChannelFlags flags;

		// NOTE: In SSCI, game mute & sound mute are packed into a single
		// byte, with engine mute in the high 4 bits and sound mute in the low
		// 4 bits.

		/**
		 * Number of times the channel was programmatically muted.
		 * TODO: Removed around mid-SCI1.1?
		 */
		uint8 gameMuteCount;

		/** Whether or not the channel was muted by the Sound resource itself. */
		bool muted;
	} channels[kNumChannels];

	/** Data increment cue value. */
	uint32 dataInc;

	/**
	 * Current playback position in ticks.
	 * TODO: was timer/ticker
	 */
	uint32 ticksElapsed;

	/**
	 * The state of `ticksElapsed` at the loop point.
	 * TODO: was loopTime
	 */
	uint32 loopTicksElapsed;

	/**
	 * A signal representing playback state.
	 */
	Signal signal;

	/**
	 * A value representing the type of sound playing back.
	 */
	State state;

	/**
	 * TODO: If true, hold at the loop?
	 */
	bool hold;

	/**
	 * If true, the value in `priority` overrides the priority given in the
	 * Sound resource.
	 * @see `ScreenItem::_fixedPriority`
	 */
	bool fixedPriority;

	/**
	 * The priority of this sound (0-15). Sounds with higher values are placed
	 * in the playback queue ahead of other sounds and will preempt playback of
	 * sounds with lower priorities.
	 */
	int8 priority;

	/**
	 * If true, the sound will loop at the (TODO: nearest?) loop point.
	 */
	bool loop;

	/**
	 * The MIDI volume of the sound (0-127).
	 */
	uint8 volume;

	/**
	 * The reverb mode.
	 */
	uint8 reverbMode;

	// NOTE: In SSCI, target volume & stop sound are packed into a single byte
	// where the volume is mask 0x7f and the stop sound flag is mask 0x80.

	/**
	 * The target volume of the fade (0-127).
	 * TODO: was fadeDest/fadeTo
	 */
	uint8 fadeTargetVolume;

	/**
	 * If true, stops the sound after the fade is complete.
	 */
	bool stopSoundOnFade;

	/**
	 * The amount of time, in ticks, between each step of the fade.
	 * TODO: was fadeTicks/fadeTicker
	 * @see `PlaneShowStyle::delay`
	 */
	uint8 fadeDelay;

	/**
	 * The amount of time remaining, in ticks, until the next step of the
	 * currently active fade.
	 * TODO: was fadeCount/fadeStep
	 */
	uint8 fadeDelayRemaining;

	/**
	 * The amount of volume adjustment to perform on each step of the fade.
	 */
	uint8 fadeSteps;

	/**
	 * If true, playback of the sound is paused.
	 * TODO: Unclear if this was ever a counter prior to SCI1.1.
	 */
	bool paused;

	/**
	 * If true, this sound is actually a digital sample which should be played
	 * through Audio/Audio32. Added around SCI1.1?
	 */
	bool isSample;

	byte peek(const int8 trackNo, const int8 extra = 0) {
		const uint16 trackOffset = resource->getUint16LEAt(trackNo * sizeof(uint16));
		return resource->getUint8At(trackOffset + tracks[trackNo].position + extra);
	}

	byte consume(const int8 trackNo) {
		const byte message = peek(trackNo);
		++tracks[trackNo].position;
		return message;
	}

	void advance(const int8 trackNo) {
		++tracks[trackNo].position;
	}
};

class SoundManager : public Common::Serializable {
public:
	SoundManager(ResourceManager &resMan, GameFeatures &features);
	~SoundManager();

	virtual void saveLoadWithSerializer(Common::Serializer &s) override;

private:
	Audio::Mixer &_mixer;
	ResourceManager &_resMan;
	Common::Mutex _mutex;

	/**
	 * The list of currently playing & queued sounds.
	 */
	Common::Array<Sci1Sound> _sounds;

	/**
	 * The version of the sound code.
	 */
	SciVersion _soundVersion;

	/**
	 * The sound driver.
	 */
	Common::ScopedPtr<Sci1SoundDriver> _driver;

#pragma mark -
#pragma mark MIDI server
// TODO: Split to SCI0 server and SCI1 server?
public:
	/**
	 * Enables and disables the sound server. This method uses a counter so
	 * requires a symmetric number of enable and disable calls.
	 */
	void enableSoundServer(const bool enable);

private:
	static inline void soundServerCallback(void *soundManager) {
		static_cast<SoundManager *>(soundManager)->soundServer();
	}

	void soundServer();
	void fadeSound(Sci1Sound &sound);
	// TODO: was parseNode
	void parseNextNode(Sci1Sound &sound, const int8 playlistIndex);

	void updateChannelVolumes();

	// TODO: was doChannelList
	void updateChannelList();

	// TODO: was playPos
	// TODO: Don't use state to retain temporaries like this,
	// pass as a parameter.
	// playlist index in upper 4 bits, lower 4 bits unused.
	uint8 _playlistIndex;
	// TODO: was ghostChnl
	// TODO: Checked only in PitchBend, Controller, ProgramChange
	bool _outOfRangeChannel;

	enum {
		kUnknownChannel = -1,
		kControlChannel = 15,

		// TODO: used for rest/delay
		kTimingOverflowFlag = 0x8000
	};

	enum {
		kNoCurrentNode = -1
	};

	enum Message {
		kCommandFlag = 0x80,
		kTimingOverflow = 0xf8,
		kEndOfTrack = 0xfc
	};

	enum Command {
		kNoteOff = 0x80,
		kNoteOn = 0x90,
		kKeyPressure = 0xa0,
		kControllerChange = 0xb0,
		kProgramChange = 0xc0,
		kChannelPressure = 0xd0,
		kPitchBend = 0xe0,
		kSysEx = 0xf0,

		kEndOfSysEx = 0xf7
	};

	enum Controller {
		kModulationController = 1,
		kVolumeController = 7,
		kPanController = 10,
		kDamperPedalController = 64,
		kMaxVoicesController = 75,
		kSingleVoiceNoteController = 78,
		kAllNotesOffController = 123
	};

	enum {
		kPlaylistSize = 16,
		kNumMappedChannels = 16
	};

	// TODO: was sampleList, appears totally broken in SCI1.1+ so unclear what
	// to do about it. In SSCI this is a 16-dword array, but the code uses it
	// to hold a pointer in _samples[0], or an offset in the first word of
	// _samples[0] and segment segment the first word of _samples[2]. The
	// pointer is to a Sci1Sound object.
	Sci1Sound *_sample;

	Sci1Sound *_playlist[kPlaylistSize];

	// TODO: Turn this into a struct. key (ChList), node (ChNodes)
	typedef Common::Array<uint8> ChannelList;

	// TODO: was chList
	// Index maps to mappedChannels. Original channel's position in _playlist in
	// upper 4 bits, original channel number in lower 4 bits.
	ChannelList _channelList;

	// TODO: Not sure if this is the right name for this thing. Maybe
	// PhysicalChannel or OutputChannel or HardwareChannel instead?
	struct MappedChannel {
		enum {
			kUnmapped = 0xFF
		};

		/**
		 * The key (playlist index << 4 | channel number) for this channel
		 * remapping.
		 * TODO: was ChNew
		 */
		uint8 key;

		/**
		 * The global priority (sound priority * channel priority) of the mapped
		 * channel (0-240).
		 * TODO: was ChPri
		 */
		uint8 priority;

		/**
		 * Number of voices used by the channel.
		 * TODO: was ChVoice
		 */
		int8 numVoices;

		/**
		 * TODO: was ChBed
		 */
		bool locked;

		MappedChannel() :
			key(kUnmapped),
			priority(0),
			numVoices(0),
			locked(false) {}

		inline int8 playlistIndex() const { return key >> 4; }
		inline void playlistIndex(const int8 index) { key = (index << 4) | (key & 0xf); }
		inline int8 channelNo() const { return key & 0xf; }
		inline void channelNo(const int8 number) { key = (key & 0xf0) | number; }
	};

	// TODO: Combine these next two things with Playlist?
	typedef Common::Array<MappedChannel> MappedChannels;
	MappedChannels _mappedChannels;

	typedef Common::Array<Sci1Sound *> MappedNodes;
	MappedNodes _mappedNodes;

	enum {
		kNoVolumeChange = -1
	};

	/**
	 * A list of new volume changes per channel.
	 */
	Common::Array<int8> _newChannelVolumes;

	// TODO: was requestChnl
	int8 _newChannelVolumesIndex;

	// TODO: was updateChannel1 & 2 (identical)
	void updateChannel(const Sci1Sound &sound, const Sci1Sound::Channel &channel, const int8 hwChannelNo);

	// TODO: was preemptChn1 & 2 (identical)
	int8 preemptChannel(const int &numFreeVoices);

	// TODO: was controlChnl
	void parseControlChannel(Sci1Sound &sound);

	void sendNoteOff(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo);
	void sendNoteOn(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo);
	void sendKeyPressure(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo);
	void sendControllerChange(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo, const bool outOfRangeChannel);
	void sendProgramChange(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo, const bool outOfRangeChannel);
	void sendChannelPressure(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo);
	void sendPitchBend(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo, const bool outOfRangeChannel);
	void sendSysEx(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo);
	void skipCommand(Sci1Sound &sound, const int8 trackNo, const Command command);

	// TODO: was DoEnd
	void finishSound(Sci1Sound &sound);

	/**
	 * The number of times the sound server has been suspended.
	 */
	int _numServerSuspensions;

	enum {
		kDefaultReverbMode = 127,
		kMaxVolume = 127
	};

	uint8 _defaultReverbMode;

	bool _needsUpdate;
	bool _restoringSaveGame;

#pragma mark -
#pragma mark Kernel
public:
};

} // End of namespace Sci
#endif
