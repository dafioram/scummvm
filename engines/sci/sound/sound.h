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
#include "common/ptr.h"
#include "common/serializer.h"
#include "common/scummsys.h"
#include "sci/engine/vm_types.h"
#include "sci/resource.h"
#include "sci/util.h"

namespace Audio { class Mixer; }

namespace Sci {

enum Sci0PlayStrategy {
	/**
	 * The caller should synchronously invoke `service` on the driver until
	 * `signal` is -1.
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
struct SciSound {
	enum {
		kNumTracks = 16,
		kNumChannels = 15
	};

	enum ChannelFlags {
		/** TODO: Channel cannot be preempted? */
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

	struct {
		/** The current playback position, in bytes. */
		uint32 position;

		/**
		 * TODO: Something to do with delaying for N ticks to avoid polyphony
		 * overflow?
		 */
		uint32 rest;

		/** TODO: The currently active operation?? for the track? */
		int8 command;

		/** TODO: The currently active channel for the track? */
		int8 channelNo;

		/** The position of the start of the current sound loop, in bytes. */
		uint32 loopPosition;

		/** The state of `rest` at the loop point. */
		uint32 loopRest;

		/** The state of `command` at the loop point. */
		int8 loopCommand;
	} tracks[kNumTracks];

	struct {
		/** MIDI pitch bend value, L 0 C 16384 H 32639. */
		uint32 pitchBend;

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
	 * The priority of this sound. Sounds with higher values are placed in the
	 * playback queue ahead of other sounds and will preempt playback of sounds
	 * with lower priorities.
	 */
	uint8 priority;

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
};

/**
 * Common Sound driver interface for all SCI revisions.
 */
class SoundDriver {
public:
	SoundDriver(Resource *patch) : _patch(patch) {}

	virtual ~SoundDriver() {}

	/**
	 * Returns the resource number for the patch resource for this driver.
	 */
	virtual int getPatchNumber() const = 0;

	/**
	 * Returns the number of simultaneous voices this driver supports.
	 */
	virtual int getNumVoices() const = 0;

	/**
	 * Returns the number of digital audio converters this driver supports.
	 */
	virtual int getNumDacs() const = 0;

protected:
	/**
	 * An optional Patch resource for the driver.
	 */
	Resource *_patch;
};

/**
 * Sound driver interface for SCI0.
 */
class Sci0SoundDriver : public SoundDriver {
public:
	// ops (offsets):
	// 0 - get device info (getPatchNumber/getNumVoices/getNumDacs)
	// 2 - init device (constructor)
	// 4 - shutdown device (destructor)
	// 6 - start sound
	// 8 - advance playback
	// 10 - set volume
	// 12 - fade sound
	// 14 - stop sound
	// 16 - pause sound
	// 18 - restore sound

	/**
	 * Advances sound playback. The given object will be updated with
	 * information about the new playback state of the driver, if it was time
	 * to advance playback. This should be called at least once per tick.
	 */
	virtual void service(Sci0Sound &) = 0;

	/**
	 * Begins playback of the sound in the given object.
	 */
	virtual Sci0PlayStrategy play(Sci0Sound &) = 0;

	/**
	 * Fade out the sound using the volume in the given Sci0Sound object.
	 */
	virtual void fade(Sci0Sound &) = 0;

	/**
	 * Stops playback.
	 */
	virtual void stop(Sci0Sound &) = 0;

	/**
	 * Pauses playback.
	 */
	virtual void pause(Sci0Sound &) = 0;

	/**
	 * Restores the driver's playback state to match the given object.
	 */
	virtual void restore(Sci0Sound &) = 0;

	/**
	 * Updates the master volume of the driver from the volume in the given
	 * object.
	 */
	virtual void setMasterVolume(Sci0Sound &) = 0;
};

/**
 * Sound driver interface for SCI1+.
 */
class Sci1SoundDriver : public SoundDriver {
public:
	// ops:
	// 0 - get device info (getPatchNumber/getNumVoices/getNumDacs)
	// 1 - init device (constructor)
	// 2 - shutdown device (destructor)
	// 3 - service driver
	// 4 - note off
	// 5 - note on
	// 6 - poly aftertouch/pressure
	// 7 - controller change
	// 8 - program/patch change
	// 9 - channel aftertouch/pressure
	// 10 - pitch bend
	// 11 - get/set reverb
	// 12 - get/set global volume
	// 13 - get/set sound on/off
	// 14 - play sample
	// 15 - stop sample
	// 16 - TODO get sample position or sample state or seek sample?
	// 17 - unused

	/**
	 * Services the driver. This should be called once per tick.
	 */
	virtual void service() = 0;

	/**
	 * Programmatically starts playback of a note. For MIDI devices, this is
	 * equivalent to a MIDI Note On message.
	 */
	virtual void noteOn(const int8 channelIndex, const int8 note, const int8 velocity) = 0;

	/**
	 * Programmatically stops playback of a note. For MIDI devices, this is
	 * equivalent to a MIDI Note Off message.
	 */
	virtual void noteOff(const int8 channelIndex, const int8 note, const int8 velocity) = 0;

	/**
	 * Sets the value of a MIDI controller to the given value. For MIDI devices,
	 * this is equivalent to a MIDI Control Change message.
	 */
	virtual void controllerChange(const int8 channelIndex, const int8 controllerNo, const int8 value) = 0;

	/**
	 * Programmatically sets the program (patch/instrument) for the given
	 * channel to the given program number. For MIDI devices, this is equivalent
	 * to a MIDI Program Change message.
	 */
	virtual void programChange(const int8 channelIndex, const int8 programNo) = 0;

	/**
	 * Programmatically sets the pitch bend for all notes in a channel. For MIDI
	 * devices, this is equivalent to a MIDI Pitch Bend Change message.
	 */
	virtual void pitchBend(const int8 channelIndex, const int16 pitch) = 0;

	/**
	 * Sets the aftertouch key pressure for a single note. For MIDI devices,
	 * this is equivalent to a MIDI Polyphonic Key Pressure message.
	 * TODO: was polyAfterTch
	 */
	virtual void keyPressure(const int8 channelIndex, const int8 note, const int8 pressure) = 0;

	/**
	 * Programmatically sets the aftertouch pressure for all notes in a channel.
	 * For MIDI devices, this is equivalent to a MIDI Channel Pressure message.
	 * TODO: was chnlAfterTch
	 */
	virtual void channelPressure(const int8 channelIndex, const int8 pressure) = 0;

	// In SSCI, these two functions were one SetReverb function, where sending
	// mode 0xFF would return the current reverb mode without changing anything.

	/**
	 * Gets the currently active reverb mode for the driver.
	 */
	virtual uint8 getReverbMode() = 0;

	/**
	 * Sets a new reverb mode for the driver and returns the previous mode.
	 */
	virtual uint8 setReverbMode(const uint8 mode) = 0;

	// In SSCI, these two functions were one MasterVol function, where sending
	// volume 0xFF would return the current volume without changing anything.

	/**
	 * Gets the master volume (0-15).
	 */
	virtual int8 getMasterVolume() = 0;

	/**
	 * Sets the master volume and returns the previous volume.
	 */
	virtual int8 setMasterVolume(const int8 volume) = 0;

	// In SSCI, these two functions were one SoundOn function, where sending
	// 0xFF would return the current state without changing anything.

	/**
	 * Gets whether or not sound playback is enabled.
	 */
	virtual bool isEnabled() = 0;

	/**
	 * Enables or disables sound playback. The previous state is returned.
	 */
	virtual bool enable(const bool enable) = 0;

	// TODO: There are three additional APIs for digital sample playback which
	// are used by at least Amiga/Mac.
};

class SoundManager : public Common::Serializable {
public:
	SoundManager(ResourceManager &resMan, GameFeatures &features);

	virtual void saveLoadWithSerializer(Common::Serializer &s) override;

private:
	Audio::Mixer &_mixer;
	ResourceManager &_resMan;
	Common::Array<SciSound> _sounds;
	Common::ScopedPtr<SoundDriver> _driver;
	SciVersion _soundVersion;

	int8 _defaultReverb;
};

} // End of namespace Sci
#endif
