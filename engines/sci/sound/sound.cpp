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
#include "sci/engine/kernel.h"
#include "sci/engine/seg_manager.h"
#include "sci/engine/selector.h"
#include "sci/sound/audio.h"
#ifdef ENABLE_SCI32
#include "sci/sound/audio32.h"
#endif
#include "sci/sound/sound.h"
#include "sci/sound/drivers/adlib.h"

namespace Sci {

/**
 * Converts a 16-bit number into two MIDI bytes.
 */
static inline void convert16To7(const uint16 word, byte &lsb, byte &msb) {
	msb = (word >> 7) & 0x7e;
	lsb = word & 0x7f;
	if (word & 0x80) {
		msb |= 1;
	}
}

static inline uint16 convert7To16(byte lsb, byte msb) {
	return (msb << 7) | lsb;
}

SoundManager::SoundManager(ResourceManager &resMan, SegManager &segMan, GameFeatures &features) :
	_resMan(resMan),
	_segMan(&segMan),
	_soundVersion(features.detectDoSoundType()),
	_restoringSound(false),
	_numServerSuspensions(0),
	_needsUpdate(false),
	_newChannelVolumes(kNumHardwareChannels, kNoVolumeChange),
	_nextVolumeChangeChannel(0),
	_defaultReverbMode(0),
	_sample(nullptr),
	_playlist(),
	_hardwareChannels(kNumHardwareChannels),
	_mappedNodes(kNumHardwareChannels),
	_channelList(kNumHardwareChannels, HardwareChannel::kUnmapped) {
	_preferSampledSounds = _soundVersion >= SCI_VERSION_2 ||
		g_sci->getGameId() == GID_GK1DEMO ||
		ConfMan.getBool("prefer_digitalsfx");
	_sounds.reserve(16);

	uint32 deviceFlags;
#ifdef ENABLE_SCI32
	if (features.generalMidiOnly()) {
		deviceFlags = MDT_MIDI;
	} else {
#endif
		deviceFlags = MDT_PCSPK | MDT_PCJR | MDT_ADLIB | MDT_MIDI | MDT_CMS;
#ifdef ENABLE_SCI32
	}
#endif

	// Default to MIDI for Windows versions of SCI1.1 games, as their
	// soundtrack is written for GM.
	if (features.useAltWinGMSound())
		deviceFlags |= MDT_PREFER_GM;

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
	} else if (features.generalMidiOnly() && musicType != MT_GM) {
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
	default:
		if (ConfMan.getBool("native_fb01")) {
			error("TODO FB-01");
//			_driver.reset(makeFb01Driver(resMan, _soundVersion));
		} else {
			error("TODO General MIDI/MT-32 (shouldn't these be separate?)");
//			_driver.reset(makeMidiDriver(resMan, _soundVersion));
		}
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
						sound.resource ? sound.resource->getId().toString().c_str() : "<none>",
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
					sound->resource->getId().toString().c_str(),
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
		const Sci1Sound::Track &track = sound->tracks[i];
		con.debugPrintf("%2d: offset %u, position %u, channel %d\n",
						i, track.offset, track.position, track.channelNo);
		con.debugPrintf("    rest %u, command %u\n",
						track.rest, track.command);
		con.debugPrintf("    loop position %u, loop rest %u, loop command %u\n",
						track.loopPosition, track.loopRest, track.loopCommand);
	}

	con.debugPrintf("\nChannels:\n");

	for (int i = 0; i < Sci1Sound::kNumChannels; ++i) {
		const Sci1Sound::Channel &channel = sound->channels[i];
		con.debugPrintf("%2d: priority %d, voices %u, note %u, volume %u\n",
						i, channel.priority, channel.numVoices, channel.currentNote, channel.volume);
		con.debugPrintf("    program %u, mod %u, pan %u, p bend %u\n",
						channel.program, channel.modulation, channel.pan, channel.pitchBend);
		con.debugPrintf("    dp %d, flags %u, mute %d, game mutes %u\n",
						channel.damperPedalOn, channel.flags, channel.muted, channel.gameMuteCount);
	}
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
		muteCounts[i] = sound.channels[i].gameMuteCount;
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
		sound.channels[i].gameMuteCount = muteCounts[i];
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

	for (int i = 0; i < kPlaylistSize; ++i) {
		if (!_playlist[i]) {
			// End of playlist
			break;
		}

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
		Sci1Sound::Channel &channel = sound.channels[channelNo];
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
		if (_channelList[hwChannelNo] != HardwareChannel::kUnmapped && (_channelList[hwChannelNo] & 0xf0) == playlistIndex) {
			const uint8 channelNo = _channelList[hwChannelNo] & 0xf;
			changeChannelVolume(sound, channelNo, hwChannelNo, enqueue);
		}
	}

	for (int trackNo = 0; trackNo < Sci1Sound::kNumTracks; ++trackNo) {
		const uint8 channelNo = sound.tracks[trackNo].channelNo;
		if (channelNo == Sci1Sound::Track::kEndOfData) {
			break;
		}

		const Sci1Sound::Channel &channel = sound.channels[channelNo];
		if ((channel.flags & Sci1Sound::Channel::kExtra) &&
			_channelList[channelNo] == HardwareChannel::kUnmapped) {
			changeChannelVolume(sound, channelNo, channelNo, enqueue);
		}
	}
}

void SoundManager::changeChannelVolume(const Sci1Sound &sound, const uint8 channelNo, const uint8 hwChannelNo, const bool enqueue) {
	const uint8 channelVolume = sound.channels[channelNo].volume * sound.volume / Sci1Sound::kMaxVolume;
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

	// TODO: Use proper initialisers in Sci1Sound?
	Common::fill(sound.channels, sound.channels + Sci1Sound::kNumChannels, Sci1Sound::Channel());
	Common::fill(sound.tracks, sound.tracks + Sci1Sound::kNumTracks, Sci1Sound::Track());
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

	findTrackOffsets(sound);

	for (int trackNo = 0; trackNo < Sci1Sound::kNumTracks; ++trackNo) {
		const uint16 trackOffset = sound.resource->getUint16LEAt(trackNo * sizeof(uint16));
		if (trackOffset == 0) {
			break;
		}
		SciSpan<const byte> trackData = sound.resource->subspan(trackOffset);
		Sci1Sound::Track &track = sound.tracks[trackNo];

		const uint8 channelNo = trackData[0];
		track.channelNo = channelNo & 0xf;
		track.command = kControllerChange | track.channelNo;
		const uint8 message = trackData[12];
		if (message == kTimingOverflow) {
			track.rest = kTimingOverflowValue;
		} else {
			track.rest = message;
		}

		// SSCI did not do a check for the control channel so early, instead
		// relied on the MIDI data not containing a bad channel number value
		// to not do out-of-bounds writes. We do the check so we can reference
		// the Channel object.
		if (track.channelNo != kControlChannel) {
			Sci1Sound::Channel &channel = sound.channels[track.channelNo];

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
			// SSCI checked whether reverbMode != kDefaultReverbMode and then
			// jumped to checking the channel pan, this was an impossible
			// condition and would have led to an out-of-bounds access so is
			// removed
			sound.reverbMode = trackData[8];
		}
	}

	if (sound.state & Sci1Sound::kExclusive) {
		for (int channelNo = 0; channelNo < Sci1Sound::kNumChannels; ++channelNo) {
			Sci1Sound::Channel &channel = sound.channels[channelNo];
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

	sound.channels[channelNo].currentNote = Sci1Sound::Channel::kNoCurrentNote;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != kNumHardwareChannels) {
		_driver->noteOff(hwChannelNo, note, velocity);
	}
}

void SoundManager::setNoteOn(Sci1Sound &sound, const uint8 channelNo, const uint8 note, const uint8 velocity) {
	Common::StackLock lock(_mutex);

	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	sound.channels[channelNo].currentNote = note;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != kNumHardwareChannels) {
		_driver->noteOn(hwChannelNo, note, velocity);
	}
}

void SoundManager::setController(Sci1Sound &sound, const uint8 channelNo, const uint8 controllerNo, uint8 value) {
	Common::StackLock lock(_mutex);

	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	Sci1Sound::Channel &channel = sound.channels[channelNo];

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
	if (hwChannelNo != kNumHardwareChannels) {
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

	sound.channels[channelNo].program = programNo;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != kNumHardwareChannels) {
		_driver->programChange(hwChannelNo, programNo);
	}
}

void SoundManager::setPitchBend(Sci1Sound &sound, const uint8 channelNo, const uint16 value) {
	Common::StackLock lock(_mutex);

	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	sound.channels[channelNo].pitchBend = value;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != kNumHardwareChannels) {
		uint8 lsb, msb;
		convert16To7(value, lsb, msb);
		_driver->pitchBend(hwChannelNo, lsb, msb);
	}
}

#pragma mark -
#pragma mark Data processing

void SoundManager::findTrackOffsets(Sci1Sound &sound) {
	SciSpan<const byte> data = *sound.resource;
	if (data[0] == 0xf0) {
		data += 8;
	}

	for (uint8 deviceId = *data++; deviceId != 0xff; deviceId = *data++) {
		if (deviceId == _driver->getDeviceId()) {
			int trackNo = 0;
			for (uint8 endMarker = *data; endMarker != 0xff; endMarker = *data) {
				assert(trackNo < Sci1Sound::kNumTracks);
				// TODO: Could be SCI-endian
				sound.tracks[trackNo++].offset = data.getUint16LEAt(2);
				data += 6;
			}
			return;
		} else {
			while (*data != 0xff) {
				data += 6;
			}
		}
	}
}

void SoundManager::parseNextNode(Sci1Sound &sound, uint8 playlistIndex) {
	// TODO: Use wall time when _restoringSound is not true
	++sound.ticksElapsed;

	assert(sound.resource);
	SciSpan<const byte> base = *sound.resource;

	bool allTracksEnded = false;
	for (int trackNo = 0; trackNo < Sci1Sound::kNumTracks; ++trackNo) {
		Sci1Sound::Track &track = sound.tracks[trackNo];
		if (track.channelNo == Sci1Sound::Track::kEndOfData) {
			allTracksEnded = true;
			break;
		}

		if (track.channelNo == Sci1Sound::Track::kSampleTrack) {
			continue;
		}

		Sci1Sound::Channel &channel = sound.channels[track.channelNo];

		uint8 hwChannelNo = HardwareChannel::kUnmapped;
		bool extraChannel;
		if (channel.flags & Sci1Sound::Channel::kExtra) {
			extraChannel = true;
			hwChannelNo = track.channelNo;
		} else {
			extraChannel = false;
			const uint8 key = (playlistIndex << 4) | track.channelNo;
			for (uint i = 0; i < kNumHardwareChannels; ++i) {
				if (_channelList[i] != key) {
					continue;
				}

				hwChannelNo = i;
				break;
			}
		}

		// restorePtr
		// TODO: Move up to avoid unnecessary calculations
		if (track.position == 0) {
			continue;
		}

		// TODO: In SSCI rest check was after position calculation so it could use track data, since we abstract that maybe it is not needed to have this so low? Move up to avoid unnecessary calculations
		if (track.rest) {
			// TODO: Use wall time when _restoringSound is not true
			--track.rest;

			if (track.rest == kTimingOverflowFlag) {
				// The last rest was a "timing overflow" rest and now its timer
				// has ticked down to zero
				const byte message = sound.consume(trackNo);
				if (message == kTimingOverflow) {
					track.rest = kTimingOverflowValue;
				} else {
					track.rest = message;
				}
			}

			continue;
		}

		// TODO: parseCommand
		byte message;
		do {
			message = sound.peek(trackNo);
			if (message & kStartOfMessageFlag) {
				track.command = message;
				sound.advance(trackNo);
			} else {
				message = track.command;
			}

			// Command/channel splitting was moved down from here nearer to its
			// point of use

			if (message == kEndOfTrack) {
				track.position = 0;
				continue;
			}

			const MessageType command = MessageType(message & 0xf0);
			const uint8 channelNo = message & 0xf;

			if (channelNo == kControlChannel) {
				parseControlChannel(sound, trackNo, command);
				if (track.position == 0) {
					continue;
				}
			} else {
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
					goto nextTrack;
				}
			}
		} while ((message = sound.consume(trackNo)) == 0);

		if (message == kTimingOverflow) {
			track.rest = kTimingOverflowValue;
		} else {
			track.rest = message;
		}
		--track.rest;

		nextTrack:
		;
	}

	if (!allTracksEnded) {
		for (int i = 0; i < Sci1Sound::kNumTracks; ++i) {
			Sci1Sound::Track &track = sound.tracks[i];
			if (track.position != 0) {
				// At least one track is still running
				return;
			}
		}
	}

	if (sound.holdPoint || sound.loop) {
		sound.ticksElapsed = sound.loopTicksElapsed;
		for (int i = 0; i < Sci1Sound::kNumTracks; ++i) {
			Sci1Sound::Track &track = sound.tracks[i];
			track.position = track.loopPosition;
			track.rest = track.loopRest;
			track.command = track.loopCommand;
		}
	} else {
		removeSoundFromPlaylist(sound);
		_needsUpdate = true;
	}
}

void SoundManager::parseControlChannel(Sci1Sound &sound, const uint8 trackNo, const MessageType command) {
	Sci1Sound::Track &track = sound.tracks[trackNo];

	switch (command) {
	case kProgramChange: {
		const byte message = sound.consume(trackNo);
		if (message == kSetLoop) {
			const byte value = sound.peek(trackNo);
			if (value == kTimingOverflow) {
				track.rest = kTimingOverflowValue;
			} else {
				track.rest = value;
			}

			track.command = kProgramChange | kControlChannel;

			for (int i = 0; i < Sci1Sound::kNumTracks; ++i) {
				sound.tracks[i].loopPosition = sound.tracks[i].position;
				sound.tracks[i].loopRest = sound.tracks[i].rest;
				sound.tracks[i].loopCommand = sound.tracks[i].command;
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
		case kLoopEndController:
			if (sound.holdPoint == value) {
				for (int i = 0; i < Sci1Sound::kNumTracks; ++i) {
					sound.tracks[i].position = 0;
				}
			}
			break;
		}
	}
	default:
		return skipCommand(sound, trackNo, command);
	}
}

void SoundManager::processNoteOff(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	const uint8 note = sound.consume(trackNo);
	const uint8 velocity = sound.consume(trackNo);

	Sci1Sound::Channel &channel = sound.channels[sound.tracks[trackNo].channelNo];
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

	sound.channels[sound.tracks[trackNo].channelNo].currentNote = note;

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

	if (isExtraChannel && _channelList[inRangeChannelNo] != HardwareChannel::kUnmapped) {
		return;
	}

	Sci1Sound::Channel &channel = sound.channels[sound.tracks[trackNo].channelNo];

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

	if (isExtraChannel && _channelList[inRangeChannelNo] != HardwareChannel::kUnmapped) {
		return;
	}

	sound.channels[sound.tracks[trackNo].channelNo].program = programNo;

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

	if (isExtraChannel && _channelList[inRangeChannelNo] != HardwareChannel::kUnmapped) {
		return;
	}

	sound.channels[sound.tracks[trackNo].channelNo].pitchBend = convert7To16(lsb, msb);

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		_driver->pitchBend(inRangeChannelNo, lsb, msb);
	}
}

void SoundManager::processSysEx(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	skipCommand(sound, trackNo, kSysEx);
}

void SoundManager::skipCommand(Sci1Sound &sound, const uint8 trackNo, const MessageType command) {
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

	Common::fill(_channelList.begin(), _channelList.end(), HardwareChannel::kUnmapped);
	Common::fill(_hardwareChannels.begin(), _hardwareChannels.end(), HardwareChannel());

	// TODO: This is just plain broken in SSCI32, it fills word SampleList with
	// 0 and then word SampleList[2] with 0. That was OK for SCI16 only since it
	// stored segment & offset. What it is really meaning to do, in both
	// versions, is to clear out the pointer, so that is what we do here.
	_sample = nullptr;

	const ChannelList oldChannelList(_channelList);

	if (_playlist[0]) {
		uint8 minChannelNo, maxChannelNo;
		_driver->getValidChannelRange(minChannelNo, maxChannelNo);

		uint8 reverbMode = _playlist[0]->reverbMode;
		if (reverbMode == kUseDefaultReverb) {
			reverbMode = _defaultReverbMode;
		}
		_driver->setReverbMode(reverbMode);

		HardwareChannels oldMappedChannels;
		int numFreeVoices = _driver->getNumVoices();
		for (int i = 0; i < kPlaylistSize; ++i) {
			if (!_playlist[i]) {
				break;
			}

			Sci1Sound &sound = *_playlist[i];
			if (sound.paused) {
				continue;
			}

			if (sound.isSample) {
				if (!_sample) {
					_sample = &sound;
				}

				continue;
			}

			oldMappedChannels = _hardwareChannels;
			const int oldNumFreeVoices = numFreeVoices;

			// loopDoTracks:
			int basePriority = 0;
			for (int trackNo = 0; trackNo < Sci1Sound::kNumTracks; ++trackNo, basePriority += 16) {
				Sci1Sound::Track &track = sound.tracks[trackNo];
				if (track.channelNo == Sci1Sound::Track::kEndOfData ||
					track.channelNo == Sci1Sound::Track::kSampleTrack ||
					track.channelNo == kControlChannel) {

					continue;
				}

				Sci1Sound::Channel &channel = sound.channels[track.channelNo];

				if ((channel.flags & Sci1Sound::Channel::kExtra) || channel.muted) {
					continue;
				}

				const uint8 key = (i << 4) | track.channelNo;

				uint8 priority = channel.priority;
				if (priority != 0) {
					priority = kNumHardwareChannels - priority + basePriority;
				}

				uint8 bestRemapIndex;
				if ((channel.flags & Sci1Sound::Channel::kLocked) &&
					_hardwareChannels[track.channelNo].key == HardwareChannel::kUnmapped) {
					bestRemapIndex = track.channelNo;
				} else {
					bestRemapIndex = HardwareChannel::kUnmapped;
					for (int channelNo = 0; channelNo < kNumHardwareChannels; ++channelNo) {
						HardwareChannel &mappedChannel = _hardwareChannels[channelNo];
						if (mappedChannel.key == HardwareChannel::kUnmapped) {
							if (channelNo >= minChannelNo && channelNo <= maxChannelNo) {
								bestRemapIndex = channelNo;
							}
						} else if (mappedChannel.key == key) {
							goto nextTrack;
						}
					}

					if (bestRemapIndex == HardwareChannel::kUnmapped) {
						if (priority != 0) {
							goto nextSound;
						}

						bestRemapIndex = preemptChannel(numFreeVoices);
					}
				}

				if (bestRemapIndex == HardwareChannel::kUnmapped) {
					_hardwareChannels = oldMappedChannels;
					numFreeVoices = oldNumFreeVoices;
					goto nextSound;
				} else {
					// checkVoices
					if (channel.numVoices > numFreeVoices) {
						if (priority == 0) {
							do {
								bestRemapIndex = preemptChannel(numFreeVoices);
								if (bestRemapIndex == HardwareChannel::kUnmapped) {
									_hardwareChannels = oldMappedChannels;
									numFreeVoices = oldNumFreeVoices;
									goto nextSound;
								}
							} while (channel.numVoices > numFreeVoices);
						} else {
							goto nextTrack;
						}
					}

					// putChOnList
					assert(bestRemapIndex < kNumHardwareChannels);
					HardwareChannel &hwChannel = _hardwareChannels[bestRemapIndex];
					hwChannel.key = key;
					hwChannel.numVoices = channel.numVoices;
					numFreeVoices -= channel.numVoices;
					hwChannel.priority = priority;

					if (channel.flags & Sci1Sound::Channel::kLocked) {
						if (bestRemapIndex != track.channelNo) {
							if (_hardwareChannels[track.channelNo].locked) {
								if (priority == 0) {
									if (_hardwareChannels[track.channelNo].priority == 0) {
										_hardwareChannels = oldMappedChannels;
										numFreeVoices = oldNumFreeVoices;
										goto nextSound;
									} else {
										numFreeVoices += _hardwareChannels[track.channelNo].numVoices;
										_hardwareChannels[track.channelNo] = hwChannel;
										hwChannel = HardwareChannel();
										numFreeVoices -= channel.numVoices;
									}
								} else {
									hwChannel = HardwareChannel();
									numFreeVoices += channel.numVoices;
								}
							} else {
								SWAP(_hardwareChannels[track.channelNo], hwChannel);
							}
						}
					} else {
						hwChannel.locked = false;
					}
				}

				nextTrack:
				;
			}

			nextSound:
			;
		}

		// pass 2
		for (int channelNo = 0; channelNo < kNumHardwareChannels; ++channelNo) {
			HardwareChannel &hwChannel = _hardwareChannels[channelNo];
			if (hwChannel.key == HardwareChannel::kUnmapped) {
				continue;
			}

			Sci1Sound *sound = _playlist[hwChannel.playlistIndex()];
			assert(sound);

			if (hwChannel.locked) {
				// copyBedCh
				_channelList[channelNo] = hwChannel.key;
				hwChannel.key = HardwareChannel::kUnmapped;

#ifdef ENABLE_SCI32
				// TODO: Determine precise version, this exists in at least SQ6.
				if (_soundVersion >= SCI_VERSION_2) {
					hwChannel.priority = oldMappedChannels[channelNo].priority;
				}
#endif

				// SSCI stored the old channel list separately premasked so used
				// a direct equality comparison; we do not, so need to mask out
				// the playlist index here.
				if (hwChannel.channelNo() == (oldChannelList[channelNo] & 0xf) &&
					_mappedNodes[channelNo] == _playlist[hwChannel.playlistIndex()]) {
					continue;
				}

				sendChannelToDriver(*sound, sound->channels[hwChannel.channelNo()], channelNo);
			} else {
				// noCopyBedCh
				for (int innerChannelNo = minChannelNo; innerChannelNo < maxChannelNo; ++innerChannelNo) {
					if (_mappedNodes[innerChannelNo] == sound &&
						// SSCI stored the old channel list separately premasked
						// so used a direct equality comparison; we do not, so
						// need to mask out the playlist index here.
						(oldChannelList[innerChannelNo] & 0xf) == hwChannel.channelNo()) {

						if (!hwChannel.locked) {
							_channelList[innerChannelNo] = hwChannel.key;
							hwChannel.key = HardwareChannel::kUnmapped;
						}

						break;
					}
				}
			}
		}

		// pass 3
		for (int i = 0; i < kNumHardwareChannels; ++i) {
			HardwareChannel &hwChannel = _hardwareChannels[i];
			if (hwChannel.key == HardwareChannel::kUnmapped) {
				continue;
			}

			uint8 lastOpenChannelNo = kNumHardwareChannels;
			for (int j = maxChannelNo; j >= minChannelNo; --j) {
				if (_channelList[j] != HardwareChannel::kUnmapped) {
					lastOpenChannelNo = j;
					break;
				}
			}
			assert(lastOpenChannelNo != kNumHardwareChannels);

			_channelList[lastOpenChannelNo] = hwChannel.key;

			Sci1Sound *sound = _playlist[hwChannel.playlistIndex()];
			assert(sound);
			sendChannelToDriver(*sound, sound->channels[hwChannel.channelNo()], lastOpenChannelNo);
		}
	} else {
		// SSCI fills _channelList with 0xffs here, but this was already just
		// done above, so we don't do anything extra.
	}

	// cleanupChnls
	for (int i = kNumHardwareChannels - 1; i >= 0; --i) {
		if (oldChannelList[i] != HardwareChannel::kUnmapped && _channelList[i] == HardwareChannel::kUnmapped) {
			_driver->controllerChange(i, kDamperPedalController, 0);
			_driver->controllerChange(i, kAllNotesOffController, 0);
			_driver->controllerChange(i, kMaxVoicesController, 0);
		}
	}

	// In SSCI the old channel list was persisted here but we do not need to do
	// that since we can just make it stack-local at the top of the function

	// TODO: Just make it so when a channel becomes unmapped it clears its state
	// so we do not need to do this
	for (int i = 0; i < kNumHardwareChannels; ++i) {
		if (_channelList[i] == HardwareChannel::kUnmapped) {
			_mappedNodes[i] = nullptr;
		} else {
			const uint8 playlistIndex = _channelList[i] >> 4;
			_mappedNodes[i] = _playlist[playlistIndex];
		}
	}
}

uint8 SoundManager::preemptChannel(const int &numFreeVoices) {
	// 'lowest' is weird here since these priority values are inverted, so the
	// biggest number is the lowest priority
	uint8 lowestPriority = 0, lowestPriorityChannelNo = kNumHardwareChannels;
	for (int i = 0; i < kNumHardwareChannels; ++i) {
		HardwareChannel &hwChannel = _hardwareChannels[i];
		if (hwChannel.priority >= lowestPriority) {
			lowestPriority = hwChannel.priority;
			lowestPriorityChannelNo = i;
		}
	}

	if (lowestPriorityChannelNo != kNumHardwareChannels) {
		_hardwareChannels[lowestPriorityChannelNo] = HardwareChannel();
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

	byte lsb, msb;
	convert16To7(channel.pitchBend, lsb, msb);
	driver.pitchBend(hwChannelNo, lsb, msb);

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

		g_sci->_audio32->stop(sound->resource->getId());
	}
#endif

	const GuiResourceId soundNo = readSelectorValue(_segMan, soundObj, SELECTOR(number));
	const ResourceId soundId(getSoundResourceType(soundNo), soundNo);

	// SSCI assigned the resource number to the Sound object here, since we
	// add a pointer to the Resource to the Sound object we just get the
	// resource number from there instead, so that property set is omitted here

	if (!readSelector(_segMan, soundObj, SELECTOR(handle)).isNull() &&
		(_soundVersion < SCI_VERSION_2 || !sound->isSample)) {

		kernelStop(soundObj);
	}

	sound->isSample = (soundId.getType() == kResourceTypeAudio);

	if (sound->isSample) {
		// SSCI32 would optionally preload audio if there was a preload flag in
		// the soundObj's `flags` selector; we do not need to worry about load
		// times, so we just don't do that
		sound->resource = _resMan.testResource(soundId);
	} else {
		sound->resource = _resMan.findResource(soundId, true);
	}

	assert(sound->resource);

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
			g_sci->_audio32->play(g_sci->_audio32->findChannelById(soundId, soundObj), soundId, true, loop, volume, nodePtr, false);
		} else
#endif
			// TODO: This doesn't loop, it should loop, Sci::Audio does not have
			// the capability to do this, it should probably just be using
			// Audio32 which was actually RE'd properly
			g_sci->_audio->startAudio(soundId.getNumber(), soundId.getTuple());
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
				g_sci->_audio32->stop(sound->resource->getId(), soundObj);
			} else
				// TODO: This should be accepting a sound number.
				g_sci->_audio->stopAudio();
#endif
		} else {
			stop(*sound);
			// A sound may be stopped before it is ever started, in which case
			// a resource won't exist yet, and so we do not need to unlock
			// anything
			if (sound->resource) {
				_resMan.unlockResource(sound->resource);
				// Try not to unlock a resource more than once
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
					g_sci->_audio32->pause(sound->resource->getId(), nodePtr);
				} else {
					g_sci->_audio32->resume(sound->resource->getId(), nodePtr);
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
		g_sci->_audio32->fadeChannel(sound->resource->getId(), nodePtr,
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
		g_sci->_audio32->setVolume(sound->resource->getId(), nodePtr, volume);
	}
#endif
	if (sound->volume != volume) {
		setVolume(*sound, volume);
		writeSelectorValue(_segMan, soundObj, SELECTOR(vol), volume);
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

	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (!sound) {
		return;
	}

	writeSelectorValue(_segMan, soundObj, SELECTOR(loop), enable ? 0xffff : 1);
#ifdef ENABLE_SCI32
	if (_soundVersion >= SCI_VERSION_2 && sound->isSample) {
		g_sci->_audio32->setLoop(sound->resource->getId(), nodePtr, enable);
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
			position = g_sci->_audio32->getPosition(sound->resource->getId(), nodePtr);
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

} // end of namespace Sci
