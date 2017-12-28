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
#include "sci/engine/features.h"
#include "sci/sound/sound.h"
#include "sci/sound/drivers/adlib.h"

namespace Sci {

SoundManager::SoundManager(ResourceManager &resMan, GameFeatures &features) :
	_resMan(resMan),
	_soundVersion(features.detectDoSoundType()),
	_numServerSuspensions(0),
	_needsUpdate(false),
	_sample(nullptr),
	_newChannelVolumes(kNumMappedChannels, -1),
	_newChannelVolumesIndex(0),
	_channelList(kNumMappedChannels, kUnknownChannel),
	_defaultReverbMode(0),
	_mappedChannels(kNumMappedChannels),
	_mappedNodes(kNumMappedChannels),
	_restoringSound(false) {
	_preferSampledSounds = _soundVersion >= SCI_VERSION_2 ||
		g_sci->getGameId() == GID_GK1DEMO ||
		ConfMan.getBool("prefer_digitalsfx");

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

	// In SSCI, this is in STerminate; since we do not implement that operation,
	// we perform its additional termination operations here
	if (_driver) {
		_driver->setMasterVolume(kMaxMasterVolume);
	}
}

void SoundManager::enableSoundServer(const bool enable) {
	Common::StackLock lock(_mutex);
	if (!enable) {
		++_numServerSuspensions;
	} else if (_numServerSuspensions) {
		--_numServerSuspensions;
	}
}

void SoundManager::setSoundOn(const bool enable) {
	Common::StackLock lock(_mutex);
	_driver->enable(enable);
}

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
	if (reverbMode == 0xff) {
		return getReverbMode();
	}

	if (reverbMode > 10) {
		return getDefaultReverbMode();
	}

	uint8 oldReverbMode = _defaultReverbMode;
	_defaultReverbMode = reverbMode;
	if (_playlist[0]) {
		oldReverbMode = _driver->setReverbMode(reverbMode);
	}
	return oldReverbMode;
}

void SoundManager::stop(Sci1Sound &sound) {
	Common::StackLock lock(_mutex);
	removeSoundFromPlaylist(sound);
	updateChannelList();
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

void SoundManager::fixupHeader(Sci1Sound &sound) {
	// SSCI checked here to see if the pointer to data was 0xffffffff. This is
	// impossible so the check is omitted.

	SciSpan<const byte> data = *sound.resource;

	if (data[33] == 0xfc && data[34] == 0xfd && data[35] == 0xfe) {
		return;
	}

	uint8 priority = 0xff;

	if (data[0] == 0xf0) {
		priority = data[1];
		data += 8;
	}

	uint16 trackOffsets[16] = {};

	for (uint8 deviceId = *data++; deviceId != 0xff; deviceId = *data++) {
		if (deviceId == _driver->getDeviceId()) {
			int trackNo = 0;
			for (uint8 endMarker = *data; endMarker != 0xff; endMarker = *data) {
				assert(trackNo < 16);
				// TODO: Could be SCI-endian
				trackOffsets[trackNo++] = data.getUint16LEAt(2);
				data += 6;
			}
		} else {
			if (*data != 0xff) {
				data += 6;
			}
		}
	}

	// writeHeader

	// :(
	// TODO: Fix the MIDI code to receive this extra header information without
	// requiring such hacks.
	byte *header = const_cast<byte *>(sound.resource->getUnsafeDataAt(0, sizeof(trackOffsets) + 4));
	memcpy(header, trackOffsets, sizeof(trackOffsets));
	header += sizeof(trackOffsets);
	*header++ = priority;
	*header++ = 0xfc;
	*header++ = 0xfd;
	*header++ = 0xfe;
}

uint8 SoundManager::play(Sci1Sound &sound, const bool isBedSound) {
	if (findPlaylistIndex(sound) != kPlaylistSize) {
		removeSoundFromPlaylist(sound);
	}

	sound.state = isBedSound ? Sci1Sound::kExclusive : Sci1Sound::kPlaying;

	fixupHeader(sound);

	// TODO: Use proper initialisers in Sci1Sound?
	Common::fill(sound.channels, sound.channels + Sci1Sound::kNumChannels, Sci1Sound::Channel());
	Common::fill(sound.tracks, sound.tracks + Sci1Sound::kNumTracks, Sci1Sound::Track());
	sound.isSample = false;
	sound.holdPoint = 0;
	sound.reverbMode = kDefaultReverbMode;
	sound.loopTicksElapsed = 0;
	sound.volume = kMaxVolume;

	const uint8 priority = sound.resource->getUint8At(32);
	if (priority != 0xff && !sound.fixedPriority) {
		sound.priority = priority;
	}

	// ecx = resource base pointer
	SciSpan<const byte> base = *sound.resource;

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
				kOutOfRangeChannelFlag = 0x10,
				kLockedChannelFlag     = 0x20,
				kMutedChannelFlag      = 0x40
			};

			if (channelNo & kOutOfRangeChannelFlag) {
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

void SoundManager::fade(Sci1Sound &sound, const int16 targetVolume, const int16 speed, const int16 steps, const bool stopAfterFade) {
	Common::StackLock lock(_mutex);
	if (sound.volume == targetVolume) {
		return;
	}

	// TODO: In SSCI stop flag was baked into the high bit of target volume
	sound.fadeTargetVolume = targetVolume;
	sound.stopSoundOnFade = stopAfterFade;
	sound.fadeAmountPerTick = steps;
	sound.fadeDelay = speed;
	sound.fadeDelayRemaining = 0;
}

void SoundManager::hold(Sci1Sound &sound, const uint8 holdPoint) {
	Common::StackLock lock(_mutex);
	sound.holdPoint = holdPoint;
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

	for (int hwChannelNo = 0; hwChannelNo < kNumMappedChannels; ++hwChannelNo) {
		// TODO: kUnknownChannel probably wrong constant to use here
		if (_channelList[hwChannelNo] != kUnknownChannel && (_channelList[hwChannelNo] & 0xf0) == playlistIndex) {
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
			_channelList[channelNo] == MappedChannel::kUnmapped) {
			changeChannelVolume(sound, channelNo, channelNo, enqueue);
		}
	}
}

void SoundManager::changeChannelVolume(const Sci1Sound &sound, const uint8 channelNo, const uint8 hwChannelNo, const bool enqueue) {
	const uint8 channelVolume = sound.channels[channelNo].volume * sound.volume / kMaxVolume;
	if (enqueue) {
		_newChannelVolumes[hwChannelNo] = channelVolume;
	} else {
		_newChannelVolumes[hwChannelNo] = kNoVolumeChange;
		_driver->controllerChange(hwChannelNo, kVolumeController, channelVolume);
	}
}

void SoundManager::saveLoadWithSerializer(Common::Serializer &s) {
	error("TODO: Save/load for new sound system");
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

		if (sound.signal != Sci1Sound::kFinished) {
			--i;
		}
	}

	updateChannelVolumes();
	_driver->service();
}

void SoundManager::updateChannelList() {
	_needsUpdate = false;
	Common::fill(_channelList.begin(), _channelList.end(), MappedChannel::kUnmapped);
	Common::fill(_mappedChannels.begin(), _mappedChannels.end(), MappedChannel());

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
		if (reverbMode == kDefaultReverbMode) {
			reverbMode = _defaultReverbMode;
		}
		_driver->setReverbMode(reverbMode);

		MappedChannels oldMappedChannels;
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

			oldMappedChannels = _mappedChannels;
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
					priority = kNumMappedChannels - priority + basePriority;
				}

				uint8 bestRemapIndex;
				if ((channel.flags & Sci1Sound::Channel::kLocked) &&
					_mappedChannels[track.channelNo].key == MappedChannel::kUnmapped) {
					bestRemapIndex = track.channelNo;
				} else {
					bestRemapIndex = kUnknownChannel;
					for (int channelNo = 0; channelNo < kNumMappedChannels; ++channelNo) {
						MappedChannel &mappedChannel = _mappedChannels[channelNo];
						if (mappedChannel.key == MappedChannel::kUnmapped) {
							if (channelNo >= minChannelNo && channelNo <= maxChannelNo) {
								bestRemapIndex = channelNo;
							}
						} else if (mappedChannel.key == key) {
							goto nextTrack;
						}
					}

					if (bestRemapIndex == kUnknownChannel) {
						if (priority != 0) {
							goto nextSound;
						}

						bestRemapIndex = preemptChannel(numFreeVoices);
					}
				}

				if (bestRemapIndex == kUnknownChannel) {
					_mappedChannels = oldMappedChannels;
					numFreeVoices = oldNumFreeVoices;
					goto nextSound;
				} else {
					// checkVoices
					if (channel.numVoices > numFreeVoices) {
						if (priority == 0) {
							do {
								bestRemapIndex = preemptChannel(numFreeVoices);
								if (bestRemapIndex == kUnknownChannel) {
									_mappedChannels = oldMappedChannels;
									numFreeVoices = oldNumFreeVoices;
									goto nextSound;
								}
							} while (channel.numVoices > numFreeVoices);
						} else {
							goto nextTrack;
						}
					}

					// putChOnList
					assert(bestRemapIndex < kNumMappedChannels);
					MappedChannel &mc = _mappedChannels[bestRemapIndex];
					mc.key = key;
					mc.numVoices = channel.numVoices;
					numFreeVoices -= channel.numVoices;
					mc.priority = priority;

					if (channel.flags & Sci1Sound::Channel::kLocked) {
						if (bestRemapIndex != track.channelNo) {
							if (_mappedChannels[track.channelNo].locked) {
								if (priority == 0) {
									if (_mappedChannels[track.channelNo].priority == 0) {
										_mappedChannels = oldMappedChannels;
										numFreeVoices = oldNumFreeVoices;
										goto nextSound;
									} else {
										numFreeVoices += _mappedChannels[track.channelNo].numVoices;
										_mappedChannels[track.channelNo] = mc;
										mc = MappedChannel();
										numFreeVoices -= channel.numVoices;
									}
								} else {
									mc = MappedChannel();
									numFreeVoices += channel.numVoices;
								}
							} else {
								SWAP(_mappedChannels[track.channelNo], mc);
							}
						}
					} else {
						mc.locked = false;
					}
				}

				nextTrack:
				;
			}

			nextSound:
			;
		}

		// pass 2
		for (int channelNo = 0; channelNo < kNumMappedChannels; ++channelNo) {
			MappedChannel &mc = _mappedChannels[channelNo];
			if (mc.key == MappedChannel::kUnmapped) {
				continue;
			}

			Sci1Sound *sound = _playlist[mc.playlistIndex()];
			assert(sound);

			if (mc.locked) {
				// copyBedCh
				_channelList[channelNo] = mc.key;
				mc.key = MappedChannel::kUnmapped;

#ifdef ENABLE_SCI32
				// TODO: Determine precise version, this exists in at least SQ6.
				if (_soundVersion >= SCI_VERSION_2) {
					mc.priority = oldMappedChannels[channelNo].priority;
				}
#endif

				// SSCI stored the old channel list separately premasked so used
				// a direct equality comparison; we do not, so need to mask out
				// the playlist index here.
				if (mc.channelNo() == (oldChannelList[channelNo] & 0xf) &&
					_mappedNodes[channelNo] == _playlist[mc.playlistIndex()]) {
					continue;
				}

				updateChannel(*sound, sound->channels[mc.channelNo()], channelNo);
			} else {
				// noCopyBedCh
				for (int innerChannelNo = minChannelNo; innerChannelNo < maxChannelNo; ++innerChannelNo) {
					if (_mappedNodes[innerChannelNo] == sound &&
						// SSCI stored the old channel list separately premasked
						// so used a direct equality comparison; we do not, so
						// need to mask out the playlist index here.
						(oldChannelList[innerChannelNo] & 0xf) == mc.channelNo()) {

						if (!mc.locked) {
							_channelList[innerChannelNo] = mc.key;
							mc.key = MappedChannel::kUnmapped;
						}

						break;
					}
				}
			}
		}

		// pass 3
		for (int i = 0; i < kNumMappedChannels; ++i) {
			MappedChannel &mc = _mappedChannels[i];
			if (mc.key == MappedChannel::kUnmapped) {
				continue;
			}

			uint8 lastOpenChannelNo = kUnknownChannel;
			for (int j = maxChannelNo; j >= minChannelNo; --j) {
				if (_channelList[j] != MappedChannel::kUnmapped) {
					lastOpenChannelNo = j;
					break;
				}
			}
			assert(lastOpenChannelNo != kUnknownChannel);

			_channelList[lastOpenChannelNo] = mc.key;

			Sci1Sound *sound = _playlist[mc.playlistIndex()];
			assert(sound);
			updateChannel(*sound, sound->channels[mc.channelNo()], lastOpenChannelNo);
		}
	} else {
		// SSCI fills _channelList with 0xffs here, but this was already just
		// done above, so we don't do anything extra.
	}

	// cleanupChnls
	for (int i = kNumMappedChannels - 1; i >= 0; --i) {
		if (oldChannelList[i] != MappedChannel::kUnmapped && _channelList[i] == MappedChannel::kUnmapped) {
			_driver->controllerChange(i, kDamperPedalController, 0);
			_driver->controllerChange(i, kAllNotesOffController, 0);
			_driver->controllerChange(i, kMaxVoicesController, 0);
		}
	}

	// In SSCI the old channel list was persisted here but we do not need to do
	// that since we can just make it stack-local at the top of the function

	// TODO: Just make it so when a channel becomes unmapped it clears its state
	// so we do not need to do this
	for (int i = 0; i < kNumMappedChannels; ++i) {
		if (_channelList[i] == MappedChannel::kUnmapped) {
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
	uint8 lowestPriority = 0, lowestPriorityChannelNo = kUnknownChannel;
	for (int i = 0; i < kNumMappedChannels; ++i) {
		MappedChannel &mc = _mappedChannels[i];
		if (mc.priority >= lowestPriority) {
			lowestPriority = mc.priority;
			lowestPriorityChannelNo = i;
		}
	}

	if (lowestPriorityChannelNo != kUnknownChannel) {
		_mappedChannels[lowestPriorityChannelNo] = MappedChannel();
	}

	return lowestPriorityChannelNo;
}

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

void SoundManager::updateChannel(const Sci1Sound &sound, const Sci1Sound::Channel &channel, const uint8 hwChannelNo) {
	Sci1SoundDriver &driver = static_cast<Sci1SoundDriver &>(*_driver);
	driver.controllerChange(hwChannelNo, kAllNotesOffController, 0);
	driver.controllerChange(hwChannelNo, kMaxVoicesController, channel.numVoices);
	driver.programChange(hwChannelNo, channel.program);
	_newChannelVolumes[hwChannelNo] = kNoVolumeChange;
	driver.controllerChange(hwChannelNo, kVolumeController, channel.volume * sound.volume / kMaxVolume);
	driver.controllerChange(hwChannelNo, kPanController, channel.pan);
	driver.controllerChange(hwChannelNo, kModulationController, channel.modulation);
	driver.controllerChange(hwChannelNo, kDamperPedalController, channel.damperPedalOn ? 127 : 0);

	byte lsb, msb;
	convert16To7(channel.pitchBend, lsb, msb);
	driver.pitchBend(hwChannelNo, lsb, msb);

	driver.controllerChange(hwChannelNo, kSingleVoiceNoteController, channel.currentNote);
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

		uint8 hwChannelNo = kUnknownChannel;
		bool extraChannel;
		if (channel.flags & Sci1Sound::Channel::kExtra) {
			extraChannel = true;
			hwChannelNo = track.channelNo;
		} else {
			extraChannel = false;
			const uint8 key = (playlistIndex << 4) | track.channelNo;
			for (uint i = 0; i < kNumMappedChannels; ++i) {
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
				const byte message = sound.consume(trackNo);
				if (message == kTimingOverflow) {
					track.rest = kTimingOverflowValue;
				} else {
					track.rest = message;
				}
			}

			continue;
		}

		// parseCommand
		byte message;
		do {
			message = sound.peek(trackNo);
			if (message & kCommandFlag) {
				track.command = message;
				++track.position;
			} else {
				message = track.command;
			}

			const Command command = Command(message & 0xf0);
			const uint8 channelNo = message & 0xf;

			if (message == kEndOfTrack) {
				track.position = 0;
				continue;
			}

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

void SoundManager::processNoteOff(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	const uint8 note = sound.consume(trackNo);
	const uint8 velocity = sound.consume(trackNo);

	Sci1Sound::Channel &channel = sound.channels[sound.tracks[trackNo].channelNo];
	if (channel.currentNote == note) {
		channel.currentNote = Sci1Sound::Channel::kNoCurrentNote;
	}

	if (hwChannelNo != kUnknownChannel && !_restoringSound) {
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

	if (hwChannelNo != kUnknownChannel && !_restoringSound) {
		_driver->noteOn(hwChannelNo & 0xf, note, velocity);
	}
}

void SoundManager::processKeyPressure(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	const uint8 note = sound.consume(trackNo);
	const uint8 pressure = sound.consume(trackNo);

	if (hwChannelNo != kUnknownChannel && !_restoringSound) {
		// SSCI did not clamp the channel range here, see if this is a problem
		if (hwChannelNo >= kNumMappedChannels) {
			warning("Key pressure note %u pressure %u channel %u out of range", note, pressure, hwChannelNo);
		}

		_driver->keyPressure(hwChannelNo, note, pressure);
	}
}

void SoundManager::processControllerChange(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool outOfRangeChannel) {
	const uint8 controllerNo = sound.consume(trackNo);
	uint8 value = sound.consume(trackNo);

	const uint8 inRangeChannelNo = hwChannelNo & 0xf;

	if (outOfRangeChannel && _channelList[inRangeChannelNo] != MappedChannel::kUnmapped) {
		return;
	}

	Sci1Sound::Channel &channel = sound.channels[sound.tracks[trackNo].channelNo];

	switch (controllerNo) {
	case kVolumeController:
		if (hwChannelNo >= kNumMappedChannels * 2) {
			return;
		}

		// There appears to be a masking error in SSCI where out of range
		// channels would write out of bounds into the save mute flags (they
		// used 0xff instead of 0xf); we do not do that, this might cause volume
		// issues since any pending volumes would have been applied again in
		// SSCI and won't here.
		if (hwChannelNo >= kNumMappedChannels) {
			warning("Out of range volume change applied to channel %u", hwChannelNo);
		}

		_newChannelVolumes[inRangeChannelNo] = kNoVolumeChange;

		channel.volume = value;
		value = value * sound.volume / kMaxVolume;

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
	case kSingleVoiceNoteController:
		channel.muted = (value != 0);
		if (channel.muted) {
			value = 1;
		}
		break;
	}

	if (hwChannelNo != kUnknownChannel && !_restoringSound) {
		_driver->controllerChange(inRangeChannelNo, controllerNo, value);
	}
}

void SoundManager::processProgramChange(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool outOfRangeChannel) {
	const uint8 programNo = sound.consume(trackNo);
	const uint8 inRangeChannelNo = hwChannelNo & 0xf;

	if (outOfRangeChannel && _channelList[inRangeChannelNo] != MappedChannel::kUnmapped) {
		return;
	}

	sound.channels[sound.tracks[trackNo].channelNo].program = programNo;

	if (hwChannelNo != kUnknownChannel && !_restoringSound) {
		_driver->programChange(inRangeChannelNo, programNo);
	}
}

void SoundManager::processChannelPressure(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	const uint8 pressure = sound.consume(trackNo);

	if (hwChannelNo != kUnknownChannel && !_restoringSound) {
		// SSCI did not clamp the channel range here, see if this is a problem
		if (hwChannelNo >= kNumMappedChannels) {
			warning("Channel pressure pressure %u channel %u out of range", pressure, hwChannelNo);
		}

		_driver->channelPressure(hwChannelNo, pressure);
	}
}

void SoundManager::processPitchBend(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo, const bool outOfRangeChannel) {
	const uint8 lsb = sound.consume(trackNo);
	const uint8 msb = sound.consume(trackNo);

	const uint8 inRangeChannelNo = hwChannelNo & 0xf;

	if (outOfRangeChannel && _channelList[inRangeChannelNo] != MappedChannel::kUnmapped) {
		return;
	}

	sound.channels[sound.tracks[trackNo].channelNo].pitchBend = convert7To16(lsb, msb);

	if (hwChannelNo != kUnknownChannel && !_restoringSound) {
		_driver->pitchBend(inRangeChannelNo, lsb, msb);
	}
}

void SoundManager::processSysEx(Sci1Sound &sound, const uint8 trackNo, const uint8 hwChannelNo) {
	skipCommand(sound, trackNo, kSysEx);
}

void SoundManager::skipCommand(Sci1Sound &sound, const uint8 trackNo, const Command command) {
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

void SoundManager::setPitchBend(Sci1Sound &sound, const uint8 channelNo, const uint16 value) {
	Common::StackLock lock(_mutex);

	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	sound.channels[channelNo].pitchBend = value;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != kNumMappedChannels) {
		uint8 lsb, msb;
		convert16To7(value, lsb, msb);
		_driver->pitchBend(hwChannelNo, lsb, msb);
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
	if (hwChannelNo != kNumMappedChannels) {
		_driver->programChange(hwChannelNo, programNo);
	}
}

void SoundManager::setNoteOff(Sci1Sound &sound, const uint8 channelNo, const uint8 note, const uint8 velocity) {
	Common::StackLock lock(_mutex);

	const uint8 key = makeChannelKey(sound, channelNo);
	if (key == kUnknownSound) {
		return;
	}

	sound.channels[channelNo].currentNote = Sci1Sound::Channel::kNoCurrentNote;

	const uint8 hwChannelNo = findHwChannelNo(key);
	if (hwChannelNo != kNumMappedChannels) {
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
	if (hwChannelNo != kNumMappedChannels) {
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
		value = channel.volume * sound.volume / kMaxVolume;
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
	case kSingleVoiceNoteController:
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
	if (hwChannelNo != kNumMappedChannels) {
		if (controllerNo == kProgramChangeController) {
			_driver->programChange(hwChannelNo, value);
		} else {
			_driver->controllerChange(hwChannelNo, controllerNo, value);
		}
	}
}

void SoundManager::parseControlChannel(Sci1Sound &sound, const uint8 trackNo, const Command command) {
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
			if (value == kDefaultReverbMode) {
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

void SoundManager::setVolume(Sci1Sound &sound, const uint8 volume) {
	Common::StackLock lock(_mutex);
	processVolumeChange(sound, volume, false);
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

void SoundManager::updateChannelVolumes() {
	uint8 &channelNo = _newChannelVolumesIndex;
	const uint8 originalChannel = channelNo;
	int numUpdates = 0;
	do {
		uint8 &volume = _newChannelVolumes[channelNo];
		if (volume != kNoVolumeChange) {
			_driver->controllerChange(channelNo, kVolumeController, volume);
			++numUpdates;
			volume = kNoVolumeChange;
		}
		if (++channelNo == kNumMappedChannels) {
			channelNo = 0;
		}
	} while (numUpdates < 2 && channelNo != originalChannel);
}

} // end of namespace Sci
