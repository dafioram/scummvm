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
#include "sci/sound/sci1sound.h"
#include "sci/sound/drivers/driver.h"
#include "sci/sound/drivers/genmidi.h"

namespace Sci {

// In SCI1early- the first playlist item is reserved for an
// exclusive sound, so may be blank even if other sounds are playing
// so must be handled separately from the loop over the rest of the
// playlist
#define VALIDATE_PLAYLIST_ITERATOR \
	if (!_playlist[i]) { \
		if (_soundVersion <= SCI_VERSION_1_EARLY && i == 0) continue; \
		else break; \
	}

static inline uint16 convert7To16(byte lsb, byte msb) {
	return (msb << 7) | lsb;
}

Sci1SoundManager::Sci1SoundManager(ResourceManager &resMan, SegManager &segMan, GameFeatures &features, GuestAdditions &guestAdditions) :
	SoundManager(resMan, segMan, features, guestAdditions),
	_useWindowsMidi(false),
	_restoringSound(false),
	_numServerSuspensions(0),
	_needsRemap(false),
	_nextVolumeChangeChannel(0),
	_defaultReverbMode(0),
	_playlist(),
	_sampleList(),
	_samplePlayer(*this, *g_system->getMixer()),
	_nextObjectId(0) {
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

	initDriver(musicType, platform);

	if (features.useAltWinGMSound()) {
		if (musicType != MT_GM) {
			warning("A Windows CD version with an alternate MIDI soundtrack has been chosen, "
					"but no MIDI music device has been selected. Reverting to the DOS soundtrack");
			features.forceDOSTracks();
		} else {
			_useWindowsMidi = true;
		}
	}

#ifdef ENABLE_SCI32
	if (gameHasGeneralMidiOnly() && musicType != MT_GM) {
		warning("This game only supports General MIDI, but a non-GM device has "
				"been selected. Some music may be wrong or missing");
	}
#endif

	// In SSCI, this is in SInit; since we do not implement that operation, we
	// perform its additional initialisation operations here
	_driver->setReverbMode(0);

	g_system->getTimerManager()->installTimerProc(soundServerCallback, 1000000 / 60, this, "SCI MIDI");
}

Sci1SoundManager::~Sci1SoundManager() {
	g_system->getTimerManager()->removeTimerProc(soundServerCallback);

	// Don't allow destruction to finish until after any in-progress sound
	// server callback has finished running
	Common::StackLock lock(_mutex);

	// SSCI1early- also looped the playlist to explicitly terminate all sounds;
	// we do not need to do this since the other destructors will do all our
	// cleaning up for us

	// In SSCI, this is in STerminate; since we do not implement that operation,
	// we perform its additional termination operations here
	if (_driver) {
		_driver->setMasterVolume(kMaxMasterVolume);
	}
}

GuiResourceId Sci1SoundManager::getSoundResourceId(const uint16 soundNo) const {
	if (_useWindowsMidi && soundNo != 0) {
		// Some Windows sounds don't exist (e.g. SQ4, room 530 - bug #3392767),
		// so actually check before using the higher value
		const ResourceId testId(kResourceTypeSound, soundNo + 1000);
		if (_resMan.testResource(testId)) {
			return soundNo + 1000;
		}
	}

	// SQ4CD has a few higher-quality samples in the Windows-range which we can
	// apply to all game versions regardless of the OS setting
	if (soundNo < 1000 && _preferSampledSounds &&
		g_sci->getGameId() == GID_SQ4 && g_sci->isCD() &&
		_resMan.testResource(ResourceId(kResourceTypeAudio, soundNo + 1000))) {

		return soundNo + 1000;
	}

	return soundNo;
}

bool Sci1SoundManager::gameHasGeneralMidiOnly() const {
#ifdef ENABLE_SCI32
	switch (g_sci->getGameId()) {
	case GID_MOTHERGOOSEHIRES:
		return true;
	case GID_KQ7: {
		if (g_sci->isDemo()) {
			return false;
		}

		const Resource *sound = _resMan.findResource(ResourceId(kResourceTypeSound, 13), false);
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

void Sci1SoundManager::restore(Sci1Sound &sound) {
	uint8 muteCounts[Sci1Sound::kNumChannels];
	const uint8 holdPoint = sound.holdPoint;

	if (_soundVersion >= SCI_VERSION_1_MIDDLE) {
		for (uint i = 0; i < Sci1Sound::kNumChannels; ++i) {
			muteCounts[i] = sound.channels[i].gameMuteCount;
		}
	}

	_restoringSound = true;

	// TODO: SSCI32 always passed `false` even if the sound was an exclusive
	// sound
	const uint8 playlistIndex = play(sound, sound.state == Sci1Sound::kExclusive);

	// This was an unchecked condition in SSCI, which would have caused some
	// problems if >16 sounds were ever saved
	if (playlistIndex == _playlist.size()) {
		return;
	}

	uint16 ticksToRestore = sound.ticksElapsed;
	sound.ticksElapsed = 0;
	const bool loopToRestore = sound.loop;
	sound.loop = true;

	// SSCI performed an early jump if ticksToRestore was 0 which is equivalent
	// to this while-loop condition
	while (sound.ticksElapsed != ticksToRestore) {
		uint16 lastTicks = sound.ticksElapsed;
		advancePlayback(sound, playlistIndex);
		if (lastTicks == sound.ticksElapsed) {
			break;
		} else if (lastTicks > sound.ticksElapsed) {
			if (_soundVersion <= SCI_VERSION_1_EARLY) {
				ticksToRestore -= lastTicks;
			} else {
				ticksToRestore -= lastTicks - sound.ticksElapsed;
			}
		}
	}

	sound.loop = loopToRestore;
	_restoringSound = false;
	if (_soundVersion >= SCI_VERSION_1_MIDDLE) {
		for (uint i = 0; i < sound.channels.size(); ++i) {
			sound.channels[i].gameMuteCount = muteCounts[i];
		}
	}
	if (_soundVersion >= SCI_VERSION_1_1) {
		sound.holdPoint = holdPoint;
	}
	remapHardwareChannels();
}

#pragma mark -
#pragma mark MIDI server

void Sci1SoundManager::enableSoundServer(const bool enable) {
	Common::StackLock lock(_mutex);

	// In SSCI1early- this function used a boolean instead of a counter, but
	// games could not access this function at all, so we can just always use
	// the counter mode

	if (!enable) {
		++_numServerSuspensions;
	} else if (_numServerSuspensions) {
		--_numServerSuspensions;
	}
}

void Sci1SoundManager::soundServer() {
	Common::StackLock lock(_mutex);
	if (_numServerSuspensions) {
		return;
	}

	if (_needsRemap) {
		remapHardwareChannels();
	}

	for (uint i = 0; i < _playlist.size(); ++i) {
		VALIDATE_PLAYLIST_ITERATOR
		Sci1Sound &sound = *_playlist[i];

		// SSCI1.1+ also checked if the resource pointer == 1 and would avoid
		// processing in that case as well; game scripts cannot send pointers to
		// the sound engine, so that value would have had to come from elsewhere
		// in the kernel, and we have no code that does that, so we do not do
		// that check
		if (sound.numPauses || (_soundVersion >= SCI_VERSION_1_1 && sound.isSample)) {
			continue;
		}

		if (sound.fadeAmountPerTick) {
			processFade(sound);

			if (sound.signal == Sci1Sound::kFinished) {
				--i;
				continue;
			}
		}

		if (_soundVersion < SCI_VERSION_1_1 && sound.isSample) {
			validateSample(sound);
		} else {
			advancePlayback(sound, i);
		}

		if (sound.signal == Sci1Sound::kFinished) {
			--i;
		}
	}

	if (_soundVersion < SCI_VERSION_1_1) {
		advanceSamplePlayback();
	}
	applyPendingVolumeChanges();
	_driver->service();
}

void Sci1SoundManager::processFade(Sci1Sound &sound) {
	if (sound.fadeDelayRemaining) {
		--sound.fadeDelayRemaining;
		return;
	} else {
		sound.fadeDelayRemaining = sound.fadeDelay;
	}

	if (sound.fadeTargetVolume == sound.volume) {
		// TODO: Check PQ1VGA
		if (_soundVersion >= SCI_VERSION_1_1) {
			sound.signal = Sci1Sound::kFadeFinished;
		}
		sound.fadeAmountPerTick = 0;
		if (sound.stopSoundOnFade) {
			removeSoundFromPlaylist(sound);
			_needsRemap = true;
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
#pragma mark Channel remapping

void Sci1SoundManager::remapHardwareChannels() {
	_needsRemap = false;

	const HardwareChannels oldChannels(_hardwareChannels);
	Common::fill(_hardwareChannels.begin(), _hardwareChannels.end(), HardwareChannel());

	if (_playlist[0] || (_soundVersion <= SCI_VERSION_1_EARLY && _playlist[1])) {
		uint8 minChannelNo, maxChannelNo;
		_driver->getRemapRange(minChannelNo, maxChannelNo);

		uint8 reverbMode = _playlist[_playlist[0] ? 0 : 1]->reverbMode;
		if (reverbMode == kUseDefaultReverb) {
			reverbMode = _defaultReverbMode;
		}
		_driver->setReverbMode(reverbMode);

		// In SSCI, this was done in the same loop as creating the channel map;
		// for the sake of clarity, and to keep makeChannelMap const, we do this
		// extra loop here
		if (_soundVersion < SCI_VERSION_1_1) {
			// This is a compatible combination of the SCI1early- and SCI1mid+
			// digital sample list creation algorithms. In SCI1mid+ at most one
			// sample is ever assigned; in SCI1early- up to `numFreeDacs`
			// samples are put into the sample list for playback.
			int numFreeDacs = getNumDacs();
			int nextSampleIndex = 0;
			Common::fill(_sampleList.begin(), _sampleList.end(), nullptr);

			for (uint i = 0; numFreeDacs && i < _playlist.size(); ++i) {
				VALIDATE_PLAYLIST_ITERATOR
				Sci1Sound &sound = *_playlist[i];
				if (sound.isSample) {
					_sampleList[nextSampleIndex++] = &sound;
					--numFreeDacs;
				}
			}
		}

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

Sci1SoundManager::HardwareChannels Sci1SoundManager::makeChannelMap(const uint8 minChannelNo, const uint8 maxChannelNo) const {
	HardwareChannels committedChannels = {};
	int committedFreeVoices = _driver->getNumVoices();
	// loopDoNodes
	for (uint i = 0, basePriority = 0; i < _playlist.size(); ++i, basePriority += Sci1Sound::kNumChannels) {
		VALIDATE_PLAYLIST_ITERATOR
		const Sci1Sound &sound = *_playlist[i];

		if (sound.numPauses > 0 || sound.isSample) {
			// jmp nextNode
			continue;
		}

		HardwareChannels workingChannels(committedChannels);
		int workingFreeVoices(committedFreeVoices);

		// loopDoTracks
		for (uint trackNo = 0; trackNo < sound.tracks.size(); ++trackNo) {
			const Sci1Sound::Track &track = sound.tracks[trackNo];
			if (track.channelNo == Sci1Sound::Track::kEndOfData ||
				track.channelNo == Sci1Sound::Track::kSampleTrack ||
				track.channelNo == kControlChannel) {

				// jmp nextChTrack
				continue;
			}

			const Sci1Sound::Channel &channel = sound.channels[track.channelNo];

			if ((channel.flags & Sci1Sound::Channel::kExtra) || channel.muted) {
				// jmp nextChTrack
				continue;
			}

			// notMutedChnl
			const uint8 key = makeChannelKey(i, track.channelNo);

			uint8 priority = channel.priority;
			if (priority != 0) {
				priority = Sci1Sound::kNumChannels - priority + basePriority;
			}

			// nonPreemptable
			if (mapSingleChannel(key, priority, workingFreeVoices, track.channelNo, channel, workingChannels, minChannelNo, maxChannelNo)) {
				committedChannels = workingChannels;
				committedFreeVoices = workingFreeVoices;
			} else {
				// blewIt
				// nextNode
				break;
			}
		}
	}

	return committedChannels;
}

bool Sci1SoundManager::mapSingleChannel(const uint8 key, const uint8 priority, int &numFreeVoices, const uint8 inChannelNo, const Sci1Sound::Channel &channel, HardwareChannels &newChannels, const uint8 minChannelNo, const uint8 maxChannelNo) const {
	uint8 bestHwChannelNo; // dh

	// nonPreemptable
	if (!(channel.flags & Sci1Sound::Channel::kLocked) || newChannels[inChannelNo].isMapped()) {
		// lookOpenChnl
		bestHwChannelNo = HardwareChannel::kUnmapped;
		for (uint hwChannelNo = 0; hwChannelNo < newChannels.size(); ++hwChannelNo) {
			const HardwareChannel &newHwChannel = newChannels[hwChannelNo];
			if (newHwChannel.key == key) {
				// jmp nextChTrack
				return true;
			} else if (!newHwChannel.isMapped() &&
				hwChannelNo >= minChannelNo && hwChannelNo <= maxChannelNo) {

				bestHwChannelNo = hwChannelNo;
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
	assert(bestHwChannelNo < newChannels.size());

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
					// TODO: We already subtracted these voices once, is this an
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

void Sci1SoundManager::commitFixedChannels(HardwareChannels &newChannels, const HardwareChannels &oldChannels, const uint8 minChannelNo, const uint8 maxChannelNo) {
	// loopPass2
	for (uint newChannelNo = 0; newChannelNo < newChannels.size(); ++newChannelNo) {
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

			if (oldChannel.channelNo() != newChannel.channelNo() ||
				oldChannel.sound != newChannel.sound) {
				// notSameBed
				sendChannelToDriver(*newChannel.sound, newChannel.sound->channels[newChannel.channelNo()], newChannelNo);
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

void Sci1SoundManager::commitDynamicChannels(const HardwareChannels &newChannels, const HardwareChannels &oldChannels, const uint8 minChannelNo, const uint8 maxChannelNo) {
	// doPass3
	for (uint i = 0; i < newChannels.size(); ++i) {
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
		sendChannelToDriver(*newChannel.sound, newChannel.sound->channels[newChannel.channelNo()], lastOpenChannelNo);
	}
}

void Sci1SoundManager::stopOldChannels(const HardwareChannels &newChannels, const HardwareChannels &oldChannels) {
	for (int i = newChannels.size() - 1; i >= 0; --i) {
		if (oldChannels[i].isMapped() && !newChannels[i].isMapped()) {
			_driver->controllerChange(i, kDamperPedalController, 0);
			_driver->controllerChange(i, kAllNotesOffController, 0);
			_driver->controllerChange(i, kMaxVoicesController, 0);
		}
	}
}

uint8 Sci1SoundManager::preemptChannel(HardwareChannels &newChannels, int &numFreeVoices) const {
	uint8 lowestPriority = 0;
	uint8 lowestPriorityChannelNo = HardwareChannel::kUnmapped;
	for (uint i = 0; i < newChannels.size(); ++i) {
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

void Sci1SoundManager::sendChannelToDriver(const Sci1Sound &sound, const Sci1Sound::Channel &channel, const uint8 hwChannelNo) {
	SoundDriver &driver = *_driver;
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
#pragma mark Effects

uint8 Sci1SoundManager::getReverbMode() const {
	Common::StackLock lock(_mutex);
	return _driver->getReverbMode();
}

uint8 Sci1SoundManager::setReverbMode(const uint8 reverbMode) {
	Common::StackLock lock(_mutex);

	// Conditions for reverbMode == 0xff and reverbMode > 10 are moved into
	// kDoSoundGlobalReverb

	uint8 oldReverbMode = _defaultReverbMode;
	_defaultReverbMode = reverbMode;

	uint8 unset;
	if (_soundVersion <= SCI_VERSION_1_EARLY) {
		// This is probably an original engine bug, since the default reverb
		// mode is the same value across all SCI versions
		unset = 0xff;
	} else {
		unset = kUseDefaultReverb;
	}

	bool valid = (_playlist[0] && _playlist[0]->reverbMode == unset);
	if (!valid && _soundVersion <= SCI_VERSION_1_EARLY) {
		valid = (_playlist[1] && _playlist[1]->reverbMode == unset);
	}

	if (valid) {
		oldReverbMode = _driver->getReverbMode();
		_driver->setReverbMode(reverbMode);
	}

	return oldReverbMode;
}

void Sci1SoundManager::setVolume(Sci1Sound &sound, const uint8 volume) {
	processVolumeChange(sound, volume, false);
}

void Sci1SoundManager::fade(Sci1Sound &sound, const int16 targetVolume, const int16 speed, const int16 steps, const bool stopAfterFade) {
	if (sound.volume == targetVolume) {
		return;
	}

	// In SSCI the stop flag got baked into the high bit of target volume,
	// make sure that we are not getting any out of range values from scripts
	// that relied on that (or for any other reason)
	assert(targetVolume >= 0 && targetVolume <= Sci1Sound::kMaxVolume);

	sound.fadeTargetVolume = targetVolume;
	sound.stopSoundOnFade = stopAfterFade;
	sound.fadeAmountPerTick = steps;
	sound.fadeDelay = speed;
	sound.fadeDelayRemaining = 0;
}

void Sci1SoundManager::mute(Sci1Sound &sound, const bool mute) {
	Common::StackLock lock(_mutex);
	for (int channelNo = sound.channels.size() - 1; channelNo >= 0; --channelNo) {
		Sci1Sound::Channel &channel = sound.channels[channelNo];
		if (mute) {
			if (channel.gameMuteCount < 15) {
				++channel.gameMuteCount;
			}
		} else if (channel.gameMuteCount) {
			--channel.gameMuteCount;
		}
	}
	remapHardwareChannels();
}

void Sci1SoundManager::processVolumeChange(Sci1Sound &sound, const uint8 volume, const bool enqueue) {
	if (volume == sound.volume) {
		return;
	}

	sound.volume = volume;

	const uint8 playlistIndex = findPlaylistIndex(sound);
	if (playlistIndex == _playlist.size()) {
		return;
	}

	for (uint hwChannelNo = 0; hwChannelNo < _hardwareChannels.size(); ++hwChannelNo) {
		const HardwareChannel &hwChannel = _hardwareChannels[hwChannelNo];
		if (hwChannel.isMapped() && hwChannel.playlistIndex() == playlistIndex) {
			changeChannelVolume(sound, hwChannel.channelNo(), hwChannelNo, enqueue);
		}
	}

	for (uint trackNo = 0; trackNo < sound.tracks.size(); ++trackNo) {
		const uint8 channelNo = sound.tracks[trackNo].channelNo;
		if (channelNo == Sci1Sound::Track::kEndOfData) {
			break;
		}

		// In SSCI, this code would overread past the end of channel flags into
		// the mute save field for the control channel
		if (channelNo == kControlChannel) {
			continue;
		}

		const Sci1Sound::Channel &channel = sound.channels[channelNo];
		if ((channel.flags & Sci1Sound::Channel::kExtra) &&
			!_hardwareChannels[channelNo].isMapped()) {

			changeChannelVolume(sound, channelNo, channelNo, enqueue);
		}
	}
}

void Sci1SoundManager::changeChannelVolume(const Sci1Sound &sound, const uint8 channelNo, const uint8 hwChannelNo, const bool enqueue) {
	const uint8 channelVolume = sound.channels[channelNo].volume * sound.volume / Sci1Sound::kMaxVolume;
	uint8 &newVolume = _newChannelVolumes[hwChannelNo];
	if (enqueue) {
		newVolume = channelVolume;
	} else {
		newVolume = kNoVolumeChange;
		_driver->controllerChange(hwChannelNo, kVolumeController, channelVolume);
	}
}

void Sci1SoundManager::applyPendingVolumeChanges() {
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
		if (++channelNo == _newChannelVolumes.size()) {
			channelNo = 0;
		}
	} while (numUpdates < 2 && channelNo != originalChannel);
}

#pragma mark -
#pragma mark Playback management

void Sci1SoundManager::pauseAll(const bool pause) {
	Common::StackLock lock(_mutex);
	for (uint i = 0; i < _playlist.size(); ++i) {
		// SSCI1mid+ didn't get rid of the i == 0 check here from SCI1early-
		// even though it was no longer valid; this does not matter functionally
		// and would just have been a tiny efficiency issue in SSCI
		VALIDATE_PLAYLIST_ITERATOR
		Sci1Sound &sound = *_playlist[i];
		if (pause) {
			++sound.numPauses;
		} else if (sound.numPauses) {
			--sound.numPauses;
		}
	}

	remapHardwareChannels();
}

static bool hasSynthesisedTracks(const Sci1Sound &sound) {
	for (uint trackNo = 0; trackNo < sound.tracks.size(); ++trackNo) {
		const Sci1Sound::Track &track = sound.tracks[trackNo];
		if (track.offset == 0) {
			break;
		}
		SciSpan<const byte> trackData = sound.resource->subspan(track.offset);
		const uint8 channelNo = trackData[0];
		if (channelNo != Sci1Sound::Track::kSampleTrack) {
			return true;
		}
	}
	return false;
}

uint8 Sci1SoundManager::play(Sci1Sound &sound, const bool exclusive) {
	const bool removed = removeSoundFromPlaylist(sound);

	uint8 playlistIndex;

	if (_soundVersion <= SCI_VERSION_1_EARLY) {
		if (removed) {
			remapHardwareChannels();
		}

		if (exclusive) {
			if (_playlist[0]) {
				removeSoundFromPlaylist(*_playlist[0]);
			}
			_playlist[0] = &sound;
		} else {
			playlistIndex = insertSoundToPlaylist(sound);
			if (playlistIndex == _playlist.size()) {
				return playlistIndex;
			}
		}
	}

	sound.state = exclusive ? Sci1Sound::kExclusive : Sci1Sound::kPlaying;
	Common::fill(sound.channels.begin(), sound.channels.end(), Sci1Sound::Channel());
	Common::fill(sound.tracks.begin(), sound.tracks.end(), Sci1Sound::Track());
	sound.isSample = false;
	sound.holdPoint = 0;
	sound.reverbMode = kUseDefaultReverb;
	sound.loopTicksElapsed = 0;
	sound.volume = Sci1Sound::kMaxVolume;

	// This is a little different than SSCI because we do not scribble a new
	// header onto the file, so this used to be stuff that fixupHeader did.
	if (_soundVersion >= SCI_VERSION_1_MIDDLE &&
		!sound.fixedPriority && sound.resource->getUint8At(0) == 0xf0) {

		sound.priority = sound.resource->getUint8At(1);
	}

	readTrackOffsets(sound);

	for (uint trackNo = 0; trackNo < sound.tracks.size(); ++trackNo) {
		Sci1Sound::Track &track = sound.tracks[trackNo];
		if (track.offset == 0) {
			break;
		}
		SciSpan<const byte> trackData = sound.resource->subspan(track.offset);

		const uint8 channelNo = trackData[0];

		if (_soundVersion < SCI_VERSION_1_1 && channelNo == Sci1Sound::Track::kSampleTrack) {
			track.channelNo = channelNo;

			if (_preferSampledSounds || !hasSynthesisedTracks(sound)) {
				sound.isSample = true;
				sound.sampleTrackNo = trackNo;
			} else {
				track.position = 0;
				track.loopPosition = 0;
			}

			continue;
		}

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
			Sci1Sound::Channel &channel = sound.channels[track.channelNo];

			// SCI1early- did not have channel flags, instead they had only
			// channels 0-15 for normal channels and 16-31 for extra channels
			// and checked the channel number instead of flags where necessary.
			// This information is never exposed outside of the MIDI engine so
			// we can just always use the SCI1mid+ flags-based implementation.

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

			if (_soundVersion >= SCI_VERSION_1_MIDDLE) {
				if (channelNo & kLockedChannelFlag) {
					channel.flags = Sci1Sound::Channel::kLocked;
				}

				if (channelNo & kMutedChannelFlag) {
					channel.muted = true;
				}
			}

			if (channel.priority == Sci1Sound::Channel::kUninitialized) {
				channel.priority = trackData[1] >> 4;
				channel.numVoices = trackData[1] & 0xf;
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

	// SSCI1early- did not have a way of marking individual channels as locked.
	// Instead, when an exclusive sound was received, it would do special work
	// to keep the exclusive sound in playlist position 0 and keep its channels
	// locked to the hardware channels. SSCI1mid+ used this flags approach
	// instead. For simplicity in implementation, we always use the flags
	// approach, since it should be compatible with the SSCI1early- channel
	// remapping algorithm.
	if (sound.state & Sci1Sound::kExclusive) {
		for (uint channelNo = 0; channelNo < sound.channels.size(); ++channelNo) {
			Sci1Sound::Channel &channel = sound.channels[channelNo];
			channel.flags = Sci1Sound::Channel::ChannelFlags(channel.flags | Sci1Sound::Channel::kLocked);
		}
	}

	if (_soundVersion >= SCI_VERSION_1_MIDDLE) {
		playlistIndex = insertSoundToPlaylist(sound);
	}

	if (playlistIndex != _playlist.size() && !_restoringSound) {
		sound.cue = 0;
		sound.ticksElapsed = 0;
		sound.signal = Sci1Sound::kNoSignal;
		sound.fadeTargetVolume = 0;
		sound.fadeDelay = 0;
		sound.fadeDelayRemaining = 0;
		sound.fadeAmountPerTick = 0;
		sound.stopSoundOnFade = false;
		sound.numPauses = 0;

		remapHardwareChannels();
	}

	return playlistIndex;
}

void Sci1SoundManager::pause(Sci1Sound &sound, const uint8 numPauses) {
	sound.numPauses = numPauses;
	remapHardwareChannels();
}

void Sci1SoundManager::pause(Sci1Sound &sound, const bool pause) {
	if (pause) {
		++sound.numPauses;
	} else if (sound.numPauses) {
		--sound.numPauses;
	}

	remapHardwareChannels();
}

void Sci1SoundManager::stop(Sci1Sound &sound) {
	removeSoundFromPlaylist(sound);
	remapHardwareChannels();
}

void Sci1SoundManager::hold(Sci1Sound &sound, const uint8 holdPoint) {
	sound.holdPoint = holdPoint;
}

Sci1Sound::Signal Sci1SoundManager::peekSignal(const Sci1Sound &sound) {
	Common::StackLock lock(_mutex);
	return sound.signal;
}

Sci1Sound::Signal Sci1SoundManager::consumeSignal(Sci1Sound &sound) {
	Sci1Sound::Signal signal = sound.signal;
	sound.signal = Sci1Sound::kNoSignal;
	return signal;
}

uint16 Sci1SoundManager::getCue(const Sci1Sound &sound) {
	Common::StackLock lock(_mutex);
	return sound.cue;
}

Sci1SoundManager::Position Sci1SoundManager::getPosition(const Sci1Sound &sound) {
	const uint16 ticksElapsed = sound.ticksElapsed;
	Position position;
	position.minutes = ticksElapsed / 3600;
	position.seconds = (ticksElapsed / 60) % 60;
	position.frames = (ticksElapsed % 60) / 2;
	return position;
}

void Sci1SoundManager::setPriority(Sci1Sound &sound, const uint8 priority) {
	if (sound.priority == priority) {
		return;
	}

	sound.priority = priority;

	const uint8 playlistIndex = findPlaylistIndex(sound);
	if (playlistIndex == _playlist.size()) {
		return;
	}

	if (_soundVersion <= SCI_VERSION_1_EARLY && playlistIndex == 0) {
		return;
	}

	for (uint i = playlistIndex; i < _playlist.size() - 1; ++i) {
		_playlist[i] = _playlist[i + 1];
	}
	_playlist[_playlist.size() - 1] = nullptr;
	insertSoundToPlaylist(sound);

	remapHardwareChannels();
}

#pragma mark -
#pragma mark Sound generation

void Sci1SoundManager::setNoteOff(Sci1Sound &sound, const uint8 channelNo, const uint8 note, const uint8 velocity) {
	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	sound.channels[channelNo].currentNote = Sci1Sound::Channel::kNoCurrentNote;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != HardwareChannel::kUnmapped) {
		_driver->noteOff(hwChannelNo, note, velocity);
	}
}

void Sci1SoundManager::setNoteOn(Sci1Sound &sound, const uint8 channelNo, const uint8 note, const uint8 velocity) {
	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	sound.channels[channelNo].currentNote = note;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != HardwareChannel::kUnmapped) {
		_driver->noteOn(hwChannelNo, note, velocity);
	}
}

void Sci1SoundManager::setController(Sci1Sound &sound, const uint8 channelNo, const uint8 controllerNo, uint8 value) {
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
		if (_soundVersion >= SCI_VERSION_1_MIDDLE) {
			enum { kUnmute = 0 };
			if (value == kUnmute) {
				if (channel.gameMuteCount) {
					--channel.gameMuteCount;
					remapHardwareChannels();
				}
			} else if (channel.gameMuteCount < 15) {
				++channel.gameMuteCount;
				remapHardwareChannels();
			}
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

void Sci1SoundManager::setProgram(Sci1Sound &sound, const uint8 channelNo, const uint8 programNo) {
	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	sound.channels[channelNo].program = programNo;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != HardwareChannel::kUnmapped) {
		_driver->programChange(hwChannelNo, programNo);
	}
}

void Sci1SoundManager::setPitchBend(Sci1Sound &sound, const uint8 channelNo, const uint16 value) {
	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	sound.channels[channelNo].pitchBend = value;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != HardwareChannel::kUnmapped) {
		_driver->pitchBend(hwChannelNo, value);
	}
}

#pragma mark -
#pragma mark Data processing

SciSpan<const byte> Sci1SoundManager::findTrackOffsets(SciSpan<const byte> data, const uint8 deviceId) const {
	if (_soundVersion >= SCI_VERSION_1_MIDDLE && data[0] == 0xf0) {
		data += 8;
	}

	for (uint8 searchId = *data++; searchId != 0xff; searchId = *data++) {
		if (searchId == deviceId) {
			return data;
		} else {
			while (*data++ != 0xff) {
				data += 5;
			}
		}
	}

	return SciSpan<const byte>();
}

void Sci1SoundManager::readTrackOffsets(Sci1Sound &sound) {
	SoundDriver::DeviceId deviceId = _driver->getDeviceId();
	SciSpan<const byte> data = findTrackOffsets(*sound.resource, deviceId);
	if (!data) {
		debugC(kDebugLevelSound, "%s has no data for device type %u", sound.resource->name().c_str(), deviceId);
		return;
	}

	int trackNo = 0;
	for (uint8 endMarker = *data; endMarker != 0xff; endMarker = *data) {
		// TODO: Could be SCI-endian
		Sci1Sound::Track &track = sound.tracks[trackNo++];
		track.offset = data.getUint16LEAt(2);
		track.size = data.getUint16LEAt(4);

		if (track.offset >= sound.resource->size()) {
			warning("Offset for %s device %u track %u is out of bounds (%u >= %u); skipping",
					sound.resource->name().c_str(), deviceId, trackNo,
					track.offset, sound.resource->size());
			track.offset = 0;
		} else if (track.offset + track.size > sound.resource->size()) {
			const uint16 maxSize = sound.resource->size() - track.offset;
			warning("Size for %s device %u track %u is out of bounds (%u > %u); truncating",
					sound.resource->name().c_str(), deviceId, trackNo,
					track.size, maxSize);
			track.size = maxSize;
		}
		data += 6;
	}
}

void Sci1SoundManager::advancePlayback(Sci1Sound &sound, uint8 playlistIndex) {
	++sound.ticksElapsed;

	for (uint trackNo = 0; trackNo < sound.tracks.size(); ++trackNo) {
		// In SSCI playlist index was shifted here, we do it at point of use
		// below

		Sci1Sound::Track &track = sound.tracks[trackNo];
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

	for (uint i = 0; i < sound.tracks.size(); ++i) {
		Sci1Sound::Track &track = sound.tracks[i];
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
		for (uint i = 0; i < sound.tracks.size(); ++i) {
			Sci1Sound::Track &track = sound.tracks[i];
			track.position = track.loopPosition;
			track.rest = track.loopRest;
			track.command = track.loopCommand;
		}
	} else {
		removeSoundFromPlaylist(sound);
		_needsRemap = true;
	}
}

void Sci1SoundManager::parseCommand(Sci1Sound &sound, const uint8 playlistIndex, const uint8 trackNo, Sci1Sound::Track &track) {
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
			Sci1Sound::Channel &channel = sound.channels[channelNo];

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
				const uint8 key = makeChannelKey(playlistIndex, track.channelNo);
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

void Sci1SoundManager::parseControlChannel(Sci1Sound &sound, const uint8 trackNo, const MidiMessageType command) {
	Sci1Sound::Track &track = sound.tracks[trackNo];

	switch (command) {
	case kProgramChange: {
		const byte message = sound.consume(trackNo);
		if (message == kSetLoop) {
			const byte value = sound.consume(trackNo);
			if (value == kFixedRest) {
				if (_soundVersion <= SCI_VERSION_1_EARLY) {
					track.rest = kFixedRestFlag | value;
				} else {
					track.rest = kFixedRestValue;
				}
			} else {
				track.rest = value;
			}

			track.command = kProgramChange | kControlChannel;

			for (uint i = 0; i < sound.tracks.size(); ++i) {
				Sci1Sound::Track &loopTrack = sound.tracks[i];
				loopTrack.loopPosition = loopTrack.position;
				loopTrack.loopRest = loopTrack.rest;
				loopTrack.loopCommand = loopTrack.command;
			}

			sound.loopTicksElapsed = sound.ticksElapsed;
			track.rest = 0;
			--track.position;
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
				for (uint i = 0; i < sound.tracks.size(); ++i) {
					sound.tracks[i].position = 0;
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

void Sci1SoundManager::processNoteOff(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	const uint8 note = sound.consume(trackNo);
	const uint8 velocity = sound.consume(trackNo);

	const uint8 channelNo = sound.tracks[trackNo].channelNo;
	Sci1Sound::Channel &channel = sound.channels[channelNo];
	if (channel.currentNote == note) {
		channel.currentNote = Sci1Sound::Channel::kNoCurrentNote;
	}

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		_driver->noteOff(hwChannelNo & 0xf, note, velocity);
	}
}

void Sci1SoundManager::processNoteOn(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	enum { kVelocityOffset = 1 };
	if (sound.peek(trackNo, kVelocityOffset) == 0) {
		return processNoteOff(sound, trackNo, hwChannelNo);
	}

	const uint8 note = sound.consume(trackNo);
	const uint8 velocity = sound.consume(trackNo);

	sound.channels[sound.tracks[trackNo].channelNo].currentNote = note;

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		_driver->noteOn(hwChannelNo & 0xf, note, velocity);
	}
}

void Sci1SoundManager::processKeyPressure(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	const uint8 note = sound.consume(trackNo);
	const uint8 pressure = sound.consume(trackNo);

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		// SSCI did not clamp the channel range here, see if this is a problem
		if (hwChannelNo >= _hardwareChannels.size()) {
			warning("Key pressure note %u pressure %u channel %u out of range", note, pressure, hwChannelNo);
		}

		_driver->keyPressure(hwChannelNo, note, pressure);
	}
}

void Sci1SoundManager::processControllerChange(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool isExtraChannel) {
	const uint8 controllerNo = sound.consume(trackNo);
	uint8 value = sound.consume(trackNo);

	const uint8 inRangeChannelNo = hwChannelNo & 0xf;

	if (isExtraChannel && _hardwareChannels[inRangeChannelNo].isMapped()) {
		return;
	}

	Sci1Sound::Channel &channel = sound.channels[sound.tracks[trackNo].channelNo];

	switch (controllerNo) {
	case kVolumeController:
		// TODO: This is vestigial in SSCI1mid+, left in from SSCI1early- where
		// high channel numbers were used for the 'extra' channel flags
		if (hwChannelNo >= _hardwareChannels.size() * 2) {
			return;
		}

		// There appears to be a masking error in SSCI where out of range
		// channels would write out of bounds into the save mute flags (they
		// used 0xff instead of 0xf); we do not do that, this might cause volume
		// issues since any pending volumes would have been applied again in
		// SSCI and won't here.
		if (hwChannelNo >= _hardwareChannels.size()) {
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
		_needsRemap = true;
		if (_soundVersion >= SCI_VERSION_1_1) {
			value &= 0xf;
		}
		channel.numVoices = value;
		break;
	case kMuteController:
		if (_soundVersion >= SCI_VERSION_1_MIDDLE) {
			_needsRemap = true;
			channel.muted = (value != 0);
			if (channel.muted) {
				value = 1;
			}
		}
		break;
	}

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		_driver->controllerChange(inRangeChannelNo, controllerNo, value);
	}
}

void Sci1SoundManager::processProgramChange(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool isExtraChannel) {
	const uint8 programNo = sound.consume(trackNo);
	const uint8 inRangeChannelNo = hwChannelNo & 0xf;

	if (isExtraChannel && _hardwareChannels[inRangeChannelNo].isMapped()) {
		return;
	}

	sound.channels[sound.tracks[trackNo].channelNo].program = programNo;

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		_driver->programChange(inRangeChannelNo, programNo);
	}
}

void Sci1SoundManager::processChannelPressure(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	const uint8 pressure = sound.consume(trackNo);

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		// SSCI did not clamp the channel range here, see if this is a problem
		if (hwChannelNo >= _hardwareChannels.size()) {
			warning("Channel pressure pressure %u channel %u out of range", pressure, hwChannelNo);
		}

		_driver->channelPressure(hwChannelNo, pressure);
	}
}

void Sci1SoundManager::processPitchBend(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool isExtraChannel) {
	const uint8 lsb = sound.consume(trackNo);
	const uint8 msb = sound.consume(trackNo);

	const uint8 inRangeChannelNo = hwChannelNo & 0xf;

	if (isExtraChannel && _hardwareChannels[inRangeChannelNo].isMapped()) {
		return;
	}

	const uint16 value = convert7To16(lsb, msb);
	sound.channels[sound.tracks[trackNo].channelNo].pitchBend = value;

	if (hwChannelNo != HardwareChannel::kUnmapped && !_restoringSound) {
		_driver->pitchBend(inRangeChannelNo, value);
	}
}

void Sci1SoundManager::processSysEx(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	skipCommand(sound, trackNo, kSysEx);
}

void Sci1SoundManager::skipCommand(Sci1Sound &sound, const uint8 trackNo, const MidiMessageType command) {
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

uint8 Sci1SoundManager::insertSoundToPlaylist(Sci1Sound &sound) {
	uint insertAt = _soundVersion <= SCI_VERSION_1_EARLY ? 1 : 0;
	for (; insertAt < _playlist.size(); ++insertAt) {
		if (!_playlist[insertAt] || sound.priority <= _playlist[insertAt]->priority) {
			break;
		}
	}

	if (insertAt == _playlist.size()) {
		return insertAt;
	}

	for (int i = int(_playlist.size()) - 2; i >= int(insertAt); --i) {
		_playlist[i + 1] = _playlist[i];
	}
	_playlist[insertAt] = &sound;
	return insertAt;
}

bool Sci1SoundManager::removeSoundFromPlaylist(Sci1Sound &sound) {
	for (uint i = 0; i < _playlist.size(); ++i) {
		VALIDATE_PLAYLIST_ITERATOR
		if (_playlist[i] == &sound) {
			sound.signal = Sci1Sound::kFinished;
			sound.state = Sci1Sound::kStopped;
			if (_soundVersion < SCI_VERSION_1_1 && (sound.sampleTrackNo & kSampleLoadedFlag)) {
				_samplePlayer.unload();
			}
			for (; i < _playlist.size() - 1; ++i) {
				_playlist[i] = _playlist[i + 1];
			}
			_playlist[i] = nullptr;
			return true;
		}
	}
	return false;
}

#pragma mark -
#pragma mark Digital sample playback

void Sci1SoundManager::validateSample(Sci1Sound &sound) {
	// In SSCI a null sample list entry was not checked, but since the list is
	// always contiguous there is no reason to continue testing after reaching a
	// null pointer
	for (uint i = 0; i < _sampleList.size() && _sampleList[i]; ++i) {
		if (&sound == _sampleList[i]) {
			return;
		}
	}

	removeSoundFromPlaylist(sound);
}

void Sci1SoundManager::advanceSamplePlayback() {
	for (uint i = 0; i < _sampleList.size() && _sampleList[i]; ++i) {
		Sci1Sound &sound = *_sampleList[i];

		++sound.ticksElapsed;

		if (!(sound.sampleTrackNo & kSampleLoadedFlag)) {
			sound.sampleTrackNo |= kSampleLoadedFlag;
			_samplePlayer.load(sound);
		} else {
			const SamplePlayer::Status status = _samplePlayer.advance(sound);
			if (status & SamplePlayer::kLooped) {
				sound.ticksElapsed = 0;
			}
			if (!(status & SamplePlayer::kPlaying)) {
				sound.isSample = false;
				sound.sampleTrackNo = 0;
				removeSoundFromPlaylist(sound);
				_needsRemap = true;
			}
		}
	}
}

Sci1SoundManager::SamplePlayer::SamplePlayer(SoundManager &manager, Audio::Mixer &mixer) :
	_manager(manager),
	_mixer(mixer),
	_playing(false) {}

Sci1SoundManager::SamplePlayer::~SamplePlayer() {
	_mixer.stopHandle(_handle);
}

void Sci1SoundManager::SamplePlayer::load(const Sci1Sound &sound) {
	if (sound.volume == 0 || _manager.getMasterVolume() == 0 || !_manager.isSoundEnabled()) {
		return;
	}

	const uint8 trackNo = sound.sampleTrackNo & 0xf;
	SciSpan<const byte> data = sound.resource->subspan(sound.tracks[trackNo].offset + 1);

	enum { kSampleMarker = Sci1Sound::Track::kSampleTrack };
	while (*data++ == kSampleMarker);

	_mixer.stopHandle(_handle);
	_loop = sound.loop;
	_playing = false;
	_pos = 8;
	_sampleRate = data.getUint16LEAt(0);
	_size = data.getUint16LEAt(2);
	_loopStart = data.getUint16LEAt(4);
	_loopEnd = data.getUint16LEAt(6);
	_data = data.subspan(0, _size);
	_mixer.playStream(Audio::Mixer::kSFXSoundType, &_handle, this, -1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO);
}

Sci1SoundManager::SamplePlayer::Status Sci1SoundManager::SamplePlayer::advance(const Sci1Sound &sound) {
	_loop = sound.loop;
	_playing = true;

	if (!_loop && _pos == _size) {
		_mixer.stopHandle(_handle);
		_playing = false;
		return kFinished;
	}

	return kPlaying;
}

void Sci1SoundManager::SamplePlayer::unload() {
	_mixer.stopHandle(_handle);
	_playing = false;
}

int Sci1SoundManager::SamplePlayer::readBuffer(int16 *buffer, const int numSamples) {
	if (!_playing) {
		return 0;
	}

	int samplesRead = 0;
	for (samplesRead = 0; samplesRead < numSamples; ++samplesRead) {
		if (_loop && _pos == _loopEnd) {
			_pos = _loopStart;
		}
		if (_pos == _size) {
			break;
		}

		*buffer++ = (_data[_pos++] << 8) ^ 0x8000;
	}
	return samplesRead;
}

#pragma mark -
#pragma mark Kernel

void Sci1SoundManager::kernelInit(const reg_t soundObj) {
	Common::StackLock lock(_mutex);

	const GuiResourceId resourceNo = readSelectorValue(_segMan, soundObj, SELECTOR(number));

	reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));
	Sci1Sound *sound;
	if (nodePtr.isNull()) {
		nodePtr = make_reg(kUninitializedSegment, _nextObjectId++);
		_sounds.push_back(Sci1Sound(soundObj, nodePtr));
		sound = &_sounds.back();
		writeSelector(_segMan, soundObj, SELECTOR(nodePtr), nodePtr);
	} else {
		kernelStop(soundObj);
		sound = findSoundByRegT(nodePtr);
		assert(sound != nullptr);
	}

	// SSCI1late- don't set `isSample` here and instead always set the sound
	// properties; for simplicity of implementation we just always follow the
	// SCI1.1 way, since it is compatible

	sound->isSample = (getSoundResourceType(resourceNo) == kResourceTypeAudio);

	if (!sound->isSample) {
		sound->loop = (readSelectorValue(_segMan, soundObj, SELECTOR(loop)) == 0xffff);
		sound->priority = readSelectorValue(_segMan, soundObj, SELECTOR(priority));
		sound->signal = Sci1Sound::kNoSignal;
		sound->cue = 0;
		sound->volume = readSelectorValue(_segMan, soundObj, SELECTOR(vol));
	}
}

void Sci1SoundManager::kernelDispose(const reg_t soundObj) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));
	kernelStop(nodePtr);
	if (!nodePtr.isNull()) {
		SoundsList::iterator it = findSoundIteratorByRegT(nodePtr);
		if (it != _sounds.end()) {
			_sounds.erase(it);
		}
	}
	writeSelector(_segMan, soundObj, SELECTOR(nodePtr), NULL_REG);
}

void Sci1SoundManager::kernelPlay(const reg_t soundObj, const bool exclusive) {
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
	// TODO: Figure out the exact SCI versions which did this, it is at least
	// SCI2.1early
	if (_soundVersion >= SCI_VERSION_2 &&
		sound->id.getType() == kResourceTypeAudio) {

		g_sci->_audio32->stop(sound->id, sound->nodePtr);
	}
#endif

	const GuiResourceId soundNo = getSoundResourceId(readSelectorValue(_segMan, soundObj, SELECTOR(number)));
	const ResourceId id(getSoundResourceType(soundNo), soundNo);
	sound->id = id;

	if (!readSelector(_segMan, soundObj, SELECTOR(handle)).isNull() &&
		(_soundVersion < SCI_VERSION_2 || !sound->isSample)) {

		kernelStop(soundObj);
	} else if (_soundVersion >= SCI_VERSION_2 && sound->isSample) {
		// SSCI did not store the resource pointer on the sound object in the
		// same way we do, so did not do this; this is only necessary in order
		// to avoid triggering the resource assertion below
		sound->resource = nullptr;
	}

	if (_soundVersion >= SCI_VERSION_1_1) {
		sound->isSample = (id.getType() == kResourceTypeAudio);
	}

	assert(!sound->resource);
	if (_soundVersion >= SCI_VERSION_1_1 && sound->isSample) {
		// SSCI32 would optionally preload audio if there was a preload flag in
		// the soundObj's `flags` selector; we do not need to worry about load
		// times, so we just don't do that
		sound->resource = _resMan.testResource(id);
	} else {
		sound->resource = _resMan.findResource(id, true);
	}

	if (_soundVersion >= SCI_VERSION_1_1 && !sound->resource) {
		writeSelectorValue(_segMan, soundObj, SELECTOR(signal), Kernel::kFinished);
		return;
	}

	// In SSCI the handle was assigned to the MemID returned by a call to
	// ResourceManager::Get, we do not allocate memory through SegManager for
	// resources so instead we just give the handle property a valid-ish pointer
	// and hope that games don't try to dereference the handle to try to read
	// raw MIDI data
	writeSelector(_segMan, soundObj, SELECTOR(handle), nodePtr);

	writeSelectorValue(_segMan, soundObj, SELECTOR(signal), Kernel::kNoSignal);
	writeSelectorValue(_segMan, soundObj, SELECTOR(min), 0);
	writeSelectorValue(_segMan, soundObj, SELECTOR(sec), 0);
	writeSelectorValue(_segMan, soundObj, SELECTOR(frame), 0);

	const bool loop = (readSelectorValue(_segMan, soundObj, SELECTOR(loop)) == 0xffff);
	const int16 volume = readSelectorValue(_segMan, soundObj, SELECTOR(vol));

	if (_soundVersion >= SCI_VERSION_1_MIDDLE &&
		(_soundVersion < SCI_VERSION_2 || !sound->isSample)) {

		sound->priority = readSelectorValue(_segMan, soundObj, SELECTOR(priority));
		sound->volume = volume;
		sound->loop = loop;
	}

	if (_soundVersion >= SCI_VERSION_1_1 && sound->isSample) {
		// SSCI set up fake VM arguments and made direct kernel calls here,
		// which is not very pleasant; we do normal calls into the audio
		// components instead
#ifdef ENABLE_SCI32
		if (_soundVersion >= SCI_VERSION_2) {
			g_sci->_audio32->play(g_sci->_audio32->findChannelById(id, soundObj), id, true, loop, volume, nodePtr, false);
		} else
#endif
			// TODO: This doesn't loop, it should loop, Sci::Audio does not have
			// the capability to do this, it should probably just be using
			// Audio32 which was actually RE'd properly
			g_sci->_audio->startAudio(kSfxModule, id.getNumber());
	} else {
		kernelUpdate(soundObj);
		play(*sound, exclusive);

		if (_soundVersion >= SCI_VERSION_1_MIDDLE) {
			writeSelectorValue(_segMan, soundObj, SELECTOR(priority), sound->priority);
		}
	}
}

void Sci1SoundManager::kernelStop(const reg_t soundObj) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));
	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (sound) {
		if (_soundVersion >= SCI_VERSION_1_1 && sound->isSample) {
			// SSCI set up fake VM arguments and made direct kernel calls here,
			// which is not very pleasant; we do normal calls into the audio
			// components instead
#ifdef ENABLE_SCI32
			if (_soundVersion >= SCI_VERSION_2) {
				g_sci->_audio32->stop(sound->id, soundObj);
			} else
				// TODO: This should be accepting a sound number so that if
				// another sound replaced the original sound it does not get
				// stopped (this is how SSCI worked).
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

int16 Sci1SoundManager::kernelPause(const reg_t soundObj) {
	error("Attempt to call SCI0 kernel pause on Sci1SoundManager");
}

void Sci1SoundManager::kernelPause(const reg_t soundObj, const int16 numPauses, const bool pauseDac) {
	Common::StackLock lock(_mutex);

	const bool shouldPause = (numPauses != 0);

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
				const ResourceId id(kResourceTypeAudio, readSelectorValue(_segMan, soundObj, SELECTOR(number)));
				if (shouldPause) {
					g_sci->_audio32->pause(id, nodePtr);
				} else {
					g_sci->_audio32->resume(id, nodePtr);
				}
			} else
#endif
				if (_soundVersion <= SCI_VERSION_1_EARLY) {
					pause(*sound, uint8(numPauses));
				} else {
					pause(*sound, shouldPause);
				}
		}
	}
}

void Sci1SoundManager::kernelFade(const reg_t soundObj) {
	error("Attempt to call SCI0 kernel pause on Sci1SoundManager");
}

void Sci1SoundManager::kernelFade(const reg_t soundObj, const int16 targetVolume, const int16 speed, const int16 steps, const bool stopAfterFade) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));

	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (!sound) {
		return;
	}

#ifdef ENABLE_SCI32
	if (_soundVersion >= SCI_VERSION_2 && sound->isSample) {
		const ResourceId id(kResourceTypeAudio, readSelectorValue(_segMan, soundObj, SELECTOR(number)));
		g_sci->_audio32->fadeChannel(id, nodePtr, targetVolume, speed, steps, stopAfterFade);
	} else
#endif
		fade(*sound, targetVolume, speed, steps, stopAfterFade);
}

void Sci1SoundManager::kernelHold(const reg_t soundObj, const int16 holdPoint) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));
	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (sound) {
		hold(*sound, holdPoint);
	}
}

void Sci1SoundManager::kernelSetVolume(const reg_t soundObj, const int16 volume) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));
	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (!sound) {
		return;
	}

#ifdef ENABLE_SCI32
	if (_soundVersion >= SCI_VERSION_2 && sound->isSample) {
		const ResourceId id(kResourceTypeAudio, readSelectorValue(_segMan, soundObj, SELECTOR(number)));
		g_sci->_audio32->setVolume(id, nodePtr, volume);
	}
#endif
	if (sound->volume != volume) {
		setVolume(*sound, volume);
		writeSelectorValue(_segMan, soundObj, SELECTOR(vol), volume);
		_guestAdditions.kDoSoundSetVolumeHook(soundObj, volume);
	}
}

void Sci1SoundManager::kernelSetPriority(const reg_t soundObj, const int16 priority) {
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

void Sci1SoundManager::kernelSetLoop(const reg_t soundObj, const bool enable) {
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
	if (_soundVersion >= SCI_VERSION_2_1_MIDDLE && sound->isSample) {
		const ResourceId id(kResourceTypeAudio, readSelectorValue(_segMan, soundObj, SELECTOR(number)));
		g_sci->_audio32->setLoop(id, nodePtr, enable);
	} else
#endif
		sound->loop = enable;
}

void Sci1SoundManager::kernelUpdateCues(const reg_t soundObj) {
	Common::StackLock lock(_mutex);

	const reg_t nodePtr = readSelector(_segMan, soundObj, SELECTOR(nodePtr));

	Sci1Sound *sound = findSoundByRegT(nodePtr);
	if (!sound) {
		return;
	}

	if (_soundVersion >= SCI_VERSION_1_1 && sound->isSample) {
		int position;
#ifdef ENABLE_SCI32
		if (_soundVersion >= SCI_VERSION_2) {
			const ResourceId id(kResourceTypeAudio, readSelectorValue(_segMan, soundObj, SELECTOR(number)));
			position = g_sci->_audio32->getPosition(id, nodePtr);

			if (_soundVersion == SCI_VERSION_3) {
				// In SSCI the volume is first set to -1 and then reset later if
				// a sample is playing in the audio player, but since our audio
				// code returns -1 for not-found samples, the extra check is not
				// needed and we can just always set it to the return value of
				// the getVolume call
				const int16 volume = g_sci->_audio32->getVolume(id, nodePtr);
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
		if (_soundVersion > SCI_VERSION_1_EARLY) {
			writeSelectorValue(_segMan, soundObj, SELECTOR(vol), sound->volume);
		}
	}
}

void Sci1SoundManager::kernelSendMidi(const int argc, const reg_t *const argv) {
	const reg_t soundObj = argv[0];
	const int16 channelNo = argv[1].toSint16();

	if (_soundVersion < SCI_VERSION_1_LATE) {
		enum { kPitchBendCommand = 0xff };

		const int16 a = argv[2].toSint16();
		const int16 b = argv[3].toSint16();

		if (a == kPitchBendCommand) {
			sendMidi(soundObj, channelNo, kPitchBend, b, 0);
		} else {
			sendMidi(soundObj, channelNo, kControllerChange, a, b);
		}
	} else {
		sendMidi(soundObj, channelNo, argv[2].toSint16(), argv[3].toSint16(), argv[4].toSint16());
	}
}

void Sci1SoundManager::kernelUpdate(const reg_t soundObj) {
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

reg_t Sci1SoundManager::kernelGlobalReverb(const int argc, const reg_t *const argv) {
	// In SSCI, these checks were in the single SetReverb function; since we
	// split the function for clarity, the checks are moved here instead
	if (argc == 0 || argv[0].toSint16() == 0xff) {
		return make_reg(0, getReverbMode());
	} else if (argv[0].toUint16() > 10) {
		return make_reg(0, getDefaultReverbMode());
	} else {
		return make_reg(0, setReverbMode(argv[0].toSint16()));
	}
}

void Sci1SoundManager::sendMidi(const reg_t soundObj, int16 channel, const int16 command, int16 a, int16 b) {
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
		b = CLIP<int16>(b, 0, 127);
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
		// In SCI1late, the MIDI ASM code accepted the signed value and
		// converted it; in SCI1.1+ the value was converted here. TODO: Change
		// this to the SCI1late mode to eliminate unnecessary arithmetic
		setPitchBend(*sound, channel, a + 0x2000);
		break;
	}
}

#pragma mark -
#pragma mark Debugging

void Sci1SoundManager::debugPrintPlaylist(Console &con) const {
	Common::StackLock lock(_mutex);

	for (uint i = 0; i < _playlist.size(); ++i) {
		if (!_playlist[i]) {
			if (_soundVersion <= SCI_VERSION_1_EARLY && i == 0) {
				con.debugPrintf(" 0: no exclusive sound\n");
				continue;
			} else {
				break;
			}
		}

		const Sci1Sound &sound = *_playlist[i];
		con.debugPrintf("%2d: ", i);
		debugPrintSound(con, sound);
	}
}

void Sci1SoundManager::debugPrintSound(Console &con, const uint index) const {
	Common::StackLock lock(_mutex);

	if (index >= _playlist.size() || !_playlist[index]) {
		con.debugPrintf("Index out of range\n");
		return;
	}

	const Sci1Sound &sound = *_playlist[index];

	debugPrintSound(con, sound);

	con.debugPrintf("\nTracks:\n");

	for (uint i = 0; i < sound.tracks.size(); ++i) {
		const Sci1Sound::Track &track = sound.tracks[i];
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

	for (uint i = 0; i < sound.channels.size(); ++i) {
		const Sci1Sound::Channel &channel = sound.channels[i];
		con.debugPrintf("%2d: priority %d, voices %u, note %u, volume %u\n",
						i, channel.priority, channel.numVoices, channel.currentNote, channel.volume);
		con.debugPrintf("    program %u, mod %u, pan %u, p bend %u\n",
						channel.program, channel.modulation, channel.pan, channel.pitchBend);
		con.debugPrintf("    dp %d, flags %u, mute %d, game mutes %u\n",
						channel.damperPedalOn, channel.flags, channel.muted, channel.gameMuteCount);
	}
}

void Sci1SoundManager::debugPrintSound(Console &con, const Sci1Sound &sound) const {
	con.debugPrintf("%s, %s, %u pauses\n",
					sound.resource ? sound.resource->name().c_str() : "<none>",
					sound.state == Sci1Sound::kStopped ? "stopped" : "playing",
					sound.numPauses);
	con.debugPrintf("    cue %u, hold point %u, loop %d\n",
					sound.cue, sound.holdPoint, sound.loop);
	con.debugPrintf("    signal %d, state %d, priority %d%s\n",
					sound.signal, sound.state, sound.priority,
					sound.fixedPriority ? " (fixed)" : "");
	con.debugPrintf("    ticks elapsed %u, reverb mode %u, volume %d",
					sound.ticksElapsed, sound.reverbMode, sound.volume);
	if (sound.fadeAmountPerTick) {
		con.debugPrintf("     -> %d\n", sound.fadeTargetVolume);
		con.debugPrintf("    fade delay %d, speed %d, stop %d\n",
						sound.fadeDelay, sound.fadeAmountPerTick, sound.stopSoundOnFade);
	} else {
		con.debugPrintf("\n");
	}
}

void Sci1SoundManager::debugPrintChannelMap(Console &con) const {
	Common::StackLock lock(_mutex);
	debugPrintChannelMap(con, _hardwareChannels);
}

void Sci1SoundManager::debugPrintDriverState(Console &con) const {
	Common::StackLock lock(_mutex);
	_driver->debugPrintState(con);
}

void Sci1SoundManager::debugStopAll() {
	Common::StackLock lock(_mutex);
	for (uint i = 0; i < _playlist.size(); ++i) {
		VALIDATE_PLAYLIST_ITERATOR
		stop(*_playlist[i]);
	}
}

void Sci1SoundManager::debugPlaySound(Console &con, const GuiResourceId resourceNo, const bool exclusive) {
	const ResourceId id(kResourceTypeSound, resourceNo);
	if (!_resMan.testResource(id)) {
		con.debugPrintf("Could not find %s\n", id.toString().c_str());
		return;
	}

	if (getSoundResourceType(resourceNo) != kResourceTypeSound) {
		con.debugPrintf("SCI1.1+ digital audio playback not supported\n");
		return;
	}

	const reg_t nodePtr = make_reg(kUninitializedSegment, _nextObjectId++);
	_sounds.push_back(Sci1Sound(NULL_REG, nodePtr));
	Sci1Sound &sound = _sounds.back();
	sound.id = id;
	sound.resource = _resMan.findResource(id, true);
	assert(sound.resource);
	play(sound, exclusive);
}

void Sci1SoundManager::debugPrintChannelMap(Console &con, const HardwareChannels &channels) const {
	Common::StackLock lock(_mutex);
	for (uint i = 0; i < channels.size(); ++i) {
		const HardwareChannel &channel = channels[i];
		if (channel.isMapped()) {
			const Resource *resource = _playlist[channel.playlistIndex()]->resource;
			con.debugPrintf("%2d: %s ch %2d pr %3d vo %2d%s\n",
							i,
							resource ? resource->name().c_str() : "<none>",
							channel.channelNo(),
							channel.priority,
							channel.numVoices,
							channel.locked ? ", locked" : "");
		} else {
			con.debugPrintf("%2d: unmapped\n", i);
		}
	}
}

} // End of namespace Sci