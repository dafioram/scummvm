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
#include "common/serializer.h"
#include "sci/engine/vm_types.h"
#include "sci/util.h"

namespace Sci {

/**
 * The playback strategy for a SCI0 sound.
 */
enum Sci0PlayStrategy {
	/**
	 * The caller should synchronously invoke `update` on the driver until
	 * `signal` is -1.
	 */
	kSync = 0,

	/**
	 * The caller should invoke `update` once on the driver and then return.
	 */
	kAsync = 1,

	/**
	 * Playback did not begin; the caller should clean up resource data
	 * immediately.
	 */
	kAbort = 3
};

struct Sci0Sound {
	// In SSCI, the first field is a Node

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

	int16 priority;

	/**
	 * The playback strategy for the sound.
	 */
	Sci0PlayStrategy strategy;

	uint16 chain; // TODO: Not a clue what this is

	int16 state;

	uint16 signal;

	/**
	 * The playback volume of the sound, from 0 to 15.
	 */
	int16 volume;

	/**
	 *
	 */
	int16 effect;
};

class SoundDriver {
public:
	virtual ~SoundDriver() {}
	virtual int getPatchNumber() const = 0;
	virtual int getNumVoices() const = 0;
	virtual int getNumDacs() const = 0;
}

class Sci0SoundDriver : public SoundDriver {
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
	 * Begins playback of the sound in the given object.
	 */
	virtual Sci0PlayStrategy play(Sci0Sound &) = 0;

	/**
	 * Advances sound playback. The given object will be updated with
	 * information about the new playback state of the driver, if it was time
	 * to advance playback.
	 */
	virtual void update(Sci0Sound &) = 0;

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

class Sci1SoundDriver : public SoundDriver {
	SoundDriver(SciSpan<const byte> &patchData);

	// ops:
	// 0 - get device info (getPatchNumber/getNumVoices/getNumDacs)
	// 1 - init device (constructor)
	// 2 - shutdown device (destructor)
	// 3 - enable/disable sound processing
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
	// 16 - check sample
	// 17 - unused

	// was Process in SSCI
	void setProcessSounds(const bool enable);

	void noteOff(const int8 channelIndex, const int8 note, const int8 velocity);

	void noteOn(const int8 channelIndex, const int8 note, const int8 velocity);

	// was PolyAftertouch in SSCI
	void keyPressure(const int8 channelIndex, const int8 note, const int8 pressure);

	void controllerChange(const int8 channelIndex, const int8 controllerNo, const int8 value);

	void programChange(const int8 channelIndex, const int8 programNo);

	// was ChannelAftertouch in SSCI
	void channelPressure(const int8 channelIndex, const int8 pressure);

	void pitchBend(const int8 channelIndex, const int16 pitch);

	// was SetReverb in SSCI
	int8 getReverbMode();
	int8 getDefaultReverbMode();
	int8 setReverbMode(const int8 mode);

	// was MasterVol in SSCI
	int8 getMasterVolume();
	int8 setMasterVolume(const int8 volume);

	// was SoundOn in SSCI
	bool getEnabled();
	bool setEnabled(const bool enable);

	// TODO: Figure out which platform drivers actually use these calls
	// (probably Amiga & Mac) and then get the correct signatures and implement
	// them
//	TODO playSample();
//	TODO stopSample();
//	TODO checkSample();
};

class SoundManager : public Common::Serializable {
public:
	SoundManager();

	virtual void saveLoadWithSerializer(Common::Serializer &s) override;

private:
	Common::Array<reg_t> _sounds;
};

} // End of namespace Sci
#endif
