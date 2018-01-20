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
#include "sci/console.h"
#include "sci/sound/sound.h"

namespace Sci {

/**
 * Common Sound driver interface for all SCI revisions.
 */
class SoundDriver {
public:
	// SCI0 ops (offsets):
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

	// SCI1 ops:
	// 0 - get device info (getPatchNumber/getNumVoices/getNumDacs)
	// 1 - init device (constructor)
	// 2 - shutdown device (destructor)
	// 3 - service driver (60Hz)
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
	// 14 - load sample
	// 15 - stop sample
	// 16 - advance sample
	// 17 - unused

	typedef uint8 DeviceId;

	SoundDriver(ResourceManager &resMan, SciVersion version) :
		_version(version),
		_isEnabled(true) {}

	virtual ~SoundDriver() {}

	/**
	 * Returns the number of simultaneous voices this driver supports.
	 */
	virtual int getNumVoices() const = 0;

	/**
	 * Returns the device ID used to find the correct data in a Sound file.
	 */
	virtual DeviceId getDeviceId() const = 0;

	/**
	 * Returns the channel range which can be used for dynamically remapped
	 * channels. In SSCI, this information would be communicated by a call to
	 * the driver's init function.
	 */
	virtual void getRemapRange(uint8 &low, uint8 &high) const = 0;

	/**
	 * Services the driver. This should be called once per tick.
	 */
	virtual void service() = 0;

	/**
	 * Programmatically starts playback of a note. For MIDI devices, this is
	 * equivalent to a MIDI Note On message.
	 */
	virtual void noteOn(const uint8 channelIndex, const uint8 note, const uint8 velocity) = 0;

	/**
	 * Programmatically stops playback of a note. For MIDI devices, this is
	 * equivalent to a MIDI Note Off message.
	 */
	virtual void noteOff(const uint8 channelIndex, const uint8 note, const uint8 velocity) = 0;

	/**
	 * Sets the value of a MIDI controller to the given value. For MIDI devices,
	 * this is equivalent to a MIDI Control Change message.
	 */
	virtual void controllerChange(const uint8 channelIndex, const uint8 controllerNo, const uint8 value) = 0;

	/**
	 * Programmatically sets the program (patch/instrument) for the given
	 * channel to the given program number. For MIDI devices, this is equivalent
	 * to a MIDI Program Change message.
	 */
	virtual void programChange(const uint8 channelIndex, const uint8 programNo) = 0;

	/**
	 * Programmatically sets the pitch bend for all notes in a channel. For MIDI
	 * devices, this is equivalent to a MIDI Pitch Bend Change message.
	 */
	virtual void pitchBend(const uint8 channelIndex, const uint16 bend) = 0;

	/**
	 * Sets the aftertouch key pressure for a single note. For MIDI devices,
	 * this is equivalent to a MIDI Polyphonic Key Pressure message.
	 * TODO: was polyAfterTch
	 */
	virtual void keyPressure(const uint8 channelIndex, const uint8 note, const uint8 pressure) = 0;

	/**
	 * Programmatically sets the aftertouch pressure for all notes in a channel.
	 * For MIDI devices, this is equivalent to a MIDI Channel Pressure message.
	 * TODO: was chnlAfterTch
	 */
	virtual void channelPressure(const uint8 channelIndex, const uint8 pressure) = 0;

	// In SSCI, these two functions were one SetReverb function, where sending
	// mode 0xFF would return the current reverb mode without changing anything.
	// In our driver model, 0xff will not be sent to setReverbMode unless there
	// is a bug.

	/**
	 * Gets the currently active reverb mode for the driver.
	 */
	virtual uint8 getReverbMode() const = 0;

	/**
	 * Sets a new reverb mode for the driver and returns the previous mode.
	 */
	virtual void setReverbMode(const uint8 mode) = 0;

	// In SSCI, these two functions were one MasterVol function, where sending
	// volume 0xFF would return the current volume without changing anything.
	// In our driver model, 0xff will not be sent to setMasterVolume unless
	// there is a bug.

	/**
	 * Gets the master volume (0-15).
	 */
	virtual uint8 getMasterVolume() const = 0;

	/**
	 * Sets the master volume and returns the previous volume.
	 */
	virtual void setMasterVolume(const uint8 volume) = 0;

	// In SSCI, these two functions were one SoundOn function, where sending
	// 0xFF would return the current state without changing anything.

	/**
	 * Gets whether or not sound playback is enabled.
	 */
	virtual bool isEnabled() const { return _isEnabled; }

	/**
	 * Enables or disables sound playback.
	 */
	virtual void enable(const bool enable) { _isEnabled = enable; }

	/**
	 * Prints state information usable for debugging the driver.
	 */
	virtual void debugPrintState(Console &con) const {
		con.debugPrintf("No debugging information available from the current driver");
	}

protected:
	/**
	 * The SCI version which should be emulated by the driver.
	 */
	SciVersion _version;

	/**
	 * Whether or not sound playback is enabled.
	 */
	bool _isEnabled;
};

} // End of namespace Sci

#endif

