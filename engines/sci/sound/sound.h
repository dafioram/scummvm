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

#include "audio/audiostream.h"
#include "audio/mixer.h"
#include "common/array.h"
#include "common/list.h"
#include "common/mutex.h"
#include "common/ptr.h"
#include "common/serializer.h"
#include "common/scummsys.h"
#include "sci/engine/vm_types.h"
#include "sci/resource.h"
#include "sci/util.h"

namespace Audio { class Mixer; }

namespace Sci {
class Console;
class GuestAdditions;
class Sci1SoundDriver;
class SegManager;

enum MidiMessage {
	/**
	 * A control channel message which marks the current position as a loop
	 * point.
	 */
	kSetLoop = 0x7f,

	/**
	 * A flag indicating that the current byte starts a new command
	 * sequence.
	 */
	kStartOfMessageFlag = 0x80,

	/**
	 * A standard MIDI message indicating the termination of a System
	 * Exclusive sequence.
	 */
	kEndOfSysEx = 0xf7,

	/**
	 * A message indicating that playback of the track needs to be delayed
	 * for a fixed duration.
	 */
	kFixedRest = 0xf8,

	/**
	 * A message indicating that the end of the track has been reached.
	 */
	kEndOfTrack = 0xfc
};

// TODO: All of these are standard MIDI status messages and could go into
// common code
enum MidiMessageType {
	kNoteOff = 0x80,
	kNoteOn = 0x90,
	kKeyPressure = 0xa0,
	kControllerChange = 0xb0,
	kProgramChange = 0xc0,
	kChannelPressure = 0xd0,
	kPitchBend = 0xe0,
	kSysEx = 0xf0
};

// TODO: Many of these controllers are standard MIDI controllers and could go
// into common code
enum MidiController {
	kModulationController = 1,
	kVolumeController = 7,
	kPanController = 10,
	kDamperPedalController = 64,
	kMaxVoicesController = 75,
	// TODO: This controller also receives the current note when channels
	// are remapped, figure out what this means.
	kMuteController = 78,
	kReverbModeController = 80,
	kHoldPointController = 82,
	kCueController = 96,
	kAllNotesOffController = 123,
	kProgramChangeController = 127
};

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
struct Sci1Sound : public Common::Serializable {
	enum {
		kNumTracks = 16,
		kNumChannels = 15,
		kMaxVolume = 127
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
		/** The sound is not playing. */
		kStopped = 0,

		/** The sound is playing. */
		kPlaying = 1,

		/**
		 * A sound created by `playBed` is playing.
		 *
		 * Sounds played non-exclusively have their instrument channels
		 * dynamically remapped to hardware channels by priority. Sounds played
		 * exclusively have an enforced 1:1 mapping from sound channel to
		 * hardware channel, and may not be preempted by other normal sounds
		 * with higher priorities.
		 */
		kExclusive = 2
	};

	/**
	 * A MIDI data stream.
	 */
	struct Track : public Common::Serializable {
		enum SpecialChannel {
			/** Track is at the end of sound data. */
			kEndOfData = 0xff,

			/** Track contains a digital sample. */
			kSampleTrack = 0xfe
		};

		/**
		 * The offset to the start of track data within the Sound resource.
		 * (This field did not exist in SSCI, which instead wrote a header
		 * directly into the sound data).
		 */
		uint16 offset;

		/**
		 * The size of the track, in bytes. This value was not used in SSCI; we
		 * use it for bounds-checking.
		 */
		uint16 size;

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
		 * The last received MIDI command (message). The upper nibble
		 * corresponds to a `MessageType`, and (for non-SysEx messages) the
		 * lower nibble corresponds to the channel number to which the command
		 * applies.
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
			offset(0),
			position(13),
			rest(0),
			command(0),
			channelNo(kEndOfData),
			loopPosition(3),
			loopRest(0),
			loopCommand(0) {}

		virtual void saveLoadWithSerializer(Common::Serializer &s) override;
	};

	/**
	 * A logical MIDI channel.
	 */
	struct Channel : public Common::Serializable {
		enum {
			kNoCurrentNote = 0xff,
			kUninitialized = 0xff
		};

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
			 */
			kExtra = 2
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

		virtual void saveLoadWithSerializer(Common::Serializer &s) override;
	};

	/**
	 * The sound's data tracks.
	 */
	Common::FixedArray<Track, kNumTracks> tracks;

	/**
	 * The sound's logical MIDI channels. These may be dynamically remapped to
	 * different hardware MIDI channels so are not a 1:1 mapping to outputs.
	 */
	Common::FixedArray<Channel, kNumChannels> channels;

	/**
	 * The nodePtr value returned to the VM to represent this sound object.
	 */
	reg_t nodePtr;

	/**
	 * The Sound's resource.
	 */
	Resource *resource;

	/**
	 * The Sound's resource ID. This is stored separately from the resource
	 * pointer because it is used for save/load.
	 */
	ResourceId id;

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
	 * The reverb mode (0-10), or `kUseDefaultReverb` to allow the reverb mode
	 * to be selected programmatically.
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
	 */
	uint8 numPauses;

	/**
	 * If true, the sound is actually a digital sample. For SCI1.1+ this means
	 * the sound should be played through Audio/Audio32; for SCI1late- this
	 * means the sound should be played through the sample list.
	 *
	 * @note In SSCI1late-, whether or not a sound was sampled was determined
	 * by whether or not `sampleTrackNo` was non-zero. In SSCI1.1 this was
	 * turned into a boolean flag instead. For clarity of implementation, we use
	 * this flag consistently for all SCI versions instead of checking the
	 * sample track number.
	 */
	bool isSample;

	/**
	 * If the sound is a sample, this is the track number for the sample track.
	 * If 0x80 bit is set, the sample has also been loaded. This is used by
	 * SCI1late-.
	 */
	uint8 sampleTrackNo;

	// In SSCI, this structure is zero-filled when it is constructed in InitSnd
	Sci1Sound(const reg_t nodePtr_ = NULL_REG) :
		tracks(),
		channels(),
		nodePtr(nodePtr_),
		resource(nullptr),
		cue(0),
		ticksElapsed(0),
		loopTicksElapsed(0),
		signal(kNoSignal),
		state(kStopped),
		holdPoint(0),
		fixedPriority(false),
		priority(0),
		loop(false),
		volume(0),
		reverbMode(0),
		fadeTargetVolume(0),
		stopSoundOnFade(false),
		fadeDelay(0),
		fadeDelayRemaining(0),
		fadeAmountPerTick(0),
		numPauses(0),
		isSample(false),
		sampleTrackNo(0) {}

	virtual void saveLoadWithSerializer(Common::Serializer &s) override;

	/**
	 * Peeks at a byte of the data stream for the given track.
	 */
	inline byte peek(const uint8 trackNo, const uint8 extra = 0) const {
		const Track &track = tracks[trackNo];
		assert(resource);
		assert(track.position + extra < track.size);
		return resource->getUint8At(track.offset + track.position + extra);
	}

	/**
	 * Consumes the next byte of the data stream for the given track.
	 */
	inline byte consume(const uint8 trackNo) {
		const byte message = peek(trackNo);
		advance(trackNo);
		return message;
	}

	/**
	 * Advances the data stream for the given track by one byte.
	 */
	inline void advance(const uint8 trackNo) {
		Track &track = tracks[trackNo];
		assert(track.position < track.size);
		++tracks[trackNo].position;
	}
};

#pragma mark -
#pragma mark SoundManager

class SoundManager : public Common::Serializable {
public:
	SoundManager(ResourceManager &resMan, SegManager &segMan, GameFeatures &features, GuestAdditions &guestAdditions);
	~SoundManager();

	/**
	 * Returns the preferred resource type for the given sound resource.
	 */
	ResourceType getSoundResourceType(const uint16 resourceNo) const {
		if (_preferSampledSounds &&
			_soundVersion >= SCI_VERSION_1_1 &&
			_resMan.testResource(ResourceId(kResourceTypeAudio, resourceNo))) {

			return kResourceTypeAudio;
		} else {
			return kResourceTypeSound;
		}
	}

	/**
	 * Resets the SoundManager state when the game is restarted.
	 */
	void reset() { _defaultReverbMode = 0; }

	/**
	 * Gets the maximum number of simultaneous voices supported by the current
	 * sound driver.
	 */
	int getNumVoices() const;

	/**
	 * Gets the number of available channels for playing digital audio samples.
	 */
	int getNumDacs() const {
		// TODO: Decide if platforms can benefit from being told of more
		// than one DAC. This would apply only for SCI1early games on PC.
		if (_preferSampledSounds) {
			return 1;
		}

		return 0;
	}

	/**
	 * Used to pause/resume sound when launcher or debugger are open.
	 */
	void systemSuspend(const bool pause);

private:
	/**
	 * Returns the preferred resource ID for the given sound resource.
	 */
	GuiResourceId getSoundResourceId(const uint16 soundNo) const;

	/**
	 * Determines whether or not the current game can only play General MIDI
	 * sounds. Used by Windows-only games.
	 */
	bool gameHasGeneralMidiOnly() const;

	typedef Common::List<Sci1Sound> SoundsList;

	ResourceManager &_resMan;
	SegManager *_segMan;
	GuestAdditions &_guestAdditions;
	Common::Mutex _mutex;

	/**
	 * The enabled state of the driver when engine execution was suspended by
	 * OSystem.
	 */
	bool _driverEnabledState;

	/**
	 * If true, prefer sampled sound effects over synthesised sound effects when
	 * possible.
	 */
	bool _preferSampledSounds;

	/**
	 * If true, behave like Windows 3 (MPC) MIDI.
	 */
	bool _useWindowsMidi;

	/**
	 * The list of currently playing & queued sounds.
	 */
	SoundsList _sounds;

	/**
	 * The currently active version of the sound code.
	 */
	SciVersion _soundVersion;

	/**
	 * The sound driver.
	 */
	Common::ScopedPtr<Sci1SoundDriver> _driver;

#pragma mark -
#pragma mark Save management
public:
	virtual void saveLoadWithSerializer(Common::Serializer &s) override;
	// TODO: was SaveRestoreAfter/RestoreAllSounds
	void reconstructPlaylist();

private:
	void legacyRestore(Common::Serializer &s);
	void serializeSounds(Common::Serializer &s);
	// TODO: was KillAllSounds
	void prepareForRestore();

	/**
	 * Restores the given sound to its previous playback state by
	 * fast-forwarding up to the point given in `ticksElapsed`.
	 * TODO: was RestoreSound
	 */
	void restore(Sci1Sound &sound);

	/**
	 * If true, a sound is in the process of being restored so should not
	 * actually generate audio output by sending notes, etc. to the hardware.
	 */
	bool _restoringSound;

#pragma mark -
#pragma mark MIDI server
// TODO: Figure out how to integrate with SCI0, or split to its own file so the
// SCI0 and SCI1 engines can coexist.
public:
	/**
	 * Enables and disables the sound server. This method uses a counter.
	 */
	void enableSoundServer(const bool enable);

private:
	static inline void soundServerCallback(void *soundManager) {
		static_cast<SoundManager *>(soundManager)->soundServer();
	}

	/**
	 * Advances the state of the sound server. Must be called once per tick in
	 * SCI1 for correct sound processing.
	 */
	void soundServer();

	/**
	 * Processes one fade step for the given sound.
	 */
	void processFade(Sci1Sound &sound);

	/**
	 * The number of times the sound server has been suspended.
	 */
	int _numServerSuspensions;

#pragma mark -
#pragma mark Channel remapping
private:
	enum {
		kNumHardwareChannels = 16
	};

	/**
	 * A physical output channel.
	 */
	struct HardwareChannel {
		enum {
			/**
			 * Special `key` value for an unmapped hardware channel.
			 */
			kUnmapped = 0xff
		};

		/**
		 * The key for the logical channel currently mapped to this hardware
		 * channel.
		 * TODO: was ChNew and ChList; ChNew was used only as a temporary for
		 * recalculating hardware channels, so it is now held on the stack
		 * TODO: Change this to separate the two fields for implementation
		 * clarity.
		 */
		uint8 key;

		/**
		 * A pointer to the sound object currently playing in this channel.
		 * TODO: was ChNodes
		 */
		Sci1Sound *sound;

		/**
		 * The global priority (sound priority * channel priority) of the
		 * logical channel currently mapped to this channel.
		 * TODO: was ChPri
		 */
		uint8 priority;

		/**
		 * The number of voices used by this channel.
		 * TODO: was ChVoice
		 */
		uint8 numVoices;

		/**
		 * If true, this channel may not be preempted to play another channel
		 * with higher priority.
		 * TODO: was ChBed.
		 */
		bool locked;

		HardwareChannel() :
			key(kUnmapped),
			sound(nullptr),
			priority(0),
			numVoices(0),
			locked(false) {}

		inline uint8 playlistIndex() const { return key >> 4; }
		inline void playlistIndex(const uint8 index) { key = (index << 4) | (key & 0xf); }
		inline uint8 channelNo() const { return key & 0xf; }
		inline void channelNo(const uint8 number) { key = (key & 0xf0) | number; }
		inline bool isMapped() const { return key != kUnmapped; }
	};

	typedef Common::FixedArray<HardwareChannel, kNumHardwareChannels> HardwareChannels;

	enum {
		kUnknownSound = 0xff
	};

	/**
	 * Creates a channel key for the given sound and channel in the format used
	 * by `_channelList`. If the given sound is not in the playlist,
	 * `kUnknownSound` is returned.
	 */
	inline uint8 makeChannelKey(const Sci1Sound &sound, const uint8 channelNo) const {
		const uint8 playlistIndex = findPlaylistIndex(sound);
		if (playlistIndex == _playlist.size()) {
			return kUnknownSound;
		}
		return makeChannelKey(playlistIndex, channelNo);
	}

	inline uint8 makeChannelKey(const uint8 playlistIndex, const uint8 channelNo) const {
		return (playlistIndex << 4) | channelNo;
	}

	/**
	 * Finds the hardware channel that is wired to the sound channel represented
	 * by the given key. Returns `HardwareChannel::kUnmapped` if the input
	 * channel is not currently mapped to a hardware channel.
	 */
	inline uint8 findHwChannelNo(const uint8 key) const {
		for (int i = 0; i < kNumHardwareChannels; ++i) {
			if (_hardwareChannels[i].key == key) {
				return i;
			}
		}
		return HardwareChannel::kUnmapped;
	}

	/**
	 * Updates the logical (sound) channel to hardware channel mapping.
	 * TODO: was doChannelList
	 */
	void remapHardwareChannels();

	/**
	 * Creates a new candidate sound-to-hardware channel mapping list.
	 * TODO: was pass 1 of doChannelList
	 */
	HardwareChannels makeChannelMap(const uint8 minChannelNo, const uint8 maxChannelNo) const;

	/**
	 * Tries to map a single input channel into the given new HardwareChannels
	 * list.
	 * @returns true if mapping was successful.
	 */
	bool mapSingleChannel(const uint8 key, const uint8 priority, int &numFreeVoices, const uint8 inChannelNo, const Sci1Sound::Channel &channel, HardwareChannels &newChannels, const uint8 minChannelNo, const uint8 maxChannelNo) const;

	/**
	 * Commits channels in fixed positions from the given new channel list into
	 * the actual hardware channel map.
	 * TODO: was pass 2 of doChannelList
	 */
	 void commitFixedChannels(HardwareChannels &newChannels, const HardwareChannels &oldChannels, const uint8 minChannelNo, const uint8 maxChannelNo);

	/**
	 * Commits all remaining channels from the given new channel list into the
	 * actual hardware channel map.
	 * TODO: was pass 3 of doChannelList
	 */
	void commitDynamicChannels(const HardwareChannels &newChannels, const HardwareChannels &oldChannels, const uint8 minChannelNo, const uint8 maxChannelNo);

	/**
	 * Stops playback of hardware channels which used to contain a channel but
	 * do no longer.
	 * TODO: was pass 4 of doChannelList
	 */
	void stopOldChannels(const HardwareChannels &newChannels, const HardwareChannels &oldChannels);

	/**
	 * Preempts the lowest priority preemptable hardware channel. Adds the newly
	 * freed number of voices to `numFreeVoices` and returns the hardware
	 * channel number which was preempted, or `HardwareChannel::kUnmapped` if no
	 * channel could be preempted.
	 * TODO: was preemptChn1/2
	 */
	uint8 preemptChannel(HardwareChannels &newChannels, int &numFreeVoices) const;

	/**
	 * Sends the state of the channel given in `channel` to the hardware driver.
	 *
	 * TODO: was updateChannel1/2
	 */
	void sendChannelToDriver(const Sci1Sound &sound, const Sci1Sound::Channel &channel, const uint8 hwChannelNo);

	/**
	 * The currently mapped MIDI output channels.
	 */
	HardwareChannels _hardwareChannels;

	/**
	 * If true, the channel mapping needs to be updated the next time the sound
	 * server is called.
	 */
	bool _needsRemap;

#pragma mark -
#pragma mark Effects
public:
	enum {
		kMaxMasterVolume = 15
	};

	// TODO: was MasterVol, one function
	uint8 getMasterVolume() const;
	void setMasterVolume(const uint8 volume);

	// TODO: was SetReverb, one function
	uint8 getReverbMode() const;
	uint8 getDefaultReverbMode() const {
		Common::StackLock lock(_mutex);
		return _defaultReverbMode;
	}
	uint8 setReverbMode(const uint8 reverbMode);

	/**
	 * Returns whether sound reproduction is enabled in the driver.
	 */
	bool isSoundEnabled() const;

	/**
	 * Enables and disables sound reproduction in the driver. This method is
	 * idempotent.
	 */
	void setSoundOn(const bool enable);

private:
	/**
	 * Immediately sets the volume of the given sound.
	 * TODO: was ChangeVolume
	 */
	void setVolume(Sci1Sound &sound, const uint8 volume);

	/**
	 * Fades the volume of the given sound to the given target volume. If a fade
	 * is in progress, it will be replaced with the new fade.
	 *
	 * @param speed The amount of volume change per step.
	 * @param steps The number of steps to perform.
	 * TODO: was FadeSound
	 */
	void fade(Sci1Sound &sound, const int16 targetVolume, const int16 speed, const int16 steps, const bool stopAfterFade);

	/**
	 * Mutes or unmutes the given sound. This method uses a counter.
	 * TODO: was MuteSound
	 */
	void mute(Sci1Sound &sound, const bool mute);

private:
	enum {
		/** Use whatever reverb mode is given in `_defaultReverbMode`. */
		kUseDefaultReverb = 127,

		/** No pending volume change. */
		kNoVolumeChange = 0xff
	};

	/**
	 * Updates the volume for the given sound. If `enqueue` is true, volume
	 * changes to the sound's corresponding hardware channels will be queued and
	 * processed over time by the sound server instead of being applied
	 * immediately.
	 * TODO: was DoChangeVol
	 */
	void processVolumeChange(Sci1Sound &sound, const uint8 volume, const bool enqueue);
	void changeChannelVolume(const Sci1Sound &sound, const uint8 channelNo, const uint8 hwChannelNo, const bool enqueue);

	/**
	 * Sends pending volume changes to the sound driver. This will send at
	 * most two volume changes per call.
	 */
	void applyPendingVolumeChanges();

	/**
	 * A list of pending volume changes, where the array indexes are indexes
	 * into `_hardwareChannels` and values are the new volumes for those
	 * channels.
	 */
	Common::FixedArray<uint8, kNumHardwareChannels> _newChannelVolumes;

	/**
	 * The next hardware channel which should be checked for a pending volume
	 * update.
	 * TODO: was requestChnl
	 */
	uint8 _nextVolumeChangeChannel;

	/**
	 * The actual reverb mode to send to hardware when a song is played with
	 * `kUseDefaultReverb` as its reverb mode.
	 */
	uint8 _defaultReverbMode;

#pragma mark -
#pragma mark Playback management
public:
	/**
	 * The SMPTE timecode of a sound playing back at 30fps.
	 */
	struct Position {
		uint16 minutes;
		uint8 seconds;
		uint8 frames;
	};

	/**
	 * Pauses or unpauses all sounds. This function uses the same counter as
	 * single-sound pauses so will also unpause individually paused sounds.
	 */
	void pauseAll(const bool pause);

private:
	/**
	 * Starts playback of the given sound. If the sound already exists in the
	 * playlist, it will be restarted.
	 *
	 * @param exclusive If true, the sound will be played back in exclusive mode
	 * (calls to `playBed` in game scripts are supposed to do this).
	 *
	 * @returns the new index of the sound in the playlist, or
	 * `_playlist.size()` if the sound was not added to the playlist.
	 *
	 * TODO: was PlaySound. returns playlist index
	 */
	uint8 play(Sci1Sound &sound, const bool exclusive);

	/**
	 * Pauses or unpauses a single sound. This method sets the number of pauses
	 * directly and is used by SCI1early-.
	 */
	void pause(Sci1Sound &sound, const uint8 numPauses);

	/**
	 * Pauses or unpauses a single sound. This method uses a counter and is used
	 * by SCI1mid+.
	 * TODO: was PauseSound, one function
	 */
	void pause(Sci1Sound &sound, const bool pause);

	/**
	 * Stops playback of the given sound.
	 * TODO: was EndSound
	 */
	void stop(Sci1Sound &sound);

	/**
	 * Sets the hold point for the given sound.
	 * TODO: was HoldSound
	 */
	void hold(Sci1Sound &sound, const uint8 holdPoint);

	/**
	 * Gets the signal for the given sound without resetting the signal.
	 * TODO: was GetSignal
	 */
	Sci1Sound::Signal peekSignal(const Sci1Sound &sound);

	/**
	 * Gets and resets the signal for the given sound.
	 * TODO: was GetSignalRset
	 */
	Sci1Sound::Signal consumeSignal(Sci1Sound &sound);

	/**
	 * Gets the last data increment cue value for the given sound.
	 * TODO: was GetDataInc
	 */
	uint16 getCue(const Sci1Sound &sound);

	/**
	 * Returns the current position of sound playback in SMPTE timecode format.
	 * TODO: was GetSMPTE
	 */
	Position getPosition(const Sci1Sound &sound);

	/**
	 * Sets the priority of the given sound and updates the playlist order to
	 * match.
	 * TODO: was ChangePri
	 */
	void setPriority(Sci1Sound &sound, const uint8 priority);

#pragma mark -
#pragma mark Sound generation
private:
	/**
	 * Programmatically stops playback of a note on the given sound channel.
	 * TODO: was SendNoteOff
	 */
	void setNoteOff(Sci1Sound &sound, const uint8 channelNo, const uint8 note, const uint8 velocity);

	/**
	 * Programmatically starts playback of a note on the given sound channel.
	 * TODO: was SendNoteOn
	 */
	void setNoteOn(Sci1Sound &sound, const uint8 channelNo, const uint8 note, const uint8 velocity);

	/**
	 * Programmatically sends a new controller value to the given sound channel.
	 * TODO: was SendContrlr
	 */
	void setController(Sci1Sound &sound, const uint8 channelNo, const uint8 controllerNo, const uint8 value);

	/**
	 * Programmatically changes the program used for the given sound channel.
	 * TODO: was SendPChange
	 */
	void setProgram(Sci1Sound &sound, const uint8 channelNo, const uint8 programNo);

	/**
	 * Programmatically changes the pitch bend for the given sound channel.
	 * TODO: was SendPitchBend
	 */
	void setPitchBend(Sci1Sound &sound, const uint8 channelNo, const uint16 value);

#pragma mark -
#pragma mark Data processing
private:
	enum {
		/**
		 * The number of the channel in the Sound which contains control data
		 * instead of instrument data.
		 */
		kControlChannel = 15,

		/**
		 * A special flag used to indicate that the current rest is a fixed
		 * duration rest.
		 */
		kFixedRestFlag = 0x8000,

		/**
		 * The time, in ticks, that a fixed rest should rest.
		 */
		kFixedRestAmount = 240,

		/**
		 * The rest value used for new fixed rests in `Sci1Sound::rest`.
		 */
		kFixedRestValue = kFixedRestFlag | kFixedRestAmount
	};

	/**
	 * Finds the offset table for the given device ID. Returns a null span no
	 * offset table exists for the given device ID.
	 */
	SciSpan<const byte> findTrackOffsets(SciSpan<const byte> data, const uint8 deviceId) const;

	/**
	 * Finds and populates the offsets to the start of track data for each track
	 * in the sound designed for the current output device. This is a highly
	 * modified version of the header fixup method from SSCI, changed so as to
	 * not overwrite parts of the original resource data in memory.
	 */
	void readTrackOffsets(Sci1Sound &sound);

	/**
	 * Processes the next message in all active tracks of the given sound.
	 * TODO: was parseNode
	 */
	void advancePlayback(Sci1Sound &sound, const uint8 playlistIndex);

	/**
	 * Processes the next sequence of non-control channel messages.
	 */
	void parseCommand(Sci1Sound &sound, const uint8 playlistIndex, const uint8 trackNo, Sci1Sound::Track &track);

	/**
	 * Processes the next control channel message.
	 * TODO: was controlChnl
	 */
	void parseControlChannel(Sci1Sound &sound, const uint8 trackNo, const MidiMessageType command);

	/**
	 * Processes a Note Off message from a data track.
	 * TODO: was NoteOff
	 */
	void processNoteOff(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo);

	/**
	 * Processes a Note On message from a data track.
	 * TODO: was NoteOn
	 */
	void processNoteOn(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo);

	/**
	 * Processes a Key Pressure (aftertouch) message from a data track.
	 * TODO: was PolyAfterTch
	 */
	void processKeyPressure(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo);

	/**
	 * Processes a Controller Change message from a data track.
	 * TODO: was Controller
	 */
	void processControllerChange(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool isExtraChannel);

	/**
	 * Processes a Program Change message from a data track.
	 * TODO: was ProgramChange
	 */
	void processProgramChange(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool isExtraChannel);

	/**
	 * Processes a Channel Pressure (aftertouch) message from a data track.
	 * TODO: was ChnlAfterTch
	 */
	void processChannelPressure(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo);

	/**
	 * Processes a Pitch Bend message from a data track.
	 * TODO: was PitchBend
	 */
	void processPitchBend(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool isExtraChannel);

	/**
	 * Processes a System Exclusive message from a data track.
	 * TODO: was SysEx
	 */
	void processSysEx(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo);

	/**
	 * Skips past data for the given command in a data track.
	 * TODO: was SkipMidi
	 */
	void skipCommand(Sci1Sound &sound, const uint8 trackNo, const MidiMessageType command);

#pragma mark -
#pragma mark Playlist management
private:
	enum {
		kPlaylistSize = 16,
		kSampleListSize = 16
	};

	/**
	 * The list of currently active sounds, sorted by decreasing sound priority.
	 */
	Common::FixedArray<Sci1Sound *, kPlaylistSize> _playlist;

	/**
	 * Finds the index of the given sound in the current playlist. Returns
	 * `_playlist.size()` if the sound could not be found in the playlist.
	 */
	inline uint8 findPlaylistIndex(const Sci1Sound &sound) const {
		uint i;
		for (i = 0; i < _playlist.size(); ++i) {
			if (_playlist[i] == &sound) {
				break;
			}
		}
		return i;
	}

	/**
	 * Inserts the given sound into _playlist. The sound must not already exist
	 * in the playlist.
	 */
	uint8 insertSoundToPlaylist(Sci1Sound &sound);

	/**
	 * Removes the given sound from _playlist.
	 * @returns true if the sound was found and removed.
	 * TODO: was DoEnd
	 */
	bool removeSoundFromPlaylist(Sci1Sound &sound);

#pragma mark -
#pragma mark Digital sample playback
private:
	enum { kSampleLoadedFlag = 0x80 };

	// In SSCI, sample playback was handled by individual drivers; we instead
	// always play back digital audio tracks within the player itself, for
	// simplicity and to allow combinations of digital + MIDI which were not
	// always possible in the original engine.
	class SamplePlayer : public Audio::AudioStream {
	public:
		enum Status {
			/** The sample is finished. */
			kFinished = 0,
			/** The sample has looped. */
			kLooped = 1,
			/** The sample is playing. */
			kPlaying = 2
		};

		SamplePlayer(SoundManager &manager, Audio::Mixer &mixer);
		~SamplePlayer();

		// In SSCI this received pointer to sample data, volume, and loop
		void load(const Sci1Sound &sound);

		// In SSCI this received pointer to sample data and used AL and AH to
		// communicate status
		Status advance(const Sci1Sound &sound);

		// In SSCI this received pointer to sample data
		void unload();

		virtual int readBuffer(int16 *buffer, int numSamples) override;

		virtual bool isStereo() const override { return false; }

		virtual int getRate() const override { return _sampleRate; }

		virtual bool endOfData() const override { return false; }

	private:
		SoundManager &_manager;
		Audio::Mixer &_mixer;
		Audio::SoundHandle _handle;
		bool _loop;
		bool _playing;
		uint16 _pos;
		uint16 _sampleRate;
		uint16 _size;
		uint16 _loopStart;
		uint16 _loopEnd;
		SciSpan<const byte> _data;
	};

	/**
	 * Validates that a sample is playing for the given sound, and removes the
	 * sound from the playlist if it is not.
	 * TODO: was SampleActive
	 */
	void validateSample(Sci1Sound &sound);

	/**
	 * Advances playback of all samples in the sample list.
	 * TODO: was DoSamples
	 */
	void advanceSamplePlayback();

	SamplePlayer _samplePlayer;

	/**
	 * The list of sampled sounds which are currently playing.
	 */
	Common::FixedArray<Sci1Sound *, kSampleListSize> _sampleList;

#pragma mark -
#pragma mark Kernel
public:
	void kernelInit(const reg_t soundObj);
	void kernelDispose(const reg_t soundObj);
	void kernelPlay(const reg_t soundObj, const bool exclusive);
	void kernelStop(const reg_t soundObj);
	void kernelPause(const reg_t soundObj, const int16 numPauses, const bool pauseDac);
	void kernelFade(const reg_t soundObj, const int16 targetVolume, const int16 speed, const int16 steps, const bool stopAfterFade);
	void kernelHold(const reg_t soundObj, const int16 holdPoint);
	void kernelSetVolume(const reg_t soundObj, const int16 volume);
	void kernelSetPriority(const reg_t soundObj, const int16 priority);
	void kernelSetLoop(const reg_t soundObj, const bool enable);
	void kernelUpdateCues(const reg_t soundObj);
	void kernelSendMidi(const int argc, const reg_t *const argv);
	void kernelUpdate(const reg_t soundObj);

private:
	struct Kernel {
		enum Signal {
			kNoSignal = 0,
			kFinished = -1
		};
	};

	class SoundByRegT {
		reg_t _key;
	public:
		inline SoundByRegT(const reg_t key) : _key(key) {}
		inline bool operator()(const Sci1Sound &sound) const {
			return sound.nodePtr == _key;
		}
	};

	inline const Sci1Sound *findSoundByRegT(const reg_t key) const {
		SoundsList::const_iterator it = Common::find_if(_sounds.begin(), _sounds.end(), SoundByRegT(key));
		if (it == _sounds.end()) {
			return nullptr;
		}
		return &*it;
	}

	inline Sci1Sound *findSoundByRegT(const reg_t key) {
		SoundsList::iterator it = Common::find_if(_sounds.begin(), _sounds.end(), SoundByRegT(key));
		if (it == _sounds.end()) {
			return nullptr;
		}
		return &*it;
	}

	inline SoundsList::iterator findSoundIteratorByRegT(const reg_t key) {
		SoundsList::iterator it = Common::find_if(_sounds.begin(), _sounds.end(), SoundByRegT(key));
		return it;
	}

	void sendMidi(const reg_t soundObj, const int16 channel, const int16 command, const int16 a, const int16 b);

	/**
	 * A serial used for a sound object nodePtr in the case that two sounds are
	 * playing simultaneously while originating from the same VM Sound object.
	 */
	uint16 _nextObjectId;

#pragma mark -
#pragma mark Debugging
public:
	void debugPrintPlaylist(Console &con) const;
	void debugPrintSound(Console &con, const uint index) const;
	void debugPrintSound(Console &con, const Sci1Sound &sound) const;
	void debugPrintChannelMap(Console &con) const;
	void debugPrintDriverState(Console &con) const;
	void debugStopAll();

private:
	void debugPrintChannelMap(Console &con, const HardwareChannels &channels) const;
};

} // End of namespace Sci
#endif
