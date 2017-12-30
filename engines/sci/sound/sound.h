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
class Console;
class Sci1SoundDriver;
class SegManager;

// TODO: All this stuff except for the kSetLoop is just standard MIDI messaging
// stuff and should go into common code
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
	kTimingOverflow = 0xf8,

	/**
	 * A message indicating that the end of the track has been reached.
	 */
	kEndOfTrack = 0xfc
};

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
	kLoopEndController = 82,
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
struct Sci1Sound {
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
		 * The offset to the start of track data within the Sound resource.
		 * (This field did not exist in SSCI, which instead wrote a header
		 * directly into the sound data).
		 */
		uint16 offset;

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
	} tracks[kNumTracks];

	/**
	 * A logical MIDI channel.
	 */
	struct Channel {
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
			 * TODO: Rename
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
	 * The nodePtr value returned to the VM to represent this sound object.
	 */
	reg_t nodePtr;

	/**
	 * The Sound's resource.
	 */
	Resource *resource;

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

	// In SSCI, this structure is zero-filled when it is constructed in InitSnd
	Sci1Sound(const reg_t nodePtr_) :
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
		paused(0),
		isSample(false) {}

	/**
	 * Peeks at a byte of the data stream for the given track.
	 */
	inline byte peek(const uint8 trackNo, const uint8 extra = 0) {
		return resource->getUint8At(tracks[trackNo].offset + tracks[trackNo].position + extra);
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

#pragma mark -
#pragma mark SoundManager

class SoundManager : public Common::Serializable {
public:
	SoundManager(ResourceManager &resMan, SegManager &segMan, GameFeatures &features);
	~SoundManager();

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

	/**
	 * Gets the maximum number of simultaneous voices supported by the current
	 * sound driver.
	 */
	int getNumVoices() const;

private:
	/**
	 * Returns the preferred resource ID for the given sound resource.
	 */
	GuiResourceId getSoundResourceId(const uint16 soundNo) const;

	typedef Common::Array<Sci1Sound> SoundsList;

	ResourceManager &_resMan;
	SegManager *_segMan;
	Common::Mutex _mutex;

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
#pragma mark Debugging
public:
	void debugPrintPlaylist(Console &con) const;
	void debugPrintSound(Console &con, const reg_t nodePtr) const;
	void debugPlay(const GuiResourceId soundId);
	void debugStopAll();

#pragma mark -
#pragma mark Save management
public:
	virtual void saveLoadWithSerializer(Common::Serializer &s) override;

private:
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

	/**
	 * If true, the channel mapping needs to be updated the next time the sound
	 * server is called.
	 */
	bool _needsUpdate;

#pragma mark -
#pragma mark Effects
public:
	enum {
		kMaxMasterVolume = 15
	};

	// TODO: was MasterVol, one function
	uint8 getMasterVolume() const;
	uint8 setMasterVolume(const uint8 volume);

	// TODO: was SetReverb, one function
	uint8 getReverbMode() const;
	uint8 getDefaultReverbMode() const { return _defaultReverbMode; }
	uint8 setReverbMode(const uint8 reverbMode);

	/**
	 * Enables and disables sound reproduction in the driver. This method is
	 * idempotent.
	 */
	void setSoundOn(const bool enable);

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
	Common::Array<uint8> _newChannelVolumes;

	/**
	 * The next hardware channel which should be checked for a pending volume
	 * update.
	 * TODO: was requestChnl
	 */
	uint8 _nextVolumeChangeChannel;

	/**
	 * The actual reverb mode to send to hardware when a song is played with
	 * `kDefaultReverbMode` as its reverb mode.
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
	 * Starts playback of the given sound. If the sound already exists in the
	 * playlist, it will be restarted.
	 *
	 * @param exclusive If true, the sound will be played back in exclusive mode
	 * (calls to `playBed` in game scripts are supposed to do this).
	 *
	 * @returns the new index of the sound in the playlist, or `kPlaylistSize`
	 * if the sound was not added to the playlist.
	 *
	 * TODO: was PlaySound. returns playlist index
	 */
	uint8 play(Sci1Sound &sound, const bool exclusive);

	/**
	 * Pauses or unpauses a single sound. This method uses a counter.
	 * TODO: was PauseSound, one function
	 */
	void pause(Sci1Sound &sound, const bool pause);

	/**
	 * Pauses or unpauses all sounds. This function uses the same counter as
	 * single-sound pauses so will also unpause individually paused sounds.
	 */
	void pauseAll(const bool pause);

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
public:
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
	// TODO: Split this enum up more
	enum {
		/**
		 * The number of the channel in the Sound which contains control data
		 * instead of instrument data.
		 */
		kControlChannel = 15,

		// TODO: used for rest/delay
		kTimingOverflowFlag = 0x8000,
		kTimingOverflowDelay = 240,
		kTimingOverflowValue = kTimingOverflowFlag | kTimingOverflowDelay
	};

	/**
	 * Finds and populates the offsets to the start of track data for each track
	 * in the sound designed for the current output device. This is a highly
	 * modified version of the header fixup method from SSCI, changed so as to
	 * not overwrite parts of the original resource data in memory.
	 */
	void findTrackOffsets(Sci1Sound &sound);

	/**
	 * Processes the next message in all active tracks of the given sound.
	 * TODO: was parseNode
	 */
	void parseNextNode(Sci1Sound &sound, const uint8 playlistIndex);

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
		kPlaylistSize = 16
	};

	// TODO: was sampleList, appears totally broken in SCI1.1+ so unclear what
	// to do about it. In SSCI this is a 16-dword array, but the code uses it
	// to hold a pointer in _samples[0], or an offset in the first word of
	// _samples[0] and segment segment the first word of _samples[2]. The
	// pointer is to a Sci1Sound object. In earlier versions this was probably a
	// list corresponding to _playlist which stored pointers to the sample
	// tracks for sounds which had digital sample tracks; need to verify that.
	Sci1Sound *_sample;

	/**
	 * The list of currently active sounds, sorted by decreasing sound priority.
	 */
	Sci1Sound *_playlist[kPlaylistSize];

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

	/**
	 * Inserts the given sound into _playlist. The sound must not already exist
	 * in the playlist.
	 */
	uint8 insertSoundToPlaylist(Sci1Sound &sound);

	/**
	 * Removes the given sound from _playlist.
	 * TODO: was DoEnd
	 */
	void removeSoundFromPlaylist(Sci1Sound &sound);

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
		 * The key (playlist index << 4 | channel number) for the logical
		 * channel currently mapped to this output channel.
		 * TODO: was ChNew
		 * TODO: Change this to separate the two fields for implementation
		 * clarity.
		 */
		uint8 key;

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
			priority(0),
			numVoices(0),
			locked(false) {}

		inline uint8 playlistIndex() const { return key >> 4; }
		inline void playlistIndex(const uint8 index) { key = (index << 4) | (key & 0xf); }
		inline uint8 channelNo() const { return key & 0xf; }
		inline void channelNo(const uint8 number) { key = (key & 0xf0) | number; }
	};

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
		if (playlistIndex == kPlaylistSize) {
			return kUnknownSound;
		}
		return (playlistIndex << 4) | channelNo;
	}

	/**
	 * Finds the hardware channel that is wired to the sound channel represented
	 * by the given key. Returns `kNumHardwareChannels` if the input channel is
	 * not currently wired to a hardware channel.
	 */
	inline uint8 findHwChannelNo(const uint8 key) const {
		uint i;
		for (i = 0; i < kNumHardwareChannels; ++i) {
			if (_channelList[i] == key) {
				break;
			}
		}
		return i;
	}

	/**
	 * Updates the output channel mapping.
	 * TODO: was doChannelList
	 */
	void updateChannelList();

	/**
	 * Preempts the lowest priority preemptable hardware channel. Adds the newly
	 * freed number of voices to `numFreeVoices` and returns the hardware
	 * channel number which was preempted, or `kNumHardwareChannels` if no
	 * channel could be preempted.
	 * TODO: was preemptChn1/2
	 */
	uint8 preemptChannel(const int &numFreeVoices);

	/**
	 * Sends the state of the channel given in `channel` to the hardware driver.
	 *
	 * TODO: was updateChannel1/2
	 */
	void sendChannelToDriver(const Sci1Sound &sound, const Sci1Sound::Channel &channel, const uint8 hwChannelNo);

	// TODO: Combine these next two things? Combine also with _channelList?
	// The reason why they are not combined already is because only the fields
	// that updateChannelList groups together were turned into the
	// HardwareChannel struct.
	typedef Common::Array<HardwareChannel> HardwareChannels;
	HardwareChannels _hardwareChannels;

	typedef Common::Array<Sci1Sound *> MappedNodes;
	/**
	 * A map from hardware channel number to pointer to the sound playing in the
	 * given channel.
	 */
	MappedNodes _mappedNodes;

	// TODO: Turn this into a struct. key (ChList), node (ChNodes)
	typedef Common::Array<uint8> ChannelList;

	/**
	 * A map from hardware channel number to sound key as used in
	 * `HardwareChannel::key`.
	 * TODO: was chList
	 */
	ChannelList _channelList;

#pragma mark -
#pragma mark Kernel
public:
	void kernelInit(const reg_t soundObj);
	void kernelDispose(const reg_t soundObj);
	void kernelPlay(const reg_t soundObj, const bool exclusive);
	void kernelStop(const reg_t soundObj);
	void kernelPause(const reg_t soundObj, const bool shouldPause, const bool pauseDac);
	void kernelFade(const reg_t soundObj, const int16 targetVolume, const int16 speed, const int16 steps, const bool stopAfterFade);
	void kernelHold(const reg_t soundObj, const int16 holdPoint);
	void kernelSetVolume(const reg_t soundObj, const int16 volume);
	void kernelSetPriority(const reg_t soundObj, const int16 priority);
	void kernelSetLoop(const reg_t soundObj, const bool enable);
	void kernelUpdateCues(const reg_t soundObj);
	void kernelSendMidi(const reg_t soundObj, const int16 channel, const int16 command, const int16 a, const int16 b);
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
};

} // End of namespace Sci
#endif
