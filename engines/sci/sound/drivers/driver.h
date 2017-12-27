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

#ifndef SCI_SOUND_DRIVERS_DRIVER_H
#define SCI_SOUND_DRIVERS_DRIVER_H

#include "audio/audiostream.h"
#include "sci/sound/sound.h"

namespace Sci {

/**
 * Common Sound driver interface for all SCI revisions.
 */
class SoundDriver {
public:
	typedef uint8 DeviceId;

	SoundDriver(ResourceManager &resMan, SciVersion version) :
		_version(version) {}

	virtual ~SoundDriver() {}

	/**
	 * Returns the number of simultaneous voices this driver supports.
	 */
	virtual int getNumVoices() const = 0;

	/**
	 * Returns the device ID used to find the correct data in a Sound file.
	 */
	virtual DeviceId getDeviceId() const = 0;

protected:
	SciVersion _version;
};

/**
 * Sound driver interface for SCI0.
 */
class Sci0SoundDriver : public SoundDriver {
public:
	Sci0SoundDriver(ResourceManager &resMan, SciVersion version) :
		SoundDriver(resMan, version) {}

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

	Sci1SoundDriver(ResourceManager &resMan, SciVersion version) :
		SoundDriver(resMan, version),
		_isEnabled(true) {}

	/**
	 * Returns the valid channel range for the driver. In SSCI, this information
	 * would be communicated by a call to the driver's init function.
	 */
	virtual void getValidChannelRange(int8 &low, int8 &high) const = 0;

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
	virtual uint8 getReverbMode() const = 0;

	/**
	 * Sets a new reverb mode for the driver and returns the previous mode.
	 */
	virtual uint8 setReverbMode(const uint8 mode) = 0;

	// In SSCI, these two functions were one MasterVol function, where sending
	// volume 0xFF would return the current volume without changing anything.

	/**
	 * Gets the master volume (0-15).
	 */
	virtual int8 getMasterVolume() const = 0;

	/**
	 * Sets the master volume and returns the previous volume.
	 */
	virtual int8 setMasterVolume(const int8 volume) = 0;

	// In SSCI, these two functions were one SoundOn function, where sending
	// 0xFF would return the current state without changing anything.

	/**
	 * Gets whether or not sound playback is enabled.
	 */
	virtual bool isEnabled() const { return _isEnabled; }

	/**
	 * Enables or disables sound playback. The previous state is returned.
	 */
	virtual bool enable(const bool enable) {
		const bool old = _isEnabled;
		_isEnabled = enable;
		return old;
	}

	// TODO: There are three additional APIs for digital sample playback which
	// are used by at least Amiga/Mac.

protected:
	/**
	 * Whether or not sound playback is enabled.
	 */
	bool _isEnabled;
};

} // End of namespace Sci

#endif

