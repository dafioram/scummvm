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

#include "audio/mididrv.h"
#include "audio/mixer.h"
#include "common/algorithm.h"
#include "common/config-manager.h"
#include "common/platform.h"
#include "common/system.h"
#include "common/timer.h"
#include "sci/console.h"
#include "sci/engine/features.h"
#include "sci/engine/guest_additions.h"
#include "sci/engine/kernel.h"
#include "sci/engine/seg_manager.h"
#include "sci/engine/selector.h"
#include "sci/sound/audio.h"
#ifdef ENABLE_SCI32
#include "sci/sound/audio32.h"
#endif
#include "sci/sound/sound.h"
#include "sci/sound/drivers/adlib.h"
#include "sci/sound/drivers/genmidi.h"

namespace Sci {

static inline uint16 convert7To16(byte lsb, byte msb) {
	return (msb << 7) | lsb;
}

static SciSpan<const byte> findTrackOffsets(SciSpan<const byte> data, const Sci1SoundDriver::DeviceId searchId) {
	if (data[0] == 0xf0) {
		data += 8;
	}

	for (uint8 deviceId = *data++; deviceId != 0xff; deviceId = *data++) {
		if (deviceId == searchId) {
			return data;
		} else {
			while (*data++ != 0xff) {
				data += 5;
			}
		}
	}

	return SciSpan<const byte>();
}

SoundManager::SoundManager(ResourceManager &resMan, SegManager &segMan, GameFeatures &features, GuestAdditions &guestAdditions) :
	_resMan(resMan),
	_segMan(&segMan),
	_guestAdditions(guestAdditions),
	_driverEnabledState(true),
	_soundVersion(features.detectDoSoundType()),
	_restoringSound(false),
	_numServerSuspensions(0),
	_needsUpdate(false),
	_nextVolumeChangeChannel(0),
	_defaultReverbMode(0),
	_sample(nullptr),
	_playlist() {
	_preferSampledSounds = _soundVersion >= SCI_VERSION_2 ||
		g_sci->getGameId() == GID_GK1DEMO ||
		ConfMan.getBool("prefer_digitalsfx");
	_sounds.reserve(16);
	Common::fill(_newChannelVolumes.begin(), _newChannelVolumes.end(), kNoVolumeChange);

	uint32 deviceFlags;
#ifdef ENABLE_SCI32
	if (gameHasGeneralMidiOnly()) {
		deviceFlags = MDT_MIDI;
	} else {
#endif
		deviceFlags = MDT_PCSPK | MDT_PCJR | MDT_ADLIB | MDT_MIDI | MDT_CMS;
#ifdef ENABLE_SCI32
	}
#endif

	// Default to MIDI for Windows versions of SCI1.1 games, as their
	// soundtrack is written for GM.
	if (features.useAltWinGMSound()) {
		deviceFlags |= MDT_PREFER_GM;
		_useWindowsMidi = true;
	} else {
		_useWindowsMidi = false;
	}

	const Common::Platform platform = g_sci->getPlatform();

	if (platform == Common::kPlatformFMTowns) {
		// TODO: Figure out why these device flags are handled this way (by
		// version)
		if (_soundVersion > SCI_VERSION_1_EARLY)
			deviceFlags = MDT_TOWNS;
		else
			deviceFlags |= MDT_TOWNS;
	}

	const MidiDriver::DeviceHandle dev = MidiDriver::detectDevice(deviceFlags);
	const MusicType musicType = MidiDriver::getMusicType(dev);

	if (features.useAltWinGMSound() && musicType != MT_GM) {
		warning("A Windows CD version with an alternate MIDI soundtrack has been chosen, "
				"but no MIDI music device has been selected. Reverting to the DOS soundtrack");
		features.forceDOSTracks();
#ifdef ENABLE_SCI32
	} else if (gameHasGeneralMidiOnly() && musicType != MT_GM) {
		warning("This game only supports General MIDI, but a non-GM device has "
				"been selected. Some music may be wrong or missing");
#endif
	}

	switch (musicType) {
	case MT_ADLIB:
		// FIXME: There's no Amiga sound option, so we hook it up to AdLib
		if (platform == Common::kPlatformAmiga || platform == Common::kPlatformMacintosh) {
			error("TODO AmigaMac");
//			_driver.reset(makeAmigaMacDriver(resMan, _soundVersion));
		} else {
			// TODO: Wrong!
			_driver.reset(static_cast<Sci1SoundDriver *>(makeAdLibDriver(resMan, _soundVersion)));
		}
		break;
	case MT_PCJR:
		error("TODO PCJr");
//		_driver.reset(makePcJrDriver(resMan, _soundVersion));
		break;
	case MT_PCSPK:
		error("TODO PCSpeaker");
//		_driver.reset(makePcSpeakerDriver(resMan, _soundVersion));
		break;
	case MT_CMS:
		error("TODO CMS");
//		_driver.reset(makeCmsDriver(resMan, _soundVersion));
		break;
	case MT_TOWNS:
		error("TODO FM-Towns");
//		_driver.reset(makeFmTownsDriver(resMan, _soundVersion));
		break;
	case MT_MT32:
		error("TODO MT-32");
		break;
	case MT_GM:
		if (ConfMan.getBool("native_fb01")) {
			error("TODO FB-01");
//			_driver.reset(makeFb01Driver(resMan, _soundVersion));
		} else {
			// TODO: Wrong!
			_driver.reset(static_cast<Sci1SoundDriver *>(makeGeneralMidiDriver(resMan, _soundVersion)));
		}
		break;
	default:
		error("Unknown music type %d", musicType);
	}

	if (!_driver) {
		// HACK: The Fun Seeker's Guide demo doesn't have patch 3 and the
		// version of the Adlib driver (adl.drv) that it includes is
		// unsupported. That demo doesn't have any sound anyway, so this
		// shouldn't be fatal.
		// The GK2 demo has a similar issue, it has no audio drivers at all but
		// plays no MIDI it is not fatal for it to have no driver.
		if (g_sci->getGameId() == GID_FUNSEEKER ||
			(g_sci->getGameId() == GID_GK2 && g_sci->isDemo())) {
			return;
		}

		error("Failed to initialize sound driver");
	}

	// In SSCI, this is in SInit; since we do not implement that operation, we
	// perform its additional initialisation operations here
	_driver->setReverbMode(0);

	g_system->getTimerManager()->installTimerProc(soundServerCallback, 1000000 / 60, this, "SCI MIDI");
}

SoundManager::~SoundManager() {
	g_system->getTimerManager()->removeTimerProc(soundServerCallback);

	// Don't allow destruction to finish until after any in-progress sound
	// server callback has finished running
	Common::StackLock lock(_mutex);

	// In SSCI, this is in STerminate; since we do not implement that operation,
	// we perform its additional termination operations here
	if (_driver) {
		_driver->setMasterVolume(kMaxMasterVolume);
	}
}

int SoundManager::getNumVoices() const {
	return _driver->getNumVoices();
}

void SoundManager::systemSuspend(const bool pause) {
	enableSoundServer(!pause);
	if (pause) {
		_driverEnabledState = _driver->isEnabled();
		_driver->enable(false);
	} else{
		_driver->enable(_driverEnabledState);
	}
}

GuiResourceId SoundManager::getSoundResourceId(const uint16 soundNo) const {
	// Some Windows sounds don't exist (e.g. SQ4, room 530 - bug #3392767) so
	// don't just unconditionally use the higher value
	if (_useWindowsMidi && soundNo != 0) {
		const ResourceId testId(kResourceTypeSound, soundNo + 1000);
		if (_resMan.testResource(testId)) {
			return soundNo + 1000;
		}
	}

	// SQ4CD has some higher-quality samples in the Windows-range
	if (soundNo < 1000 && _preferSampledSounds &&
		g_sci->getGameId() == GID_SQ4 && g_sci->isCD() &&
		_resMan.testResource(ResourceId(kResourceTypeAudio, soundNo + 1000))) {

		return soundNo + 1000;
	}

	return soundNo;
}

bool SoundManager::gameHasGeneralMidiOnly() const {
#ifdef ENABLE_SCI32
	switch (g_sci->getGameId()) {
	case GID_MOTHERGOOSEHIRES:
		return true;
	case GID_KQ7: {
		if (g_sci->isDemo()) {
			return false;
		}

		Resource *sound = _resMan.findResource(ResourceId(kResourceTypeSound, 13), false);
		if (!sound) {
			return false;
		}

		enum { kAdLib = 0 };
		return findTrackOffsets(*sound, kAdLib);
	}
	default:
		break;
	}
#endif

	return false;
}

#pragma mark -
#pragma mark Save management

void SoundManager::saveLoadWithSerializer(Common::Serializer &s) {
	error("TODO: Save/load for new sound system");
}

void SoundManager::restore(Sci1Sound &sound) {
	Common::StackLock lock(_mutex);

	uint8 muteCounts[Sci1Sound::kNumChannels];
	const uint8 holdPoint = sound.holdPoint;

	for (int i = 0; i < Sci1Sound::kNumChannels; ++i) {
		muteCounts[i] = sound.getChannel(i).gameMuteCount;
	}

	_restoringSound = true;

	const uint8 playlistIndex = play(sound, false);

	uint16 ticksToRestore = sound.ticksElapsed;
	sound.ticksElapsed = 0;
	const bool loopToRestore = sound.loop;
	sound.loop = true;

	while (sound.ticksElapsed != ticksToRestore) {
		uint16 lastTicks = sound.ticksElapsed;
		parseNextNode(sound, playlistIndex);
		if (lastTicks == sound.ticksElapsed) {
			break;
		} else if (lastTicks > sound.ticksElapsed) {
			ticksToRestore -= lastTicks - sound.ticksElapsed;
		}
	}

	sound.loop = loopToRestore;
	_restoringSound = false;
	for (int i = 0; i < Sci1Sound::kNumChannels; ++i) {
		sound.getChannel(i).gameMuteCount = muteCounts[i];
	}
	sound.holdPoint = holdPoint;
	updateChannelList();
}

#pragma mark -
#pragma mark MIDI server

void SoundManager::enableSoundServer(const bool enable) {
	Common::StackLock lock(_mutex);
	if (!enable) {
		++_numServerSuspensions;
	} else if (_numServerSuspensions) {
		--_numServerSuspensions;
	}
}

void SoundManager::soundServer() {
	Common::StackLock lock(_mutex);
	if (_numServerSuspensions) {
		return;
	}

	if (_needsUpdate) {
		updateChannelList();
	}

	for (int i = 0; i < kPlaylistSize && _playlist[i]; ++i) {
		Sci1Sound &sound = *_playlist[i];

		// TODO: SSCI also checked if the resource pointer == 1 and would avoid
		// processing in that case as well, not sure why this is a thing or what
		// is the best way to do that.
		if (sound.paused || sound.isSample) {
			continue;
		}

		if (sound.fadeAmountPerTick) {
			processFade(sound);

			if (sound.signal == Sci1Sound::kFinished) {
				--i;
				continue;
			}
		}

		parseNextNode(sound, i);

		if (sound.signal == Sci1Sound::kFinished) {
			--i;
		}
	}

	applyPendingVolumeChanges();
	_driver->service();
}

void SoundManager::processFade(Sci1Sound &sound) {
	if (sound.fadeDelayRemaining) {
		// TODO: Use wall time for this instead of relying on realtime callbacks
		--sound.fadeDelayRemaining;
		return;
	} else {
		sound.fadeDelayRemaining = sound.fadeDelay;
	}

	if (sound.fadeTargetVolume == sound.volume) {
		sound.signal = Sci1Sound::kFadeFinished;
		sound.fadeAmountPerTick = 0;
		if (sound.stopSoundOnFade) {
			removeSoundFromPlaylist(sound);
			_needsUpdate = true;
		}
		return;
	}

	const int delta = ABS(int(sound.fadeTargetVolume) - int(sound.volume));
	uint8 newVolume = sound.volume;
	if (delta < sound.fadeAmountPerTick) {
		newVolume = sound.fadeTargetVolume;
	} else if (sound.fadeTargetVolume < sound.volume) {
		newVolume -= sound.fadeAmountPerTick;
	} else {
		newVolume += sound.fadeAmountPerTick;
	}

	processVolumeChange(sound, newVolume, true);
}

#pragma mark -
#pragma mark Effects

uint8 SoundManager::getMasterVolume() const {
	Common::StackLock lock(_mutex);
	return _driver->getMasterVolume();
}

uint8 SoundManager::setMasterVolume(uint8 volume) {
	Common::StackLock lock(_mutex);
	if (volume > kMaxMasterVolume) {
		volume = kMaxMasterVolume;
	}
	return _driver->setMasterVolume(volume);
}

uint8 SoundManager::getReverbMode() const {
	Common::StackLock lock(_mutex);
	return _driver->getReverbMode();
}

uint8 SoundManager::setReverbMode(const uint8 reverbMode) {
	Common::StackLock lock(_mutex);

	// Conditions for reverbMode == 0xff and reverbMode > 10 are moved into
	// kDoSoundGlobalReverb

	uint8 oldReverbMode = _defaultReverbMode;
	_defaultReverbMode = reverbMode;
	if (_playlist[0]) {
		oldReverbMode = _driver->setReverbMode(reverbMode);
	}
	return oldReverbMode;
}

void SoundManager::setSoundOn(const bool enable) {
	Common::StackLock lock(_mutex);
	_driver->enable(enable);
}

void SoundManager::setVolume(Sci1Sound &sound, const uint8 volume) {
	Common::StackLock lock(_mutex);
	processVolumeChange(sound, volume, false);
}

void SoundManager::fade(Sci1Sound &sound, const int16 targetVolume, const int16 speed, const int16 steps, const bool stopAfterFade) {
	Common::StackLock lock(_mutex);
	if (sound.volume == targetVolume) {
		return;
	}

	// TODO: In SSCI stop flag was baked into the high bit of target volume,
	// make sure that we pull that out if we need to
	sound.fadeTargetVolume = targetVolume;
	sound.stopSoundOnFade = stopAfterFade;
	sound.fadeAmountPerTick = steps;
	sound.fadeDelay = speed;
	sound.fadeDelayRemaining = 0;
}

void SoundManager::mute(Sci1Sound &sound, const bool mute) {
	Common::StackLock lock(_mutex);
	for (int channelNo = Sci1Sound::kNumChannels - 1; channelNo >= 0; --channelNo) {
		Sci1Sound::Channel &channel = sound.getChannel(channelNo);
		if (mute) {
			if (channel.gameMuteCount < 15) {
				++channel.gameMuteCount;
			}
		} else if (channel.gameMuteCount) {
			--channel.gameMuteCount;
		}
	}
	updateChannelList();
}

void SoundManager::processVolumeChange(Sci1Sound &sound, const uint8 volume, const bool enqueue) {
	if (volume == sound.volume) {
		return;
	}

	sound.volume = volume;

	uint8 playlistIndex = findPlaylistIndex(sound);
	if (playlistIndex == kPlaylistSize) {
		return;
	}
	playlistIndex <<= 4;

	for (int hwChannelNo = 0; hwChannelNo < kNumHardwareChannels; ++hwChannelNo) {
		const HardwareChannel &hwChannel = _hardwareChannels[hwChannelNo];
		if (hwChannel.isMapped() && hwChannel.playlistIndex() == playlistIndex) {
			changeChannelVolume(sound, hwChannel.channelNo(), hwChannelNo, enqueue);
		}
	}

	for (int trackNo = 0; trackNo < Sci1Sound::kNumTracks; ++trackNo) {
		const uint8 channelNo = sound.getTrack(trackNo).channelNo;
		if (channelNo == Sci1Sound::Track::kEndOfData) {
			break;
		}

		const Sci1Sound::Channel &channel = sound.getChannel(channelNo);
		if ((channel.flags & Sci1Sound::Channel::kExtra) &&
			!_hardwareChannels[channelNo].isMapped()) {

			changeChannelVolume(sound, channelNo, channelNo, enqueue);
		}
	}
}

void SoundManager::changeChannelVolume(const Sci1Sound &sound, const uint8 channelNo, const uint8 hwChannelNo, const bool enqueue) {
	const uint8 channelVolume = sound.getChannel(channelNo).volume * sound.volume / Sci1Sound::kMaxVolume;
	uint8 &newVolume = _newChannelVolumes[hwChannelNo];
	if (enqueue) {
		newVolume = channelVolume;
	} else {
		newVolume = kNoVolumeChange;
		_driver->controllerChange(hwChannelNo, kVolumeController, channelVolume);
	}
}

void SoundManager::applyPendingVolumeChanges() {
	uint8 &channelNo = _nextVolumeChangeChannel;
	const uint8 originalChannel = channelNo;
	int numUpdates = 0;
	do {
		uint8 &volume = _newChannelVolumes[channelNo];
		if (volume != kNoVolumeChange) {
			_driver->controllerChange(channelNo, kVolumeController, volume);
			++numUpdates;
			volume = kNoVolumeChange;
		}
		if (++channelNo == kNumHardwareChannels) {
			channelNo = 0;
		}
	} while (numUpdates < 2 && channelNo != originalChannel);
}

#pragma mark -
#pragma mark Playback management

uint8 SoundManager::play(Sci1Sound &sound, const bool exclusive) {
	if (findPlaylistIndex(sound) != kPlaylistSize) {
		removeSoundFromPlaylist(sound);
	}

	sound.state = exclusive ? Sci1Sound::kExclusive : Sci1Sound::kPlaying;
	sound.resetPrivateState();
	sound.isSample = false;
	sound.holdPoint = 0;
	sound.reverbMode = kUseDefaultReverb;
	sound.loopTicksElapsed = 0;
	sound.volume = Sci1Sound::kMaxVolume;

	SciSpan<const byte> base = *sound.resource;

	// This is a little different than SSCI because we do not scribble a new
	// header onto the file, so this used to be stuff that fixupHeader did.

	if (!sound.fixedPriority && base[0] == 0xf0) {
		sound.priority = base[1];
		base += 8;
	}

	readTrackOffsets(sound);

	for (int trackNo = 0; trackNo < Sci1Sound::kNumTracks; ++trackNo) {
		Sci1Sound::Track &track = sound.getTrack(trackNo);
		if (track.offset == 0) {
			break;
		}
		SciSpan<const byte> trackData = sound.resource->subspan(track.offset);

		const uint8 channelNo = trackData[0];
		track.channelNo = channelNo & 0xf;
		track.command = kControllerChange | track.channelNo;
		const uint8 message = trackData[12];
		if (message == kFixedRest) {
			track.rest = kFixedRestValue;
		} else {
			track.rest = message;
		}

		// SSCI did not do a check for the control channel so early, instead
		// relied on the MIDI data not containing a bad channel number value
		// to not do out-of-bounds writes. We do the check so we can reference
		// the Channel object.
		if (track.channelNo != kControlChannel) {
			Sci1Sound::Channel &channel = sound.getChannel(track.channelNo);

			enum {
				kExtraChannelFlag  = 0x10,
				kLockedChannelFlag = 0x20,
				kMutedChannelFlag  = 0x40
			};

			if (channelNo & kExtraChannelFlag) {
				track.position = 3;
				track.rest = 0;
				channel.flags = Sci1Sound::Channel::kExtra;
				continue;
			}

			if (channelNo & kLockedChannelFlag) {
				channel.flags = Sci1Sound::Channel::kLocked;
			}

			if (channelNo & kMutedChannelFlag) {
				channel.muted = true;
			}

			if (channel.priority == Sci1Sound::Channel::kUninitialized) {
				channel.priority = trackData[1] >> 4;
				channel.numVoices = trackData[1] & 4;
			}

			if (channel.program == Sci1Sound::Channel::kUninitialized) {
				channel.program = trackData[4];
			}

			if (channel.volume == Sci1Sound::Channel::kUninitialized) {
				channel.volume = trackData[8];
			}

			if (channel.pan == Sci1Sound::Channel::kUninitialized) {
				channel.pan = trackData[11];
			}
		} else {
			// SSCI checked whether reverbMode != kUseDefaultReverb and then
			// jumped to checking the channel pan, this was an impossible
			// condition and would have led to an out-of-bounds access so is
			// removed
			sound.reverbMode = trackData[8];
		}
	}

	if (sound.state & Sci1Sound::kExclusive) {
		for (int channelNo = 0; channelNo < Sci1Sound::kNumChannels; ++channelNo) {
			Sci1Sound::Channel &channel = sound.getChannel(channelNo);
			channel.flags = Sci1Sound::Channel::ChannelFlags(channel.flags | Sci1Sound::Channel::kLocked);
		}
	}

	const uint8 playlistIndex = insertSoundToPlaylist(sound);
	if (playlistIndex != kPlaylistSize && !_restoringSound) {
		sound.cue = 0;
		sound.ticksElapsed = 0;
		sound.signal = Sci1Sound::kNoSignal;
		sound.fadeTargetVolume = 0;
		sound.fadeDelay = 0;
		sound.fadeDelayRemaining = 0;
		sound.fadeAmountPerTick = 0;
		sound.stopSoundOnFade = false;
		sound.paused = 0;

		updateChannelList();
	}

	return playlistIndex;
}

void SoundManager::pause(Sci1Sound &sound, const bool pause) {
	Common::StackLock lock(_mutex);
	if (pause) {
		++sound.paused;
	} else if (sound.paused) {
		--sound.paused;
	}

	updateChannelList();
}

void SoundManager::pauseAll(const bool pause) {
	Common::StackLock lock(_mutex);
	for (int i = 0; i < kPlaylistSize; ++i) {
		Sci1Sound *sound = _playlist[i];
		if (!sound) {
			// TODO: This is weird
			if (i == 0) {
				continue;
			} else {
				break;
			}
		}

		if (pause) {
			++sound->paused;
		} else if (sound->paused) {
			--sound->paused;
		}
	}

	updateChannelList();
}

void SoundManager::stop(Sci1Sound &sound) {
	Common::StackLock lock(_mutex);
	removeSoundFromPlaylist(sound);
	updateChannelList();
}

void SoundManager::hold(Sci1Sound &sound, const uint8 holdPoint) {
	Common::StackLock lock(_mutex);
	sound.holdPoint = holdPoint;
}

Sci1Sound::Signal SoundManager::peekSignal(const Sci1Sound &sound) {
	Common::StackLock lock(_mutex);
	return sound.signal;
}

Sci1Sound::Signal SoundManager::consumeSignal(Sci1Sound &sound) {
	Common::StackLock lock(_mutex);
	Sci1Sound::Signal signal = sound.signal;
	sound.signal = Sci1Sound::kNoSignal;
	return signal;
}

uint16 SoundManager::getCue(const Sci1Sound &sound) {
	Common::StackLock lock(_mutex);
	return sound.cue;
}

SoundManager::Position SoundManager::getPosition(const Sci1Sound &sound) {
	uint16 ticksElapsed;
	{
		Common::StackLock lock(_mutex);
		ticksElapsed = sound.ticksElapsed;
	}

	Position position;
	position.minutes = ticksElapsed / 3600;
	position.seconds = (ticksElapsed / 60) % 60;
	position.frames = (ticksElapsed % 60) / 2;
	return position;
}

void SoundManager::setPriority(Sci1Sound &sound, const uint8 priority) {
	Common::StackLock lock(_mutex);
	if (sound.priority == priority) {
		return;
	}

	sound.priority = priority;

	const uint8 playlistIndex = findPlaylistIndex(sound);
	if (playlistIndex == kPlaylistSize) {
		return;
	}

	for (int i = playlistIndex; i < kPlaylistSize - 1; ++i) {
		_playlist[i] = _playlist[i + 1];
	}
	_playlist[kPlaylistSize - 1] = nullptr;
	insertSoundToPlaylist(sound);

	updateChannelList();
}

#pragma mark -
#pragma mark Sound generation

void SoundManager::setNoteOff(Sci1Sound &sound, const uint8 channelNo, const uint8 note, const uint8 velocity) {
	Common::StackLock lock(_mutex);

	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	sound.getChannel(channelNo).currentNote = Sci1Sound::Channel::kNoCurrentNote;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != HardwareChannel::kUnmapped) {
		_driver->noteOff(hwChannelNo, note, velocity);
	}
}

void SoundManager::setNoteOn(Sci1Sound &sound, const uint8 channelNo, const uint8 note, const uint8 velocity) {
	Common::StackLock lock(_mutex);

	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	sound.getChannel(channelNo).currentNote = note;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != HardwareChannel::kUnmapped) {
		_driver->noteOn(hwChannelNo, note, velocity);
	}
}

void SoundManager::setController(Sci1Sound &sound, const uint8 channelNo, const uint8 controllerNo, uint8 value) {
	Common::StackLock lock(_mutex);

	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	Sci1Sound::Channel &channel = sound.getChannel(channelNo);

	switch (controllerNo) {
	case kVolumeController:
		channel.volume = value;
		value = channel.volume * sound.volume / Sci1Sound::kMaxVolume;
		break;
	case kPanController:
		channel.pan = value;
		break;
	case kModulationController:
		channel.modulation = value;
		break;
	case kDamperPedalController:
		channel.damperPedalOn = (value != 0);
		break;
	case kMuteController:
		enum { kUnmute = 0 };
		if (value == kUnmute) {
			if (channel.gameMuteCount) {
				--channel.gameMuteCount;
				updateChannelList();
			}
		} else if (channel.gameMuteCount < 15) {
			++channel.gameMuteCount;
			updateChannelList();
		}
		break;
	case kProgramChangeController:
		channel.program = value;
		break;
	}

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != HardwareChannel::kUnmapped) {
		if (controllerNo == kProgramChangeController) {
			_driver->programChange(hwChannelNo, value);
		} else {
			_driver->controllerChange(hwChannelNo, controllerNo, value);
		}
	}
}

void SoundManager::setProgram(Sci1Sound &sound, const uint8 channelNo, const uint8 programNo) {
	Common::StackLock lock(_mutex);

	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	sound.getChannel(channelNo).program = programNo;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != HardwareChannel::kUnmapped) {
		_driver->programChange(hwChannelNo, programNo);
	}
}

void SoundManager::setPitchBend(Sci1Sound &sound, const uint8 channelNo, const uint16 value) {
	Common::StackLock lock(_mutex);

	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	sound.getChannel(channelNo).pitchBend = value;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != HardwareChannel::kUnmapped) {
		_driver->pitchBend(hwChannelNo, value);
	}
}

#pragma mark -
#pragma mark Data processing

void SoundManager::readTrackOffsets(Sci1Sound &sound) {
	Sci1SoundDriver::DeviceId deviceId = _driver->getDeviceId();
	SciSpan<const byte> data = findTrackOffsets(*sound.resource, deviceId);
	if (!data) {
		error("Unable to find track offsets for device ID %u in %s", deviceId, sound.resource->name().c_str());
	}

	int trackNo = 0;
	for (uint8 endMarker = *data; endMarker != 0xff; endMarker = *data) {
		// TODO: Could be SCI-endian
		Sci1Sound::Track &track = sound.getTrack(trackNo++);
		track.offset = data.getUint16LEAt(2);
		track.size = data.getUint16LEAt(4);
		data += 6;
	}
}

void SoundManager::parseNextNode(Sci1Sound &sound, uint8 playlistIndex) {
	// TODO: Use wall time when _restoringSound is not true
	++sound.ticksElapsed;

	for (int trackNo = 0; trackNo < Sci1Sound::kNumTracks; ++trackNo) {
		// In SSCI playlist index was shifted here, we do it at point of use
		// below

		Sci1Sound::Track &track = sound.getTrack(trackNo);
		if (track.channelNo == Sci1Sound::Track::kEndOfData) {
			break;
		}

		if (track.channelNo == Sci1Sound::Track::kSampleTrack) {
			continue;
		}

		// SSCI did channel flags checking here; we do that later on since it
		// is wasted work for idle tracks and cannot be done for the control
		// channel without triggering assertions (since the control channel has
		// no corresponding data channel in `Sci1Sound::channels`)

		// restorePtr
		if (track.position == 0) {
			continue;
		}

		// notFrozenTrk
		if (track.rest) {
			// TODO: Use wall time when _restoringSound is not true
			--track.rest;

			if (track.rest == kFixedRestFlag) {
				// The last rest was a "timing overflow" rest and now its timer
				// has ticked down to zero
				const byte message = sound.consume(trackNo);
				if (message == kFixedRest) {
					track.rest = kFixedRestValue;
				} else {
					track.rest = message;
				}
			}
		} else {
			parseCommand(sound, playlistIndex, trackNo, track);
		}
	}

	// outParse

	for (int i = 0; i < Sci1Sound::kNumTracks; ++i) {
		Sci1Sound::Track &track = sound.getTrack(i);
		if (track.channelNo == Sci1Sound::Track::kEndOfData) {
			break;
		}
		if (track.position != 0) {
			// At least one track is still running
			return;
		}
	}

	if (sound.holdPoint || sound.loop) {
		sound.ticksElapsed = sound.loopTicksElapsed;
		for (int i = 0; i < Sci1Sound::kNumTracks; ++i) {
			Sci1Sound::Track &track = sound.getTrack(i);
			track.position = track.loopPosition;
			track.rest = track.loopRest;
			track.command = track.loopCommand;
		}
	} else {
		removeSoundFromPlaylist(sound);
		_needsUpdate = true;
	}
}

void SoundManager::parseCommand(Sci1Sound &sound, const uint8 playlistIndex, const uint8 trackNo, Sci1Sound::Track &track) {
	// parseCommand
	byte message; // dl
	do {
		message = sound.peek(trackNo);
		if (message & kStartOfMessageFlag) {
			track.command = message;
			sound.advance(trackNo);
		} else {
			// runningStat
			message = track.command;
		}

		// parseIt

		// Command/channel splitting was moved down from here nearer to its
		// point of use

		if (message == kEndOfTrack) {
			track.position = 0;
			// jmp parseNext
			return;
		}

		const MidiMessageType command = MidiMessageType(message & 0xf0); // ah
		const uint8 channelNo = message & 0xf; // al

		// notEndTrk
		if (channelNo == kControlChannel) {
			parseControlChannel(sound, trackNo, command);
			if (track.position == 0) {
				// jmp parseNext
				return;
			}
		} else {
			// notControlCh
			Sci1Sound::Channel &channel = sound.getChannel(channelNo);

			// In SSCI, this code was earlier in the function, and for the
			// control channel, it would overread past the end of channel
			// flags into the mute save field
			uint8 hwChannelNo;
			bool extraChannel;
			if (channel.flags & Sci1Sound::Channel::kExtra) {
				extraChannel = true;
				hwChannelNo = track.channelNo;
			} else {
				extraChannel = false;
				const uint8 key = (playlistIndex << 4) | track.channelNo;
				hwChannelNo = findHwChannelNo(key);
			}

			// al = hwChannelNo
			// ah = command
			// bx = sound
			// si = trackNo

			switch (command) {
			case kNoteOff:
				processNoteOff(sound, trackNo, hwChannelNo);
				break;
			case kNoteOn:
				processNoteOn(sound, trackNo, hwChannelNo);
				break;
			case kKeyPressure:
				processKeyPressure(sound, trackNo, hwChannelNo);
				break;
			case kControllerChange:
				processControllerChange(sound, trackNo, hwChannelNo, extraChannel);
				break;
			case kProgramChange:
				processProgramChange(sound, trackNo, hwChannelNo, extraChannel);
				break;
			case kChannelPressure:
				processChannelPressure(sound, trackNo, hwChannelNo);
				break;
			case kPitchBend:
				processPitchBend(sound, trackNo, hwChannelNo, extraChannel);
				break;
			case kSysEx:
				processSysEx(sound, trackNo, hwChannelNo);
				break;
			default:
				warning("Unknown command %u in track %u", command, trackNo);
				track.position = 0;
				return;
			}
		}
	} while ((message = sound.consume(trackNo)) == 0);

	if (message == kFixedRest) {
		track.rest = kFixedRestValue;
	} else {
		track.rest = message;
	}
	--track.rest;
}

void SoundManager::parseControlChannel(Sci1Sound &sound, const uint8 trackNo, const MidiMessageType command) {
	Sci1Sound::Track &track = sound.getTrack(trackNo);

	switch (command) {
	case kProgramChange: {
		const byte message = sound.consume(trackNo);
		if (message == kSetLoop) {
			const byte value = sound.peek(trackNo);
			if (value == kFixedRest) {
				track.rest = kFixedRestValue;
			} else {
				track.rest = value;
			}

			track.command = kProgramChange | kControlChannel;

			for (int i = 0; i < Sci1Sound::kNumTracks; ++i) {
				Sci1Sound::Track &loopTrack = sound.getTrack(i);
				loopTrack.loopPosition = loopTrack.position;
				loopTrack.loopRest = loopTrack.rest;
				loopTrack.loopCommand = loopTrack.command;
			}

			sound.loopTicksElapsed = sound.ticksElapsed;
			track.rest = 0;
		} else if (!_restoringSound) {
			sound.signal = Sci1Sound::Signal(message);
		}
		break;
	}
	case kControllerChange: {
		const uint8 controllerNo = sound.consume(trackNo);
		uint8 value = sound.consume(trackNo);
		switch (controllerNo) {
		case kReverbModeController:
			if (value == kUseDefaultReverb) {
				value = _defaultReverbMode;
			}
			sound.reverbMode = value;
			_driver->setReverbMode(value);
			break;
		case kCueController:
			if (!_restoringSound) {
				++sound.cue;
			}
			break;
		case kHoldPointController:
			if (sound.holdPoint == value) {
				for (int i = 0; i < Sci1Sound::kNumTracks; ++i) {
					sound.getTrack(i).position = 0;
				}
			}
			break;
		}
		break;
	}
	default:
		return skipCommand(sound, trackNo, command);
	}
}

void SoundManager::processNoteOff(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	const uint8 note = sound.consume(trackNo);
	const uint8 velocity = sound.consume(trackNo);

	const uint8 channelNo = sound.getTrack(trackNo).channelNo;
	Sci1Sound::Channel &channel = sound.getChannel(channelNo);
	if (channel.currentNote == note) {
		channel.currentNote = Sci1Sound::Channel::kNoCurrentNote;
	}

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		_driver->noteOff(hwChannelNo & 0xf, note, velocity);
	}
}

void SoundManager::processNoteOn(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	if (sound.peek(trackNo, 1) == 0) {
		return processNoteOff(sound, trackNo, hwChannelNo);
	}

	const uint8 note = sound.consume(trackNo);
	const uint8 velocity = sound.consume(trackNo);

	sound.getChannel(sound.getTrack(trackNo).channelNo).currentNote = note;

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		_driver->noteOn(hwChannelNo & 0xf, note, velocity);
	}
}

void SoundManager::processKeyPressure(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	const uint8 note = sound.consume(trackNo);
	const uint8 pressure = sound.consume(trackNo);

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		// SSCI did not clamp the channel range here, see if this is a problem
		if (hwChannelNo >= kNumHardwareChannels) {
			warning("Key pressure note %u pressure %u channel %u out of range", note, pressure, hwChannelNo);
		}

		_driver->keyPressure(hwChannelNo, note, pressure);
	}
}

void SoundManager::processControllerChange(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool isExtraChannel) {
	const uint8 controllerNo = sound.consume(trackNo);
	uint8 value = sound.consume(trackNo);

	const uint8 inRangeChannelNo = hwChannelNo & 0xf;

	if (isExtraChannel && _hardwareChannels[inRangeChannelNo].isMapped()) {
		return;
	}

	Sci1Sound::Channel &channel = sound.getChannel(sound.getTrack(trackNo).channelNo);

	switch (controllerNo) {
	case kVolumeController:
		if (hwChannelNo >= kNumHardwareChannels * 2) {
			return;
		}

		// There appears to be a masking error in SSCI where out of range
		// channels would write out of bounds into the save mute flags (they
		// used 0xff instead of 0xf); we do not do that, this might cause volume
		// issues since any pending volumes would have been applied again in
		// SSCI and won't here.
		if (hwChannelNo >= kNumHardwareChannels) {
			warning("Out of range volume change applied to channel %u", hwChannelNo);
		}

		_newChannelVolumes[inRangeChannelNo] = kNoVolumeChange;

		channel.volume = value;
		value = value * sound.volume / Sci1Sound::kMaxVolume;

		break;
	case kPanController:
		channel.pan = value;
		break;
	case kModulationController:
		channel.modulation = value;
		break;
	case kDamperPedalController:
		channel.damperPedalOn = (value != 0);
		break;
	case kMaxVoicesController:
		_needsUpdate = true;
		channel.numVoices = value & 0xf;
		break;
	case kMuteController:
		channel.muted = (value != 0);
		if (channel.muted) {
			value = 1;
		}
		break;
	}

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		_driver->controllerChange(inRangeChannelNo, controllerNo, value);
	}
}

void SoundManager::processProgramChange(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool isExtraChannel) {
	const uint8 programNo = sound.consume(trackNo);
	const uint8 inRangeChannelNo = hwChannelNo & 0xf;

	if (isExtraChannel && _hardwareChannels[inRangeChannelNo].isMapped()) {
		return;
	}

	sound.getChannel(sound.getTrack(trackNo).channelNo).program = programNo;

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		_driver->programChange(inRangeChannelNo, programNo);
	}
}

void SoundManager::processChannelPressure(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	const uint8 pressure = sound.consume(trackNo);

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		// SSCI did not clamp the channel range here, see if this is a problem
		if (hwChannelNo >= kNumHardwareChannels) {
			warning("Channel pressure pressure %u channel %u out of range", pressure, hwChannelNo);
		}

		_driver->channelPressure(hwChannelNo, pressure);
	}
}

void SoundManager::processPitchBend(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool isExtraChannel) {
	const uint8 lsb = sound.consume(trackNo);
	const uint8 msb = sound.consume(trackNo);

	const uint8 inRangeChannelNo = hwChannelNo & 0xf;

	if (isExtraChannel && _hardwareChannels[inRangeChannelNo].isMapped()) {
		return;
	}

	const uint16 value = convert7To16(lsb, msb);
	sound.getChannel(sound.getTrack(trackNo).channelNo).pitchBend = value;

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		_driver->pitchBend(inRangeChannelNo, value);
	}
}

void SoundManager::processSysEx(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	skipCommand(sound, trackNo, kSysEx);
}

void SoundManager::skipCommand(Sci1Sound &sound, const uint8 trackNo, const MidiMessageType command) {
	switch (command) {
	case kSysEx:
		while (sound.consume(trackNo) != kEndOfSysEx);
		break;
	default:
		sound.advance(trackNo);
		// fall through
	case kProgramChange:
	case kChannelPressure:
		sound.advance(trackNo);
		break;
	}
}

#pragma mark -
#pragma mark Playlist management

uint8 SoundManager::insertSoundToPlaylist(Sci1Sound &sound) {
	int insertAt;
	for (insertAt = 0; insertAt < kPlaylistSize; ++insertAt) {
		if (!_playlist[insertAt] ||
			sound.priority <= _playlist[insertAt]->priority) {

			break;
		}
	}

	if (insertAt == kPlaylistSize) {
		return insertAt;
	}

	for (int i = kPlaylistSize - 2; i >= insertAt; --i) {
		_playlist[i + 1] = _playlist[i];
	}
	_playlist[insertAt] = &sound;
	return insertAt;
}

void SoundManager::removeSoundFromPlaylist(Sci1Sound &sound) {
	for (int i = 0; i < kPlaylistSize; ++i) {
		if (_playlist[i] == &sound) {
			sound.signal = Sci1Sound::kFinished;
			sound.state = Sci1Sound::kStopped;
			for (; i < kPlaylistSize - 1; ++i) {
				_playlist[i] = _playlist[i + 1];
			}
			_playlist[i] = nullptr;
			break;
		}
	}
}

#pragma mark -
#pragma mark Channel remapping

void SoundManager::updateChannelList() {
	_needsUpdate = false;

	const HardwareChannels oldChannels(_hardwareChannels);
	Common::fill(_hardwareChannels.begin(), _hardwareChannels.end(), HardwareChannel());

	if (_playlist[0]) {
		uint8 minChannelNo, maxChannelNo;
		_driver->getRemapRange(minChannelNo, maxChannelNo);

		uint8 reverbMode = _playlist[0]->reverbMode;
		if (reverbMode == kUseDefaultReverb) {
			reverbMode = _defaultReverbMode;
		}
		_driver->setReverbMode(reverbMode);

		// loopDoNodes
		HardwareChannels newChannels = makeChannelMap(minChannelNo, maxChannelNo);

		// doPass2
		commitFixedChannels(newChannels, oldChannels, minChannelNo, maxChannelNo);

		// doPass3
		commitDynamicChannels(newChannels, oldChannels, minChannelNo, maxChannelNo);
	}

	// cleanupChnls
	stopOldChannels(_hardwareChannels, oldChannels);

	// In SSCI the old channel list was persisted here; we do not need to do
	// this since we just use the stack for this and record the state at the
	// start of each update

	// In SSCI the channel sound pointers were updated here; we merged this into
	// the makeChannelMap pass
}

SoundManager::HardwareChannels SoundManager::makeChannelMap(const uint8 minChannelNo, const uint8 maxChannelNo) const {
	HardwareChannels committedChannels(_hardwareChannels);
	int committedFreeVoices = _driver->getNumVoices();
	int basePriority = 0;
	// loopDoNodes
	for (int i = 0; i < kPlaylistSize; ++i, basePriority += 16) {
		if (!_playlist[i]) {
			// jmp doPass2
			break;
		}

		const Sci1Sound &sound = *_playlist[i];

		if (sound.paused || sound.isSample) {
			// jmp nextNode
			continue;
		}

		HardwareChannels workingChannels(committedChannels);
		int workingFreeVoices(committedFreeVoices);

		// loopDoTracks
		for (int trackNo = 0; trackNo < Sci1Sound::kNumTracks; ++trackNo) {
			const Sci1Sound::Track &track = sound.getTrack(trackNo);
			if (track.channelNo == Sci1Sound::Track::kEndOfData ||
				track.channelNo == Sci1Sound::Track::kSampleTrack ||
				track.channelNo == kControlChannel) {

				// jmp nextChTrack
				continue;
			}

			const Sci1Sound::Channel &channel = sound.getChannel(track.channelNo);

			if ((channel.flags & Sci1Sound::Channel::kExtra) || channel.muted) {
				// jmp nextChTrack
				continue;
			}

			// notMutedChnl
			const uint8 key = (i << 4) | track.channelNo;

			uint8 priority = channel.priority;
			if (priority != 0) {
				priority = kNumHardwareChannels - priority + basePriority;
			}

			// nonPreemptable
			if (mapSingleChannel(key, priority, workingFreeVoices, track.channelNo, channel, workingChannels, minChannelNo, maxChannelNo)) {
				committedChannels = workingChannels;
				committedFreeVoices = workingFreeVoices;
			}

			// blewIt
			// nextNode
		}
	}

	return committedChannels;
}

bool SoundManager::mapSingleChannel(const uint8 key, const uint8 priority, int &numFreeVoices, const uint8 inChannelNo, const Sci1Sound::Channel &channel, HardwareChannels &newChannels, const uint8 minChannelNo, const uint8 maxChannelNo) const {
	uint8 bestHwChannelNo; // dh

	// nonPreemptable
	if (!(channel.flags & Sci1Sound::Channel::kLocked) || newChannels[inChannelNo].isMapped()) {
		// lookOpenChnl
		bestHwChannelNo = HardwareChannel::kUnmapped;
		for (int hwChannelNo = 0; hwChannelNo < kNumHardwareChannels; ++hwChannelNo) {
			const HardwareChannel &newHwChannel = newChannels[hwChannelNo];
			if (newHwChannel.key == key) {
				// jmp nextChTrack
				return true;
			} else if (!newHwChannel.isMapped() &&
				hwChannelNo >= minChannelNo && hwChannelNo <= maxChannelNo) {

				bestHwChannelNo = hwChannelNo;
				// continue to find the last free channel
				// TODO: can we not just do this by iterating in reverse?
			}
		}

		// nextLookChnl
		if (bestHwChannelNo == HardwareChannel::kUnmapped) {
			if (priority != 0) {
				// jmp nextNode, which is equivalent to jmp blewIt at this point
				return false;
			}

			// gotToGetChnl
			bestHwChannelNo = preemptChannel(newChannels, numFreeVoices);
			if (bestHwChannelNo == HardwareChannel::kUnmapped) {
				// jmp blewIt
				return false;
			}
		}
		// jmp checkVoices
	} else {
		bestHwChannelNo = inChannelNo;
		// jmp checkVoices
	}

	// checkVoices
	if (channel.numVoices <= numFreeVoices) {
		// jmp putChOnList
	} else if (priority == 0) {
		// loopPreEmpt
		do {
			bestHwChannelNo = preemptChannel(newChannels, numFreeVoices);
		} while (bestHwChannelNo != HardwareChannel::kUnmapped && channel.numVoices <= numFreeVoices);

		if (bestHwChannelNo == HardwareChannel::kUnmapped) {
			// jmp blewIt
			return false;
		}
	} else {
		// jmp nextChTrack
		return true;
	}

	// putChOnList
	assert(bestHwChannelNo < kNumHardwareChannels);

	HardwareChannel &newHwChannel = newChannels[bestHwChannelNo];
	newHwChannel.key = key;
	newHwChannel.numVoices = channel.numVoices;
	newHwChannel.priority = priority;
	// This assignment here replaces the loopChNodes cleanup pass
	newHwChannel.sound = _playlist[newHwChannel.playlistIndex()];
	numFreeVoices -= channel.numVoices;

	if (channel.flags & Sci1Sound::Channel::kLocked) {
		// checkRightChnl
		newHwChannel.locked = true;
		if (bestHwChannelNo == inChannelNo) {
			// jmp nextChTrack
			return true;
		}

		HardwareChannel &preferredChannel = newChannels[inChannelNo];

		// notRightChnl
		if (preferredChannel.locked) {
			// whichBedWins
			if (priority == 0) {
				// checkOtherBed
				if (preferredChannel.priority == 0) {
					// jmp blewIt
					return false;
				} else {
					// preemptBed
					numFreeVoices += preferredChannel.numVoices;
					preferredChannel = newHwChannel;
					newHwChannel = HardwareChannel();
					// TODO: We already subtracted our voices once, is this an
					// original engine bug?
					numFreeVoices -= channel.numVoices;
					// fall through to nextChTrack
					return true;
				}
			} else {
				newHwChannel = HardwareChannel();
				numFreeVoices += channel.numVoices;
				// jmp nextChTrack
				return true;
			}
		} else {
			SWAP(newChannels[inChannelNo], newChannels[bestHwChannelNo]);
			// jmp nextChTrack
			return true;
		}
	} else {
		// jmp nextChTrack
		return true;
	}
}

void SoundManager::commitFixedChannels(HardwareChannels &newChannels, const HardwareChannels &oldChannels, const uint8 minChannelNo, const uint8 maxChannelNo) {
	// loopPass2
	for (int newChannelNo = 0; newChannelNo < kNumHardwareChannels; ++newChannelNo) {
		HardwareChannel &newChannel = newChannels[newChannelNo];
		if (!newChannel.isMapped()) {
			// jmp nextPass2
			continue;
		}

		// notEmptyChNew
		assert(newChannel.sound);

		if (newChannel.locked) {
			// copyBedCh
			const HardwareChannel &oldChannel = oldChannels[newChannelNo];
			_hardwareChannels[newChannelNo] = newChannel;

			// SSCI32 did some thing here copying priority information from the
			// old list of channels, but priority is never used after the first
			// pass so this operation was useless and is omitted

			if (oldChannel.playlistIndex() != newChannel.playlistIndex() ||
				oldChannel.sound != newChannel.sound) {
				// The sound changed, either because a sound changed or
				// because the sounds were reordered in the playlist

				// notSameBed
				sendChannelToDriver(*newChannel.sound, newChannel.sound->getChannel(newChannel.channelNo()), newChannelNo);
			}
			// jmp nextPass2

			// This erasure of information from the new channel list occurred
			// immediately after assigning to _hardwareChannels in SSCI; since
			// we want to keep referencing this data for a bit for clarity, we
			// defer resetting it until the end of this block
			newChannel.key = HardwareChannel::kUnmapped;
		} else {
			// noCopyBedCh
			for (int outChannelNo = minChannelNo; outChannelNo <= maxChannelNo; ++outChannelNo) {
				const HardwareChannel &oldChannel = oldChannels[outChannelNo];

				// loopSameNode
				if (oldChannel.sound == newChannel.sound &&
					oldChannel.channelNo() == newChannel.channelNo() &&
					!newChannels[outChannelNo].locked) {
					// sameNodeCh
					_hardwareChannels[outChannelNo] = newChannel;
					newChannel.key = HardwareChannel::kUnmapped;
				}
				// jmp nextPass2
			}
		}
	}
}

void SoundManager::commitDynamicChannels(const HardwareChannels &newChannels, const HardwareChannels &oldChannels, const uint8 minChannelNo, const uint8 maxChannelNo) {
	// doPass3
	for (int i = 0; i < kNumHardwareChannels; ++i) {
		// loopPass3
		const HardwareChannel &newChannel = newChannels[i];
		if (newChannel.key == HardwareChannel::kUnmapped) {
			// jmp nextPass3
			continue;
		}

		// findLastOpen
		uint8 lastOpenChannelNo = HardwareChannel::kUnmapped;
		for (int j = maxChannelNo; j >= minChannelNo; --j) {
			if (!_hardwareChannels[j].isMapped()) {
				lastOpenChannelNo = j;
				break;
			}
		}
		assert(lastOpenChannelNo != HardwareChannel::kUnmapped);

		_hardwareChannels[lastOpenChannelNo] = newChannel;
		sendChannelToDriver(*newChannel.sound, newChannel.sound->getChannel(newChannel.channelNo()), lastOpenChannelNo);
	}
}

void SoundManager::stopOldChannels(const HardwareChannels &newChannels, const HardwareChannels &oldChannels) {
	for (int i = kNumHardwareChannels - 1; i >= 0; --i) {
		if (oldChannels[i].isMapped() && !newChannels[i].isMapped()) {
			_driver->controllerChange(i, kDamperPedalController, 0);
			_driver->controllerChange(i, kAllNotesOffController, 0);
			_driver->controllerChange(i, kMaxVoicesController, 0);
		}
	}
}

uint8 SoundManager::preemptChannel(HardwareChannels &newChannels, int &numFreeVoices) const {
	uint8 lowestPriority = 0;
	uint8 lowestPriorityChannelNo = HardwareChannel::kUnmapped;
	for (int i = 0; i < kNumHardwareChannels; ++i) {
		HardwareChannel &newChannel = newChannels[i];
		// This is a little confusing because lower priorities are higher
		// numerically
		if (lowestPriority < newChannel.priority) {
			lowestPriority = newChannel.priority;
			lowestPriorityChannelNo = i;
		}
	}

	if (lowestPriorityChannelNo != HardwareChannel::kUnmapped) {
		numFreeVoices += newChannels[lowestPriorityChannelNo].numVoices;
		newChannels[lowestPriorityChannelNo] = HardwareChannel();
	}

	return lowestPriorityChannelNo;
}

void SoundManager::sendChannelToDriver(const Sci1Sound &sound, const Sci1Sound::Channel &channel, const uint8 hwChannelNo) {
	Sci1SoundDriver &driver = static_cast<Sci1SoundDriver &>(*_driver);
	driver.controllerChange(hwChannelNo, kAllNotesOffController, 0);
	driver.controllerChange(hwChannelNo, kMaxVoicesController, channel.numVoices);
	driver.programChange(hwChannelNo, channel.program);

	_newChannelVolumes[hwChannelNo] = kNoVolumeChange;
	driver.controllerChange(hwChannelNo, kVolumeController, channel.volume * sound.volume / Sci1Sound::kMaxVolume);

	driver.controllerChange(hwChannelNo, kPanController, channel.pan);
	driver.controllerChange(hwChannelNo, kModulationController, channel.modulation);
	driver.controllerChange(hwChannelNo, kDamperPedalController, channel.damperPedalOn ? 127 : 0);
	driver.pitchBend(hwChannelNo, channel.pitchBend);

	// TODO: Unclear what this is for, check drivers for information
	driver.controllerChange(hwChannelNo, kMuteController, channel.currentNote);
}

#pragma mark -
#pragma mark Kernel

void SoundManager::kernelInit(const reg_t soundObj) {
	Common::StackLock lock(_mutex);

	const GuiResourceId resourceNo = readSelectorValue(_segMan, soundObj, SELECTOR(number));

	// TODO: This is how the old ScummVM SCI MIDI code handled generating a
	// nodePtr, and, this is a pretty bad way to do it, since it means that if
	// a cloned object is passed into init, its nodePtr value gets copied to a
	// new object, the old cloned object gets destroyed and then its reg_t is
	// reused to init a new sound, we might end up with two Sci1Sound objects
	// with the same key even though they should be distinct. This may never
	// happen, in which case this ID reuse makes things a bit simpler for us,
	// but if it does, this warning will let us know so we work harder at not
	// blowing up.
	if (soundObj.getSegment() == _segMan->findSegmentByType(SEG_TYPE_CLONES)) {
		warning("Sound is initialised from a clone");
	}

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));
	Sci1Sound *sound;
	if (nodePtr.isNull()) {
		_sounds.push_back(Sci1Sound(soundObj));
		sound = &_sounds.back();
		writeSelector(_segMan, soundObj, SELECTOR(nodePtr), soundObj);
	} else {
		kernelStop(nodePtr);
		sound = findSoundByRegT(nodePtr);
		assert(sound != nullptr);
	}

	sound->isSample = (getSoundResourceType(resourceNo) == kResourceTypeAudio);
	if (!sound->isSample) {
		sound->loop = (readSelectorValue(_segMan, soundObj, SELECTOR(loop)) == 0xffff);
		sound->priority = readSelectorValue(_segMan, soundObj, SELECTOR(priority));
		sound->signal = Sci1Sound::kNoSignal;
		sound->cue = 0;
		sound->volume = readSelectorValue(_segMan, soundObj, SELECTOR(vol));
	}
}

void SoundManager::kernelDispose(const reg_t soundObj) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));
	kernelStop(nodePtr);
	if (!nodePtr.isNull()) {
		Sci1Sound *it = findSoundByRegT(nodePtr);
		if (it) {
			_sounds.erase(it);
		}
	}
	writeSelector(_segMan, soundObj, SELECTOR(nodePtr), NULL_REG);
}

void SoundManager::kernelPlay(const reg_t soundObj, const bool exclusive) {
	Common::StackLock lock(_mutex);

	reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));
	if (nodePtr.isNull()) {
		kernelInit(soundObj);
		nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));
	}

	Sci1Sound *sound = findSoundByRegT(nodePtr);

	if (!sound) {
		writeSelectorValue(_segMan, soundObj, SELECTOR(signal), Kernel::kFinished);
		return;
	}

#ifdef ENABLE_SCI32
	// TODO: Figure out the exact SCI versions which did this
	if (_soundVersion >= SCI_VERSION_2 &&
		sound->resource &&
		sound->resource->getType() == kResourceTypeAudio) {

		g_sci->_audio32->stop(sound->id);
	}
#endif

	const GuiResourceId soundNo = getSoundResourceId(readSelectorValue(_segMan, soundObj, SELECTOR(number)));

	sound->id = ResourceId(getSoundResourceType(soundNo), soundNo);

	if (!readSelector(_segMan, soundObj, SELECTOR(handle)).isNull() &&
		(_soundVersion < SCI_VERSION_2 || !sound->isSample)) {

		kernelStop(soundObj);
	} else if (_soundVersion >= SCI_VERSION_2 && sound->isSample) {
		// SSCI did not store the resource pointer on the sound object in the
		// same way we do, so did not do this; this is only necessary in order
		// to avoid triggering the resource assertion below
		sound->resource = nullptr;
	}

	sound->isSample = (sound->id.getType() == kResourceTypeAudio);

	assert(!sound->resource);
	if (sound->isSample) {
		// SSCI32 would optionally preload audio if there was a preload flag in
		// the soundObj's `flags` selector; we do not need to worry about load
		// times, so we just don't do that
		sound->resource = _resMan.testResource(sound->id);
	} else {
		sound->resource = _resMan.findResource(sound->id, true);
	}

	if (!sound->resource) {
		writeSelectorValue(_segMan, soundObj, SELECTOR(signal), Kernel::kFinished);
		return;
	}

	// In SSCI the handle was assigned to the MemID returned by a call to
	// ResourceManager::Get, we do not allocate memory through SegManager for
	// resources so instead we just give the handle property a valid pointer
	writeSelector(_segMan, soundObj, SELECTOR(handle), soundObj);

	writeSelectorValue(_segMan, soundObj, SELECTOR(signal), Kernel::kNoSignal);
	writeSelectorValue(_segMan, soundObj, SELECTOR(min), 0);
	writeSelectorValue(_segMan, soundObj, SELECTOR(sec), 0);
	writeSelectorValue(_segMan, soundObj, SELECTOR(frame), 0);

	const bool loop = (readSelectorValue(_segMan, soundObj, SELECTOR(loop)) == 0xffff);
	const int16 volume = readSelectorValue(_segMan, soundObj, SELECTOR(vol));

	if (_soundVersion < SCI_VERSION_2 || !sound->isSample) {
		sound->priority = readSelectorValue(_segMan, soundObj, SELECTOR(priority));
		sound->volume = volume;
		sound->loop = loop;
	}

	if (sound->isSample) {
		// SSCI set up fake VM arguments and made direct kernel calls here,
		// which is not very pleasant; we do normal calls into the audio
		// components instead
#ifdef ENABLE_SCI32
		if (_soundVersion >= SCI_VERSION_2) {
			g_sci->_audio32->play(g_sci->_audio32->findChannelById(sound->id, soundObj), sound->id, true, loop, volume, nodePtr, false);
		} else
#endif
			// TODO: This doesn't loop, it should loop, Sci::Audio does not have
			// the capability to do this, it should probably just be using
			// Audio32 which was actually RE'd properly
			g_sci->_audio->startAudio(kSfxModule, sound->id.getNumber());
	} else {
		kernelUpdate(soundObj);
		play(*sound, exclusive);

		writeSelectorValue(_segMan, soundObj, SELECTOR(priority), sound->priority);
	}
}

void SoundManager::kernelStop(const reg_t soundObj) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));
	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (sound) {
		if (sound->isSample) {
			// SSCI set up fake VM arguments and made direct kernel calls here,
			// which is not very pleasant; we do normal calls into the audio
			// components instead
#ifdef ENABLE_SCI32
			if (_soundVersion >= SCI_VERSION_2) {
				g_sci->_audio32->stop(sound->id, soundObj);
			} else
				// TODO: This should be accepting a sound number.
				g_sci->_audio->stopAudio();
#endif
			sound->resource = nullptr;
		} else {
			stop(*sound);
			// A sound may be stopped before it is ever started, in which case
			// a resource won't exist yet, and so we do not need to unlock
			// anything
			if (sound->resource) {
				_resMan.unlockResource(sound->resource);
				// Don't try to unlock a resource more than once
				sound->resource = nullptr;
			}
		}
	}

	writeSelector(_segMan, soundObj, SELECTOR(handle), NULL_REG);
	writeSelectorValue(_segMan, soundObj, SELECTOR(signal), Kernel::kFinished);
}

void SoundManager::kernelPause(const reg_t soundObj, const bool shouldPause, const bool pauseDac) {
	Common::StackLock lock(_mutex);

	if (soundObj.isNull()) {
		pauseAll(shouldPause);
#ifdef ENABLE_SCI32
		// SSCI set up fake VM arguments and made direct kernel calls here,
		// which is not very pleasant; we do normal calls into the audio
		// components instead
		if (pauseDac && _soundVersion >= SCI_VERSION_2) {
			if (shouldPause) {
				g_sci->_audio32->pause(kAllChannels);
			} else {
				g_sci->_audio32->resume(kAllChannels);
			}
		}
#endif
	} else {
		const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));
		Sci1Sound *sound = findSoundByRegT(nodePtr);
		if (sound) {
#ifdef ENABLE_SCI32
			// SSCI set up fake VM arguments and made direct kernel calls here,
			// which is not very pleasant; we do normal calls into the audio
			// components instead
			if (pauseDac && _soundVersion >= SCI_VERSION_2 && sound->isSample) {
				if (shouldPause) {
					g_sci->_audio32->pause(sound->id, nodePtr);
				} else {
					g_sci->_audio32->resume(sound->id, nodePtr);
				}
			} else
#endif
				pause(*sound, shouldPause);
		}
	}
}

void SoundManager::kernelFade(const reg_t soundObj, const int16 targetVolume, const int16 speed, const int16 steps, const bool stopAfterFade) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));

	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (!sound) {
		return;
	}

#ifdef ENABLE_SCI32
	if (_soundVersion >= SCI_VERSION_2 && sound->isSample) {
		g_sci->_audio32->fadeChannel(sound->id, nodePtr,
									 targetVolume, speed, steps, stopAfterFade);
	} else
#endif
		fade(*sound, targetVolume, speed, steps, stopAfterFade);
}

void SoundManager::kernelHold(const reg_t soundObj, const int16 holdPoint) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));
	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (sound) {
		hold(*sound, holdPoint);
	}
}

void SoundManager::kernelSetVolume(const reg_t soundObj, const int16 volume) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));
	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (!sound) {
		return;
	}

#ifdef ENABLE_SCI32
	if (_soundVersion >= SCI_VERSION_2 && sound->isSample) {
		g_sci->_audio32->setVolume(sound->id, nodePtr, volume);
	}
#endif
	if (sound->volume != volume) {
		setVolume(*sound, volume);
		writeSelectorValue(_segMan, soundObj, SELECTOR(vol), volume);
		_guestAdditions.kDoSoundSetVolumeHook(soundObj, volume);
	}
}

void SoundManager::kernelSetPriority(const reg_t soundObj, const int16 priority) {
	enum {
		kFixedPriority = 2
	};

	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));

	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (!sound) {
		return;
	}

	uint16 flags = readSelectorValue(_segMan, soundObj, SELECTOR(flags));
	if (priority == -1) {
		sound->fixedPriority = false;
		flags &= ~kFixedPriority;
	} else {
		sound->fixedPriority = true;
		flags |= kFixedPriority;
		setPriority(*sound, priority);
	}
	writeSelectorValue(_segMan, soundObj, SELECTOR(flags), flags);
}

void SoundManager::kernelSetLoop(const reg_t soundObj, const bool enable) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));

	// SSCI normally did not set this selector until after the sound was found,
	// but if we do this then at least the torch SFX at the character select in
	// QFG4 does not loop, since the game sets loop before starting the sound
	writeSelectorValue(_segMan, soundObj, SELECTOR(loop), enable ? 0xffff : 1);

	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (!sound) {
		return;
	}

#ifdef ENABLE_SCI32
	if (_soundVersion >= SCI_VERSION_2 && sound->isSample) {
		g_sci->_audio32->setLoop(sound->id, nodePtr, enable);
	} else
#endif
		sound->loop = enable;
}

void SoundManager::kernelUpdateCues(const reg_t soundObj) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));

	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (!sound) {
		return;
	}

	if (sound->isSample) {
		int position;
#ifdef ENABLE_SCI32
		if (_soundVersion >= SCI_VERSION_2) {
			position = g_sci->_audio32->getPosition(sound->id, nodePtr);

			if (getSciVersion() == SCI_VERSION_3) {
				// In SSCI the volume is first set to -1 and then reset later if
				// a sample is playing in the audio player, but since our audio
				// code returns -1 for not-found samples, the extra check is not
				// needed and we can just always set it to the return value of
				// the getVolume call
				const int16 volume = g_sci->_audio32->getVolume(sound->id, nodePtr);
				writeSelectorValue(_segMan, soundObj, SELECTOR(vol), volume);
			}
		} else
#endif
			position = g_sci->_audio->getAudioPosition();

		if (position == -1) {
			kernelStop(soundObj);
		}
	} else {
		const Sci1Sound::Signal signal = consumeSignal(*sound);
		switch (signal) {
		case Sci1Sound::kFinished:
			kernelStop(soundObj);
			break;
		case Sci1Sound::kNoSignal:
			if (readSelectorValue(_segMan, soundObj, SELECTOR(dataInc)) != sound->cue) {
				writeSelectorValue(_segMan, soundObj, SELECTOR(dataInc), sound->cue);
				writeSelectorValue(_segMan, soundObj, SELECTOR(signal), sound->cue + 0x7f);
			}
			break;
		default:
			writeSelectorValue(_segMan, soundObj, SELECTOR(signal), signal);
		}

		const Position position(getPosition(*sound));
		writeSelectorValue(_segMan, soundObj, SELECTOR(min), position.minutes);
		writeSelectorValue(_segMan, soundObj, SELECTOR(sec), position.seconds);
		writeSelectorValue(_segMan, soundObj, SELECTOR(frame), position.frames);
		writeSelectorValue(_segMan, soundObj, SELECTOR(vol), sound->volume);
	}
}

void SoundManager::kernelSendMidi(const reg_t soundObj, int16 channel, const int16 command, int16 a, int16 b) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));

	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (!sound) {
		return;
	}

	--channel;

	if (command == kPitchBend) {
		a = CLIP<int16>(a, -0x2000, 0x1fff);
	} else {
		a = CLIP<int16>(a, 0, 127);
		b = CLIP<int16>(a, 0, 127);
	}

	switch (command) {
	case kNoteOff:
		setNoteOff(*sound, channel, a, b);
		break;
	case kNoteOn:
		setNoteOn(*sound, channel, a, b);
		break;
	case kControllerChange:
		setController(*sound, channel, a, b);
		break;
	case kProgramChange:
		setProgram(*sound, channel, a);
		break;
	case kPitchBend:
		setPitchBend(*sound, channel, a + 0x2000);
		break;
	}
}

void SoundManager::kernelUpdate(const reg_t soundObj) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));

	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (!sound || (_soundVersion >= SCI_VERSION_2 && sound->isSample)) {
		return;
	}

	sound->loop = (readSelectorValue(_segMan, soundObj, SELECTOR(loop)) == 0xffff);
	const int16 volume = readSelectorValue(_segMan, soundObj, SELECTOR(vol));
	if (sound->volume != volume) {
		setVolume(*sound, volume);
	}
	const int16 priority = readSelectorValue(_segMan, soundObj, SELECTOR(priority));
	if (sound->priority != priority) {
		setPriority(*sound, priority);
	}
}

#pragma mark -
#pragma mark Debugging

void SoundManager::debugPrintPlaylist(Console &con) const {
	Common::StackLock lock(_mutex);

	for (int i = 0; i < kPlaylistSize; ++i) {
		if (!_playlist[i]) {
			return;
		}

		const Sci1Sound &sound = *_playlist[i];

		const char *status;
		if (sound.state == Sci1Sound::kStopped) {
			status = "stopped";
		} else if (sound.paused) {
			status = "paused";
		} else {
			status = "playing";
		}

		con.debugPrintf("%d: %04x:%04x (%s), resource id: %s, status: %s\n",
						i,
						PRINT_REG(sound.nodePtr),
						_segMan->getObjectName(sound.nodePtr),
						sound.resource ? sound.id.toString().c_str() : "<none>",
						status);
	}
}

void SoundManager::debugPrintSound(Console &con, const reg_t nodePtr) const {
	Common::StackLock lock(_mutex);

	const Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (!sound) {
		con.debugPrintf("Sound not found in playlist");
		return;
	}

	con.debugPrintf("%s, %s, %u pauses\n",
					sound->id.toString().c_str(),
					sound->state == Sci1Sound::kStopped ? "stopped" : "playing",
					sound->paused);
	con.debugPrintf("cue %u, hold point %u, loop %d\n",
					sound->cue, sound->holdPoint, sound->loop);
	con.debugPrintf("signal %d, state %d, priority %d%s\n",
					sound->signal, sound->state, sound->priority,
					sound->fixedPriority ? " (fixed)" : "");
	con.debugPrintf("ticks elapsed %u, reverb mode %u, volume %d",
					sound->ticksElapsed, sound->reverbMode, sound->volume);
	if (sound->fadeAmountPerTick) {
		con.debugPrintf(" -> %d\n", sound->fadeTargetVolume);
		con.debugPrintf("fade delay %d, speed %d, stop %d\n",
						sound->fadeDelay, sound->fadeAmountPerTick, sound->stopSoundOnFade);
	} else {
		con.debugPrintf("\n");
	}

	con.debugPrintf("\nTracks:\n");

	for (int i = 0; i < Sci1Sound::kNumTracks; ++i) {
		const Sci1Sound::Track &track = sound->getTrack(i);
		if (track.offset == 0) {
			break;
		}
		con.debugPrintf("%2d: offset %u, position %u, channel %d\n",
						i, track.offset, track.position, track.channelNo);
		con.debugPrintf("    rest %u, command %u\n",
						track.rest, track.command);
		con.debugPrintf("    loop position %u, loop rest %u, loop command %u\n",
						track.loopPosition, track.loopRest, track.loopCommand);
	}

	con.debugPrintf("\nChannels:\n");

	for (int i = 0; i < Sci1Sound::kNumChannels; ++i) {
		const Sci1Sound::Channel &channel = sound->getChannel(i);
		con.debugPrintf("%2d: priority %d, voices %u, note %u, volume %u\n",
						i, channel.priority, channel.numVoices, channel.currentNote, channel.volume);
		con.debugPrintf("    program %u, mod %u, pan %u, p bend %u\n",
						channel.program, channel.modulation, channel.pan, channel.pitchBend);
		con.debugPrintf("    dp %d, flags %u, mute %d, game mutes %u\n",
						channel.damperPedalOn, channel.flags, channel.muted, channel.gameMuteCount);
	}
}

void SoundManager::debugPrintChannelMap(Console &con) const {
	debugPrintChannelMap(con, _hardwareChannels);
}

void SoundManager::debugPrintChannelMap(Console &con, const HardwareChannels &channels) const {
	for (int i = 0; i < kNumHardwareChannels; ++i) {
		const HardwareChannel &channel = _hardwareChannels[i];
		if (channel.isMapped()) {
			con.debugPrintf("%2d: %s ch %2d pr %3d vo %2d%s\n",
							i,
							_playlist[channel.playlistIndex()]->id.toString().c_str(),
							channel.channelNo(),
							channel.priority,
							channel.numVoices,
							channel.locked ? ", locked" : "");
		} else {
			con.debugPrintf("%2d: unmapped\n", i);
		}
	}
}

void SoundManager::debugPrintDriverState(Console &con) const {
	_driver->debugPrintState(con);
}

void SoundManager::debugPlay(const GuiResourceId soundId) {
	warning("TODO: Not implemented yet");
}

void SoundManager::debugStopAll() {
	for (int i = 0; i < kPlaylistSize; ++i) {
		if (!_playlist[i]) {
			return;
		}

		stop(*_playlist[i]);
	}
}

} // end of namespace Sci
