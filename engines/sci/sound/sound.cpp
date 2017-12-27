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
	_mixer(*g_system->getMixer()),
	_soundVersion(features.detectDoSoundType()),
	_numServerSuspensions(0),
	_needsUpdate(false),
	_sample(nullptr),
	_newChannelVolumes(kNumMappedChannels, -1),
	_newChannelVolumesIndex(0),
	_channelList(kNumMappedChannels, 0xff),
	_outOfRangeChannel(false),
	_defaultReverbMode(0),
	_mappedChannels(kNumMappedChannels),
	_mappedNodes(kNumMappedChannels),
	_restoringSaveGame(false) {

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
		// TODO: Figure out why these device flags are handled this way, by
		// version
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

	g_system->getTimerManager()->installTimerProc(soundServerCallback, 1000000 / 60, this, "SCI MIDI");
}

SoundManager::~SoundManager() {
	g_system->getTimerManager()->removeTimerProc(soundServerCallback);
}

void SoundManager::enableSoundServer(const bool enable) {
	Common::StackLock lock(_mutex);
	if (!enable) {
		++_numServerSuspensions;
	} else if (_numServerSuspensions) {
		--_numServerSuspensions;
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

		if (sound.fadeSteps) {
			fadeSound(sound);

			if (sound.signal == Sci1Sound::kFinished) {
				// TODO: There was some extra work done here to
				// rewind the iterator because it would be advanced
				// again, this seems questionable since it would mean
				// that doFade removed the sound from the playlist but
				// did not clear the memory for the Sound object
				continue;
			}
		}

		parseNextNode(sound, i);

		if (sound.signal != Sci1Sound::kFinished) {
			// TODO: Same thing with finished signal as above, the it
			// would only be advanced for unfinished Sounds
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
		int8 minChannelNo, maxChannelNo;
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
				if (track.channelNo == Sci1Sound::Track::kEndOfTracks ||
					track.channelNo == Sci1Sound::Track::kSampleTrack ||
					track.channelNo == kControlChannel) {

					continue;
				}

				Sci1Sound::Channel &channel = sound.channels[track.channelNo];

				if ((channel.flags & Sci1Sound::kOutOfRange) || channel.muted) {
					continue;
				}

				const uint8 key = (i << 4) | track.channelNo;

				uint8 priority = channel.priority;
				if (priority != 0) {
					priority = kNumMappedChannels - priority + basePriority;
				}

				int8 bestRemapIndex;
				if ((channel.flags & Sci1Sound::kLocked) &&
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
					if (channel.polyphony > numFreeVoices) {
						if (priority == 0) {
							do {
								bestRemapIndex = preemptChannel(numFreeVoices);
								if (bestRemapIndex == kUnknownChannel) {
									_mappedChannels = oldMappedChannels;
									numFreeVoices = oldNumFreeVoices;
									goto nextSound;
								}
							} while (channel.polyphony > numFreeVoices);
						} else {
							goto nextTrack;
						}
					}

					// putChOnList
					assert(bestRemapIndex >= 0 && bestRemapIndex < kNumMappedChannels);
					MappedChannel &mc = _mappedChannels[bestRemapIndex];
					mc.key = key;
					mc.numVoices = channel.polyphony;
					numFreeVoices -= channel.polyphony;
					mc.priority = priority;

					if (channel.flags & Sci1Sound::kLocked) {
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
										numFreeVoices -= channel.polyphony;
									}
								} else {
									mc = MappedChannel();
									numFreeVoices += channel.polyphony;
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

			int8 lastOpenChannelNo = -1;
			for (int j = maxChannelNo; j >= minChannelNo; --j) {
				if (_channelList[j] != MappedChannel::kUnmapped) {
					lastOpenChannelNo = j;
					break;
				}
			}
			assert(lastOpenChannelNo > -1);

			_channelList[lastOpenChannelNo] = mc.key;

			Sci1Sound *sound = _playlist[mc.playlistIndex()];
			assert(sound);
			updateChannel(*sound, sound->channels[mc.channelNo()], lastOpenChannelNo);
		}
	} else {
		// SSCI fills _channelList with 0xFFs here, but this was already just
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
			const int8 playlistIndex = _channelList[i] >> 4;
			_mappedNodes[i] = _playlist[playlistIndex];
		}
	}
}

int8 SoundManager::preemptChannel(const int &numFreeVoices) {
	// 'lowest' is weird here since these priority values are inverted, so the
	// biggest number is the lowest priority
	int8 lowestPriority = 0, lowestPriorityChannelNo = kUnknownChannel;
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
	msb = (word >> 7) & ~1;
	lsb = word & 0xFF;
	if (lsb & 0x80) {
		msb |= 1;
	}
}

static inline uint16 convert7To16(byte lsb, byte msb) {
	return (msb << 7) | lsb;
}

void SoundManager::updateChannel(const Sci1Sound &sound, const Sci1Sound::Channel &channel, const int8 hwChannelNo) {
	Sci1SoundDriver &driver = static_cast<Sci1SoundDriver &>(*_driver);
	driver.controllerChange(hwChannelNo, kAllNotesOffController, 0);
	driver.controllerChange(hwChannelNo, kMaxVoicesController, channel.polyphony);
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

void SoundManager::parseNextNode(Sci1Sound &sound, int8 playlistIndex) {
	_playlistIndex = playlistIndex << 4;
	++sound.ticksElapsed;

	assert(sound.resource);
	SciSpan<const byte> base = *sound.resource;

	bool allTracksEnded = false;
	for (int trackNo = 0; trackNo < Sci1Sound::kNumTracks; ++trackNo) {
		Sci1Sound::Track &track = sound.tracks[trackNo];
		if (track.channelNo == Sci1Sound::Track::kEndOfTracks) {
			allTracksEnded = true;
			break;
		}

		if (track.channelNo == Sci1Sound::Track::kSampleTrack) {
			continue;
		}

		Sci1Sound::Channel &channel = sound.channels[track.channelNo];

		int8 hwChannelNo = kUnknownChannel;
		bool outOfRangeChannel;
		if (channel.flags & Sci1Sound::kOutOfRange) {
			outOfRangeChannel = true;
			hwChannelNo = track.channelNo;
		} else {
			outOfRangeChannel = false;
			const uint8 key = _playlistIndex | track.channelNo;
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
			--track.rest;
			if (track.rest == kTimingOverflowFlag) {
				const byte message = sound.consume(trackNo);
				if (message == kTimingOverflow) {
					track.rest = kTimingOverflowFlag | 240;
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

			const Command command = Command(message & 0xF0);
			const int8 channelNo = message & 0x0F;

			if (message == kEndOfTrack) {
				track.position = 0;
				continue;
			}

			if (channelNo == kControlChannel) {
				parseControlChannel(sound);
				if (track.position == 0) {
					continue;
				}
			} else {
				switch (command) {
				case kNoteOff:
					sendNoteOff(sound, trackNo, hwChannelNo);
					break;
				case kNoteOn:
					sendNoteOn(sound, trackNo, hwChannelNo);
					break;
				case kKeyPressure:
					sendKeyPressure(sound, trackNo, hwChannelNo);
					break;
				case kControllerChange:
					sendControllerChange(sound, trackNo, hwChannelNo, outOfRangeChannel);
					break;
				case kProgramChange:
					sendProgramChange(sound, trackNo, hwChannelNo, outOfRangeChannel);
					break;
				case kChannelPressure:
					sendChannelPressure(sound, trackNo, hwChannelNo);
					break;
				case kPitchBend:
					sendPitchBend(sound, trackNo, hwChannelNo, outOfRangeChannel);
					break;
				case kSysEx:
					sendSysEx(sound, trackNo, hwChannelNo);
					break;
				default:
					warning("Unknown command %u in track %d", command, trackNo);
					track.position = 0;
					goto nextTrack;
				}
			}
		} while ((message = sound.consume(trackNo)) == 0);

		if (message == kTimingOverflow) {
			track.rest = kTimingOverflowFlag | 240;
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

	if (sound.hold || sound.loop) {
		sound.ticksElapsed = sound.loopTicksElapsed;
		for (int i = 0; i < Sci1Sound::kNumTracks; ++i) {
			Sci1Sound::Track &track = sound.tracks[i];
			track.position = track.loopPosition;
			track.rest = track.loopRest;
			track.command = track.loopCommand;
		}
	} else {
		finishSound(sound);
		_needsUpdate = true;
	}
}

void SoundManager::sendNoteOff(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo) {
	const int8 note = sound.consume(trackNo);
	const int8 velocity = sound.consume(trackNo);

	Sci1Sound::Channel &channel = sound.channels[sound.tracks[trackNo].channelNo];
	if (channel.currentNote == note) {
		channel.currentNote = kNoCurrentNode;
	}

	if (hwChannelNo != kUnknownChannel && !_restoringSaveGame) {
		_driver->noteOff(hwChannelNo & 0xf, note, velocity);
	}
}

void SoundManager::sendNoteOn(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo) {
	if (sound.peek(trackNo, 1) == 0) {
		return sendNoteOff(sound, trackNo, hwChannelNo);
	}

	const int8 note = sound.consume(trackNo);
	const int8 velocity = sound.consume(trackNo);

	sound.channels[sound.tracks[trackNo].channelNo].currentNote = note;

	if (hwChannelNo != kUnknownChannel && !_restoringSaveGame) {
		_driver->noteOn(hwChannelNo & 0xf, note, velocity);
	}
}

void SoundManager::sendKeyPressure(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo) {
	const int8 note = sound.consume(trackNo);
	const int8 pressure = sound.consume(trackNo);

	if (hwChannelNo != kUnknownChannel && !_restoringSaveGame) {
		_driver->keyPressure(hwChannelNo, note, pressure);
	}
}

void SoundManager::sendControllerChange(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo, const bool outOfRangeChannel) {
	const int8 controllerNo = sound.consume(trackNo);
	int8 value = sound.consume(trackNo);

	const int8 inRangeChannelNo = hwChannelNo & 0xf;

	if (outOfRangeChannel && _channelList[inRangeChannelNo] != MappedChannel::kUnmapped) {
		return;
	}

	Sci1Sound::Channel &channel = sound.channels[sound.tracks[trackNo].channelNo];

	switch (controllerNo) {
	case kVolumeController:
		// SSCI treated hwChannelNo as unsigned here so had only a single check
		if (hwChannelNo < 0 || hwChannelNo >= kNumMappedChannels * 2) {
			return;
		}

		// There appears to be a masking error in SSCI where out of range
		// channels would write out of bounds into the save mute flags (they
		// used 0xff instead of 0xf); we do not do that, this might cause volume
		// issues since any pending volumes would have been applied again in
		// SSCI and won't here.
		if (hwChannelNo >= kNumMappedChannels) {
			warning("Out of range volume change applied to channel %d", hwChannelNo);
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
		channel.polyphony = value & 0xf;
		break;
	case kSingleVoiceNoteController:
		channel.muted = (value != 0);
		if (channel.muted) {
			value = 1;
		}
		break;
	}

	if (hwChannelNo != kUnknownChannel && !_restoringSaveGame) {
		_driver->controllerChange(inRangeChannelNo, controllerNo, value);
	}
}

void SoundManager::sendProgramChange(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo, const bool outOfRangeChannel) {
	const int8 programNo = sound.consume(trackNo);
	const int8 inRangeChannelNo = hwChannelNo & 0xf;

	if (outOfRangeChannel && _channelList[inRangeChannelNo] != MappedChannel::kUnmapped) {
		return;
	}

	sound.channels[sound.tracks[trackNo].channelNo].program = programNo;

	if (hwChannelNo != kUnknownChannel && !_restoringSaveGame) {
		_driver->programChange(inRangeChannelNo, programNo);
	}
}

void SoundManager::sendChannelPressure(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo) {
	const int8 pressure = sound.consume(trackNo);

	if (hwChannelNo != kUnknownChannel && !_restoringSaveGame) {
		// In SSCI the channel was not masked here, unlike in other functions
		_driver->channelPressure(hwChannelNo, pressure);
	}
}

void SoundManager::sendPitchBend(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo, const bool outOfRangeChannel) {
	const int8 lsb = sound.consume(trackNo);
	const int8 msb = sound.consume(trackNo);

	const int8 inRangeChannelNo = hwChannelNo & 0xf;

	if (outOfRangeChannel && _channelList[inRangeChannelNo] != MappedChannel::kUnmapped) {
		return;
	}

	sound.channels[sound.tracks[trackNo].channelNo].pitchBend = convert7To16(lsb, msb);

	if (hwChannelNo != kUnknownChannel && !_restoringSaveGame) {
		// In SSCI the channel was not masked here, unlike in other functions
		_driver->pitchBend(inRangeChannelNo, lsb, msb);
	}
}

void SoundManager::sendSysEx(Sci1Sound &sound, const int8 trackNo, const int8 hwChannelNo) {
	skipCommand(sound, trackNo, kSysEx);
}

void SoundManager::skipCommand(Sci1Sound &sound, const int8 trackNo, const Command command) {
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

void SoundManager::updateChannelVolumes() {
	int8 &channelNo = _newChannelVolumesIndex;
	const int8 originalChannel = channelNo;
	int numUpdates = 0;
	do {
		int8 &volume = _newChannelVolumes[channelNo];
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
