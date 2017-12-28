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

	/**
	 * Sound signals for engine events.
	 */
	enum Signal {
		/** No event. */
		kNoSignal = 0,

		/** The fade has finished. */
		kFadeFinished = 0xfe,

		/** The sound has finished. */
		kFinished = 0xff
	};

	/**
	 * The sound playback state.
	 */
	enum State {
		/**
		 * The sound is not playing.
		 */
		kStopped = 0,

		/**
		 * The sound is playing.
		 */
		kPlaying = 1,

		/**
		 * A sound created by `playBed` is playing.
		 *
		 * Sounds played back normally have 15 instrument channels plus a
		 * control channel, and the instrument channels are dynamically remapped
		 * to hardware channels algorithmically.
		 *
		 * Sounds played back exclusively with `playBed` instead use a 1:1
		 * mapping from sound channel to hardware channel, so offer 15
		 * instrument channels plus a percussion channel instead. They also may
		 * not be preempted by other sounds with higher priorities.
		 */
		kExclusive = 2
	};

	/**
	 * The Sound's resource.
	 */
	Resource *resource;

	/**
	 * A MIDI data stream.
	 */
	struct Track {
		enum SpecialChannel {
			/** Track is at the end of sound data. */
			kEndOfData = 0xff,

			/** Track contains a digital sample. */
			kSampleTrack = 0xfe
		};

		/**
		 * The current playback position of the track, in bytes.
		 */
		uint16 position;

		/**
		 * The number of ticks to wait until processing the next message in the
		 * track. The high bit, when set, indicates that a fixed amount of time
		 * (4 seconds) was held from a single delay message.
		 * TODO: Rename to 'delay'
		 */
		uint16 rest;

		/**
		 * The last received command in the high 4 bits, the channel for the
		 * command in the low 4 bits.
		 * @see `SoundManager::Command`
		 */
		uint8 command;

		/**
		 * The sound channel for this track (0-15).
		 */
		uint8 channelNo;

		/**
		 * The state of `position` at the start point of the current loop.
		 */
		uint16 loopPosition;

		/**
		 * The state of `rest` at the start point of the current loop.
		 */
		uint16 loopRest;

		/**
		 * The state of `command` at the start point of the current loop.
		 */
		uint8 loopCommand;

		Track() :
			position(13),
			rest(0),
			command(0),
			channelNo(kEndOfData),
			loopPosition(3),
			loopRest(0),
			loopCommand(0) {}
	} tracks[kNumTracks];

	/**
	 * A logical MIDI channel.
	 */
	struct Channel {
		/**
		 * Flags representing different channel output configurations.
		 */
		enum ChannelFlags {
			/**
			 * The channel will follow the normal rules for mapping and
			 * preemption.
			 */
			kNoFlags = 0,

			/**
			 * The channel cannot be preempted or mapped to a different output
			 * channel.
			 */
			kLocked = 1,

			/**
			 * The channel is played directly to the same-numbered output
			 * channel, but only when no other logical channel has been remapped
			 * onto the same output channel.
			 * TODO: Rename
			 */
			kExtra = 2
		};

		enum {
			kNoCurrentNote = 0xff,
			kUninitialized = 0xff
		};

		// NOTE: In SSCI, damper pedal state & pitch bend are packed into a
		// single word where the pitch bend is mask 0x7f7f and the damper pedal
		// flag is mask 0x8000.

		/**
		 * Whether or not the damper pedal is on.
		 */
		bool damperPedalOn;

		/**
		 * The current pitch bend for the channel, where 0x2000 is no bend.
		 * This value is held here as a single 14-bit number, not two 7-bit
		 * bytes.
		 */
		uint16 pitchBend;

		// NOTE: In SSCI, polyphony & priority are packed into a single byte,
		// with priority in the high 4 bits and polyphony in the low 4 bits.

		/**
		 * The maximum number of voices used by this channel, 0-15.
		 */
		uint8 numVoices;

		/**
		 * The priority of the channel, 0-15, where larger numbers represent
		 * lower priority. Channels with a priority of 0 cannot be preempted.
		 * This is combined with the Sound's priority to generate a global
		 * channel priority list for the entire playlist.
		 */
		uint8 priority;

		/**
		 * The level of vibrato applied to the channel.
		 */
		uint8 modulation;

		/**
		 * The stereo pan of the channel, L 0 C 64 R 127.
		 */
		uint8 pan;

		/**
		 * The volume of the channel, 0-127. This is combined with the Sound
		 * volume to generate a final output volume.
		 */
		uint8 volume;

		/**
		 * The currently selected program (instrument) for the channel, 0-127.
		 */
		uint8 program;

		/**
		 * The currently playing note for this channel, 0-127. If no note is
		 * playing, the value is `kNoCurrentNote`.
		 */
		uint8 currentNote;

		/**
		 * Output configuration flags for the channel.
		 */
		ChannelFlags flags;

		// NOTE: In SSCI, game mute & sound mute are packed into a single
		// byte, with engine mute in the high 4 bits and sound mute in the low
		// 4 bits.

		/**
		 * The number of times the channel was programmatically muted.
		 * TODO: Removed around mid-SCI1.1?
		 */
		uint8 gameMuteCount;

		/**
		 * Whether or not the channel was muted by the Sound resource itself.
		 */
		bool muted;

		Channel() :
			damperPedalOn(false),
			pitchBend(0x2000),
			numVoices(kUninitialized),
			priority(kUninitialized),
			modulation(0),
			pan(kUninitialized),
			volume(kUninitialized),
			program(kUninitialized),
			currentNote(kNoCurrentNote),
			flags(kNoFlags),
			gameMuteCount(0),
			muted(false) {}
	} channels[kNumChannels];

	/**
	 * A cue value which increments by one whenever a cue controller message is
	 * encountered in the control channel.
	 * TODO: was dataInc
	 */
	uint16 cue;

	/**
	 * The current playback position, in ticks.
	 * TODO: was timer/ticker
	 */
	uint16 ticksElapsed;

	/**
	 * The state of `ticksElapsed` at the last loop point.
	 * TODO: was loopTime
	 */
	uint16 loopTicksElapsed;

	/**
	 * The last playback event that occurred. These values may be generated by
	 * the engine itself or received in the sound's control channel.
	 */
	Signal signal;

	/**
	 * The current playback state of the sound.
	 */
	State state;

	/**
	 * If non-zero, sound playback will be stopped once the hold point matching
	 * the given value is reached in the control channel.
	 */
	uint8 holdPoint;

	/**
	 * If true, any priority value given in the sound data will be ignored.
	 * @see `ScreenItem::_fixedPriority`
	 */
	bool fixedPriority;

	/**
	 * The priority of this sound (0-15). Sounds with higher values are placed
	 * in the playback queue ahead of other sounds and will preempt playback of
	 * sounds with lower priorities. This is combined with Channel priorities to
	 * generate a global channel priority list for the entire playlist.
	 */
	uint8 priority;

	/**
	 * If true, the sound will loop at the last point given by a `kSetLoop`
	 * message in the sound's control channel.
	 */
	bool loop;

	/**
	 * The overall volume of the sound (0-127). This is combined with Channel
	 * volumes to generate a final output volume for each channel.
	 */
	uint8 volume;

	/**
	 * The reverb mode (0-10).
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
	 * TODO: was fadeSteps
	 */
	uint8 fadeAmountPerTick;

	/**
	 * If non-zero, playback of the sound is paused.
	 * TODO: Unclear if this was ever a counter prior to SCI1.1.
	 */
	uint8 paused;

	/**
	 * If true, the sound is actually a digital sample which should be played
	 * through Audio/Audio32. TODO: Added around SCI1.1?
	 */
	bool isSample;

	/**
	 * Peeks at a byte of the data stream for the given track.
	 */
	inline byte peek(const uint8 trackNo, const uint8 extra = 0) {
		const uint16 trackOffset = resource->getUint16LEAt(trackNo * sizeof(uint16));
		return resource->getUint8At(trackOffset + tracks[trackNo].position + extra);
	}

	/**
	 * Consumes the next byte of the data stream for the given track.
	 */
	inline byte consume(const uint8 trackNo) {
		const byte message = peek(trackNo);
		++tracks[trackNo].position;
		return message;
	}

	/**
	 * Advances the data stream for the given track by one byte.
	 */
	inline void advance(const uint8 trackNo) {
		++tracks[trackNo].position;
	}
};

class SoundManager : public Common::Serializable {
public:
	SoundManager(ResourceManager &resMan, GameFeatures &features);
	~SoundManager();

	virtual void saveLoadWithSerializer(Common::Serializer &s) override;

	/**
	 * Returns the preferred resource type for the given sound resource.
	 */
	ResourceType getSoundResourceType(const uint16 resourceNo) const {
		if (_preferSampledSounds && _resMan.testResource(ResourceId(kResourceTypeAudio, resourceNo))) {
			return kResourceTypeAudio;
		} else {
			return kResourceTypeSound;
		}
	}

private:
	ResourceManager &_resMan;
	Common::Mutex _mutex;

	/**
	 * If true, prefer sampled sound effects over synthesised sound effects when
	 * possible.
	 */
	bool _preferSampledSounds;

	/**
	 * The list of currently playing & queued sounds.
	 */
	Common::Array<Sci1Sound> _sounds;

	/**
	 * The currently active version of the sound code.
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
	// TODO: Public APIs are:
	// PatchReq (unused)
	// Init (unused)
	// Terminate (unused)
	// ProcessSounds (semi-private) ~> soundServer
	// SoundOn ~> setSoundOn
	// RestoreSound (semi-private) ~> restore
	// MasterVol ~> get/setMasterVolume
	// SetReverb ~> get/getDefault/setReverb
	// PlaySound ~> play
	// EndSound ~> stop
	// PauseSound ~> pause/pauseAll
	// FadeSound ~> fade
	// HoldSound ~> hold
	// MuteSound ~> mute
	// ChangeVol ~> setVolume
	// ChangePri ~> setPriority
	// GetDataInc ~> getCue
	// GetSignal ~> peekSignal
	// GetSignalRset ~> consumeSignal
	// GetSMPTE ~> getPosition
	// SendNoteOff ~> setNoteOff
	// SendNoteOn ~> setNoteOn
	// SendContrlr ~> setController
	// SendPChange ~> setProgram
	// SendPBend ~> setPitchBend
	// AskDriver (unused)

	enum {
		/** Maximum master volume. */
		kMaxMasterVolume = 15,

		/** Maximum channel volume. */
		kMaxVolume = 127
	};

	/**
	 * Enables and disables the sound server. This method uses a counter so
	 * requires a symmetric number of enable and disable calls.
	 */
	void enableSoundServer(const bool enable);

	/**
	 * Enables and disables sound reproduction in the driver. This method does
	 * not use a counter so one call to enable sound will cancel all previous
	 * calls to disable sound.
	 */
	void setSoundOn(const bool enable);

	// TODO: was PlaySound. returns playlist index
	uint8 play(Sci1Sound &sound, const bool isBedSound);

	// TODO: was MasterVol, one function
	uint8 getMasterVolume() const;
	uint8 setMasterVolume(const uint8 volume);

	// TODO: was SetReverb, one function
	uint8 getReverbMode() const;
	uint8 getDefaultReverbMode() const { return _defaultReverbMode; }
	uint8 setReverbMode(const uint8 reverbMode);

	// TODO: was GetSignal
	Sci1Sound::Signal peekSignal(const Sci1Sound &sound);

	// TODO: was GetSignalRset
	Sci1Sound::Signal consumeSignal(Sci1Sound &sound);

	// TODO: was GetDataInc
	uint16 getCue(const Sci1Sound &sound);

	// TODO: was GetSMPTE
	struct Position { uint16 minutes; uint8 seconds; uint8 frames; };
	Position getPosition(const Sci1Sound &sound);

	// TODO: was RestoreSound
	void restore(Sci1Sound &sound);

	// TODO: was ChangePri
	void setPriority(Sci1Sound &sound, const uint8 priority);

	// TODO: was EndSound
	void stop(Sci1Sound &sound);

	// TODO: was FadeSound
	void fade(Sci1Sound &sound, const int16 targetVolume, const int16 speed, const int16 steps, const bool stopAfterFade);

	// TODO: was ChangeVolume
	void setVolume(Sci1Sound &sound, const uint8 volume);

	// TODO: was PauseSound, one function
	void pause(Sci1Sound &sound, const bool pause);
	void pauseAll(const bool pause);

	// TODO: was HoldSound
	void hold(Sci1Sound &sound, const uint8 holdPoint);

	// TODO: was SendNoteOff
	void setNoteOff(Sci1Sound &sound, const uint8 channelNo, const uint8 note, const uint8 velocity);

	// TODO: was SendNoteOn
	void setNoteOn(Sci1Sound &sound, const uint8 channelNo, const uint8 note, const uint8 velocity);

	// TODO: was SendContrlr
	void setController(Sci1Sound &sound, const uint8 channelNo, const uint8 controllerNo, const uint8 value);

	// TODO: was SendPChange
	void setProgram(Sci1Sound &sound, const uint8 channelNo, const uint8 programNo);

	// TODO: was SendPitchBend
	void setPitchBend(Sci1Sound &sound, const uint8 channelNo, const uint16 value);

	// TODO: was MuteSound
	void mute(Sci1Sound &sound, const bool mute);

private:
	static inline void soundServerCallback(void *soundManager) {
		static_cast<SoundManager *>(soundManager)->soundServer();
	}

	void soundServer();

	void processFade(Sci1Sound &sound);
	// TODO: was parseNode
	void parseNextNode(Sci1Sound &sound, const uint8 playlistIndex);

	void updateChannelVolumes();

	// TODO: was doChannelList
	void updateChannelList();

	/**
	 * Finds the index of the given sound in the current playlist. Returns
	 * `kPlaylistSize` if the sound could not be found in the playlist.
	 */
	inline uint8 findPlaylistIndex(const Sci1Sound &sound) const {
		uint i;
		for (i = 0; i < kPlaylistSize; ++i) {
			if (_playlist[i] == &sound) {
				break;
			}
		}
		return i;
	}

	enum {
		kUnknownSound = 0xff
	};

	/**
	 * Creates a channel key in the format used for `_channelList`.
	 */
	inline uint8 makeChannelKey(const Sci1Sound &sound, const uint8 channelNo) const {
		const uint8 playlistIndex = findPlaylistIndex(sound);
		if (playlistIndex == kPlaylistSize) {
			return kUnknownSound;
		}
		return (playlistIndex << 4) | channelNo;
	}

	/**
	 * Finds the hardware channel that is wired to the Sound channel represented
	 * by the given key. Returns `kNumMappedChannels` if the input channel is
	 * not currently wired to a hardware channel.
	 */
	inline uint8 findHwChannelNo(const uint8 key) const {
		uint i;
		for (i = 0; i < kNumMappedChannels; ++i) {
			if (_channelList[i] == key) {
				break;
			}
		}
		return i;
	}

	enum {
		kUnknownChannel = 0xff,
		kControlChannel = 15,

		// TODO: used for rest/delay
		kTimingOverflowFlag = 0x8000,
		kTimingOverflowDelay = 240,
		kTimingOverflowValue = kTimingOverflowFlag | kTimingOverflowDelay
	};

	enum Message {
		kSetLoop = 0x7f,
		kCommandFlag = 0x80,
		kEndOfSysEx = 0xf7,
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
		kSysEx = 0xf0
	};

	enum Controller {
		kModulationController = 1,
		kVolumeController = 7,
		kPanController = 10,
		kDamperPedalController = 64,
		kMaxVoicesController = 75,
		kSingleVoiceNoteController = 78,
		kReverbModeController = 80,
		kLoopEndController = 82,
		kCueController = 96,
		kAllNotesOffController = 123,
		kProgramChangeController = 127
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

	/**
	 * The list of currently active sounds, sorted by decreasing sound priority.
	 */
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
			kUnmapped = 0xff
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
		uint8 numVoices;

		/**
		 * TODO: was ChBed.
		 */
		bool locked;

		MappedChannel() :
			key(kUnmapped),
			priority(0),
			numVoices(0),
			locked(false) {}

		inline uint8 playlistIndex() const { return key >> 4; }
		inline void playlistIndex(const uint8 index) { key = (index << 4) | (key & 0xf); }
		inline uint8 channelNo() const { return key & 0xf; }
		inline void channelNo(const uint8 number) { key = (key & 0xf0) | number; }
	};

	// TODO: Combine these next two things?
	typedef Common::Array<MappedChannel> MappedChannels;
	MappedChannels _mappedChannels;

	typedef Common::Array<Sci1Sound *> MappedNodes;
	MappedNodes _mappedNodes;

	enum {
		kNoVolumeChange = 0xff
	};

	/**
	 * A list of new volume changes per channel.
	 */
	Common::Array<uint8> _newChannelVolumes;

	// TODO: was requestChnl
	uint8 _newChannelVolumesIndex;

	// TODO: was updateChannel1 & 2 (identical)
	void updateChannel(const Sci1Sound &sound, const Sci1Sound::Channel &channel, const uint8 hwChannelNo);

	// TODO: was preemptChn1 & 2 (identical)
	uint8 preemptChannel(const int &numFreeVoices);

	// TODO: was controlChnl
	void parseControlChannel(Sci1Sound &sound, const uint8 trackNo, const Command command);

	// TODO: was NoteOff
	void processNoteOff(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo);
	// TODO: was NoteOn
	void processNoteOn(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo);
	// TODO: was PolyAfterTch
	void processKeyPressure(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo);
	// TODO: was Controller
	void processControllerChange(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool outOfRangeChannel);
	// TODO: was ProgramChange
	void processProgramChange(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool outOfRangeChannel);
	// TODO: was ChnlAfterTch
	void processChannelPressure(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo);
	// TODO: was PitchBend
	void processPitchBend(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool outOfRangeChannel);
	// TODO: was SysEx
	void processSysEx(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo);
	// TODO: was SkipMidi
	void skipCommand(Sci1Sound &sound, const uint8 trackNo, const Command command);

	uint8 insertSoundToPlaylist(Sci1Sound &sound);
	// TODO: was DoEnd
	void removeSoundFromPlaylist(Sci1Sound &sound);
	// TODO: was DoChangeVol
	void processVolumeChange(Sci1Sound &sound, const uint8 volume, const bool enqueue);

	void changeChannelVolume(const Sci1Sound &sound, const uint8 channelNo, const uint8 hwChannelNo, const bool enqueue);

	void fixupHeader(Sci1Sound &sound);

	/**
	 * The number of times the sound server has been suspended.
	 */
	int _numServerSuspensions;

	enum {
		/**
		 * TODO: Better name, this is a hard-coded value from scripts which gets
		 * converted into the actual default reverb mode value.
		 */
		kDefaultReverbMode = 127
	};

	uint8 _defaultReverbMode;

	bool _needsUpdate;
	bool _restoringSound;

#pragma mark -
#pragma mark Kernel
public:
/* TODO:
 	{ SIG_SOUNDSCI1,       0, MAP_CALL(DoSoundMasterVolume),       "(i)",                  NULL },
	{ SIG_SOUNDSCI1,       1, MAP_CALL(DoSoundMute),               "(i)",                  NULL },
	{ SIG_SOUNDSCI1,       2, MAP_EMPTY(DoSoundRestore),           NULL,                   NULL },
	{ SIG_SOUNDSCI1,       3, MAP_CALL(DoSoundGetPolyphony),       "",                     NULL },
	{ SIG_SOUNDSCI1,       4, MAP_CALL(DoSoundGetAudioCapability), "",                     NULL },
	{ SIG_SOUNDSCI1,       5, MAP_CALL(DoSoundSuspend),            "i",                    NULL },
	{ SIG_SOUNDSCI1,       6, MAP_CALL(DoSoundInit),               "o",                    NULL },
	{ SIG_SOUNDSCI1,       7, MAP_CALL(DoSoundDispose),            "o",                    NULL },
	{ SIG_SOUNDSCI1LATE,   8, MAP_CALL(DoSoundPlay),               "oi",                   NULL },
	{ SIG_SCI32,           8, MAP_CALL(DoSoundPlay),               "o",                    kDoSoundPlay_workarounds },
	{ SIG_SOUNDSCI1,       9, MAP_CALL(DoSoundStop),               "o",                    NULL },
	{ SIG_SOUNDSCI1,      10, MAP_CALL(DoSoundPause),              "[o0]i",                NULL },
	{ SIG_SOUNDSCI1,      11, MAP_CALL(DoSoundFade),               "oiiii",                kDoSoundFade_workarounds },
	{ SIG_SOUNDSCI1,      12, MAP_CALL(DoSoundSetHold),            "oi",                   NULL },
	{ SIG_SOUNDSCI1,      13, MAP_EMPTY(DoSoundDummy),             NULL,                   NULL },
	{ SIG_SOUNDSCI1,      14, MAP_CALL(DoSoundSetVolume),          "oi",                   NULL },
	{ SIG_SOUNDSCI1,      15, MAP_CALL(DoSoundSetPriority),        "oi",                   NULL },
	{ SIG_SOUNDSCI1,      16, MAP_CALL(DoSoundSetLoop),            "oi",                   NULL },
	{ SIG_SOUNDSCI1,      17, MAP_CALL(DoSoundUpdateCues),         "o",                    NULL },
	{ SIG_SOUNDSCI1,      18, MAP_CALL(DoSoundSendMidi),           "oiiii",                NULL },
	{ SIG_SOUNDSCI1,      19, MAP_CALL(DoSoundGlobalReverb),       "(i)",                  NULL },
	{ SIG_SOUNDSCI1,      20, MAP_CALL(DoSoundUpdate),             "o",                    NULL },
*/
};

} // End of namespace Sci
#endif
