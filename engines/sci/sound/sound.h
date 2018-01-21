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
#include "audio/mididrv.h"
#include "common/array.h"
#include "common/list.h"
#include "common/mutex.h"
#include "common/ptr.h"
#include "common/serializer.h"
#include "common/scummsys.h"
#include "sci/engine/vm_types.h"
#include "sci/resource.h"
#include "sci/sound/drivers/driver.h"
#include "sci/util.h"

namespace Audio { class Mixer; }

namespace Sci {
class Console;
class GuestAdditions;
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

#pragma mark -
#pragma mark SoundManager

class SoundManager : public Common::Serializable {
public:
	SoundManager(ResourceManager &resMan, SegManager &segMan, GameFeatures &features, GuestAdditions &guestAdditions);

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
	virtual void reset() = 0;

	/**
	 * Gets the maximum number of simultaneous voices supported by the current
	 * sound driver.
	 */
	int getNumVoices() const {
		return _driver->getNumVoices();
	}

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

protected:
	ResourceManager &_resMan;
	SegManager *_segMan;
	GuestAdditions &_guestAdditions;
	Common::Mutex _mutex;

	void initDriver(MusicType musicType, Common::Platform platform);

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
	 * The currently active version of the sound code.
	 */
	SciVersion _soundVersion;

	/**
	 * The sound driver.
	 */
	Common::ScopedPtr<SoundDriver> _driver;

#pragma mark -
#pragma mark Save management
public:
	virtual void reconstructPlaylist() = 0;

#pragma mark -
#pragma mark Sound server
public:
	/**
	 * Enables and disables the sound server. This method uses a counter.
	 */
	virtual void enableSoundServer(const bool enable) = 0;

	virtual void pauseAll(const bool pause) = 0;

#pragma mark -
#pragma mark Effects
public:
	enum {
		kMaxMasterVolume = 15
	};

	// TODO: was MasterVol, one function
	uint8 getMasterVolume() const;
	void setMasterVolume(const uint8 volume);

	/**
	 * Returns whether sound reproduction is enabled in the driver.
	 */
	bool isSoundEnabled() const;

	/**
	 * Enables and disables sound reproduction in the driver. This method is
	 * idempotent.
	 */
	void setSoundOn(const bool enable);

#pragma mark -
#pragma mark Kernel
public:
	virtual void kernelInit(const reg_t soundObj) = 0;
	virtual void kernelDispose(const reg_t soundObj) = 0;
	virtual void kernelPlay(const reg_t soundObj, const bool exclusive) = 0;
	virtual void kernelStop(const reg_t soundObj) = 0;
	virtual int16 kernelPause(const reg_t soundObj) = 0;
	virtual void kernelPause(const reg_t soundObj, const int16 numPauses, const bool pauseDac) = 0;
	virtual void kernelFade(const reg_t soundObj) = 0;
	virtual void kernelFade(const reg_t soundObj, const int16 targetVolume, const int16 speed, const int16 steps, const bool stopAfterFade) = 0;
	virtual void kernelHold(const reg_t soundObj, const int16 holdPoint) = 0;
	virtual void kernelSetVolume(const reg_t soundObj, const int16 volume) = 0;
	virtual void kernelSetPriority(const reg_t soundObj, const int16 priority) = 0;
	virtual void kernelSetLoop(const reg_t soundObj, const bool enable) = 0;
	virtual void kernelUpdateCues(const reg_t soundObj) = 0;
	virtual void kernelSendMidi(const int argc, const reg_t *const argv) = 0;
	virtual void kernelUpdate(const reg_t soundObj) = 0;
	virtual reg_t kernelGlobalReverb(const int argc, const reg_t *const argv) = 0;

protected:
	struct Kernel {
		enum Signal {
			kNoSignal = 0,
			kFinished = -1
		};
	};

#pragma mark -
#pragma mark Debugging
public:
	virtual void debugPrintPlaylist(Console &con) const = 0;
	virtual void debugPrintSound(Console &con, const uint index) const = 0;
	virtual void debugPrintChannelMap(Console &con) const = 0;
	virtual void debugPrintDriverState(Console &con) const = 0;
	virtual void debugPlaySound(Console &con, const GuiResourceId resourceNo, const bool exclusive) = 0;
	virtual void debugStopAll() = 0;
};

} // End of namespace Sci
#endif
