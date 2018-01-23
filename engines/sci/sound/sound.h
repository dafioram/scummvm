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
#include "audio/mixer.h"
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

	/**
	 * Initialises the sound driver for the given music type/platform.
	 * This must be called from the constructor.
	 */
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
	/**
	 * Reconstucts the playlist and restores sound playback after all other
	 * parts of the save game have been restored.
	 */
	virtual void reconstructPlaylist() = 0;

#pragma mark -
#pragma mark MIDI server
public:
	/**
	 * Enables and disables the sound server.
	 */
	void enableSoundServer(const bool enable);

	/**
	 * Pauses or unpauses all sounds.
	 */
	virtual void pauseAll(const bool pause) = 0;

protected:
	/**
	 * The number of times the sound server has been suspended.
	 */
	int _numServerSuspensions;

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

protected:
	virtual uint8 getMasterVolumeImpl() const = 0;
	virtual void setMasterVolumeImpl(const uint8 volume) = 0;

#pragma mark -
#pragma mark Data processing
protected:
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

	inline uint16 convert7To16(const byte lsb, const byte msb) const {
		return (msb << 7) | lsb;
	}

#pragma mark -
#pragma mark Digital sample playback
protected:
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

		struct Sample {
			enum {
				kLoopForever = 0xffff
			};

			SciSpan<const byte> data;
			bool isSigned;
			uint16 numLoops;
			uint16 startPosition;
			uint16 rate;
			uint16 size;
			uint16 loopStart;
			uint16 loopEnd;
			uint8 volume;
		};

		void load(const Sample &sample);

		// In SSCI this received pointer to sample data and used AL and AH to
		// communicate status
		Status advance(const uint16 numLoops);

		void pause() { _playing = false; }

		// In SSCI this received pointer to sample data
		void unload();

		virtual int readBuffer(int16 *buffer, int numSamples) override;

		virtual bool isStereo() const override { return false; }

		virtual int getRate() const override { return _sample.rate; }

		virtual bool endOfData() const override { return false; }

		uint16 consumeLoops() {
			const uint16 numLoops = _numTimesLooped;
			_numTimesLooped = 0;
			return numLoops;
		}

	private:
		SoundManager &_manager;
		Audio::Mixer &_mixer;
		Audio::SoundHandle _handle;
		Sample _sample;
		bool _playing;
		uint16 _numTimesLooped;
		uint16 _pos;
	};

	SamplePlayer _samplePlayer;

#pragma mark -
#pragma mark Kernel
public:
	virtual void kernelInit(const reg_t soundObj) = 0;
	virtual void kernelDispose(const reg_t soundObj) = 0;
	virtual void kernelPlay(const reg_t soundObj, const bool exclusive) = 0;
	virtual void kernelStop(const reg_t soundObj) = 0;
	virtual bool kernelPause(const bool paused) {
		error("Wrong SCI version for kernelPause");
	}
	virtual void kernelPause(const reg_t soundObj, const int16 numPauses, const bool pauseDac) {
		error("Wrong SCI version for kernelPause");
	}
	virtual void kernelFade(const reg_t soundObj) {
		error("Wrong SCI version for kernelFade");
	}
	virtual void kernelFade(const reg_t soundObj, const int16 targetVolume, const int16 speed, const int16 steps, const bool stopAfterFade) {
		error("Wrong SCI version for kernelFade");
	}
	virtual void kernelHold(const reg_t soundObj, const int16 holdPoint) {
		error("Wrong SCI version for kernelHold");
	}
	virtual void kernelSetVolume(const reg_t soundObj, const int16 volume) {
		error("Wrong SCI version for kernelSetVolume");
	}
	virtual void kernelSetPriority(const reg_t soundObj, const int16 priority) {
		error("Wrong SCI version for kernelSetPriority");
	}
	virtual void kernelSetLoop(const reg_t soundObj, const bool enable) {
		error("Wrong SCI version for kernelSetLoop");
	}
	virtual void kernelUpdateCues(const reg_t soundObj) {
		error("Wrong SCI version for kernelUpdateCues");
	}
	virtual void kernelSendMidi(const int argc, const reg_t *const argv) {
		error("Wrong SCI version for kernelSendMidi");
	}
	virtual void kernelUpdate(const reg_t soundObj) = 0;
	virtual reg_t kernelGlobalReverb(const int argc, const reg_t *const argv) {
		error("Wrong SCI version for kernelGlobalReverb");
	}
	virtual void kernelPlayNext() {
		error("Wrong SCI version for kernelPlayNext");
	}

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
	void debugPrintDriverState(Console &con) const;
	virtual void debugPlaySound(Console &con, const GuiResourceId resourceNo, const bool exclusive) = 0;
	virtual void debugStopAll() = 0;
};

} // End of namespace Sci
#endif
