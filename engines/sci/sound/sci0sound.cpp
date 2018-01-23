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

#include "sci/engine/kernel.h"
#include "sci/engine/selector.h"
#include "sci/sound/sci0sound.h"

namespace Sci {

Sci0SoundManager::Sci0SoundManager(ResourceManager &resMan, SegManager &segMan, GameFeatures &features, GuestAdditions &guestAdditions) :
	SoundManager(resMan, segMan, features, guestAdditions),
	_state(),
	_hardwareChannels(),
	_masterVolume(12),
	_lastNumServerSuspensions(0) {

	uint32 deviceFlags = MDT_PCSPK | MDT_PCJR | MDT_ADLIB | MDT_MIDI | MDT_CMS;

	const Common::Platform platform = g_sci->getPlatform();

	if (platform == Common::kPlatformFMTowns) {
		deviceFlags |= MDT_TOWNS;
	}

	const MidiDriver::DeviceHandle dev = MidiDriver::detectDevice(deviceFlags);
	const MusicType musicType = MidiDriver::getMusicType(dev);

	initDriver(musicType, platform);

	if (!_driver) {
		return;
	}

	g_system->getTimerManager()->installTimerProc(soundServerCallback, 1000000 / 60, this, "SCI MIDI");
}

Sci0SoundManager::~Sci0SoundManager() {
	if (!_driver) {
		return;
	}

	g_system->getTimerManager()->removeTimerProc(soundServerCallback);

	// Don't allow destruction to finish until after any in-progress sound
	// server callback has finished running
	Common::StackLock lock(_mutex);
}

void Sci0SoundManager::reset() {
	for (SoundsList::iterator sound = _sounds.begin(); sound != _sounds.end(); ++sound) {
		kernelStop(sound->soundObj);
	}
	_sounds.clear();
}

#pragma mark -
#pragma mark Save management

void Sci0SoundManager::reconstructPlaylist() {
	// SSCI loaded all the sound resources at once here, but we do not need to
	// do this preloading

	Sci0Sound *sound = findSoundByState<kStateActive>();
	if (!sound) {
		sound = findSoundByState<kStateBlocked>();
	}
	if (sound) {
		resume(*sound);
	}

	setSoundVolumes(isSoundEnabled() ? getMasterVolume() : 0);
}

#pragma mark -
#pragma mark MIDI server

void Sci0SoundManager::soundServer() {
	Common::StackLock lock(_mutex);
	if (_numServerSuspensions) {
		return;
	}

	for (SoundsList::iterator sound = _sounds.begin(); sound != _sounds.end(); ++sound) {
		if (sound->state != kStateActive) {
			continue;
		}

		if (sound->strategy == kStrategyAsync) {
			advancePlayback(*sound, false);
		}

		if (sound->signal == Kernel::kNoSignal) {
			continue;
		}

		const int16 signal = sound->signal;
		sound->signal = Kernel::kNoSignal;

		// TODO: Technically these writes to objects are a thread safety
		// violation, so put them in a queue which is processed by run_vm
		// instead
		if (signal == Kernel::kFinished) {
			if (sound->numLoops > 0) {
				--sound->numLoops;
			}
			writeSelectorValue(_segMan, sound->soundObj, SELECTOR(loop), sound->numLoops);
			if (sound->numLoops) {
				advancePlayback(*sound, false);
			} else {
				kernelStop(sound->soundObj);
			}
		} else {
			writeSelectorValue(_segMan, sound->soundObj, SELECTOR(signal), signal);
		}
	}
}

void Sci0SoundManager::advancePlayback(Sci0Sound &sound, const bool restoring) {
	if (_state.rest) {
		if (restoring) {
			_state.rest = 0;
		} else {
			--_state.rest;
			if (_state.currentFadeVolume) {
				processFade(sound);
			}
		}
		return;
	}

	for (;;) {
		if (_state.state == kStateReady) {
			const uint8 message = sound.consume();
			if (message == kEndOfTrack) {
				processEndOfTrack(sound);
				return;
			} else if (message == kFixedRest) {
				_state.rest = kFixedRestAmount;
				_state.state = kStateReady;
				if (restoring) {
					continue;
				} else {
					return;
				}
			} else {
				_state.state = kStateBlocked;
				_state.rest += message;
				if (_state.rest) {
					if (restoring) {
						_state.rest = 0;
					} else {
						--_state.rest;
						if (_state.currentFadeVolume) {
							processFade(sound);
						}
					}
					return;
				}
			}
		}

		// notReady
		_state.state = kStateBlocked;

		uint8 message = sound.peek();
		if (message & kStartOfMessageFlag) {
			sound.advance();
			_state.lastCommand = message;
		} else {
			message = _state.lastCommand;
		}

		if (message == kEndOfTrack) {
			processEndOfTrack(sound);
			return;
		}

		const uint8 channel = message & 0xf;
		const uint8 command = message & 0xf0;

		if (command == kProgramChange && channel == kControlChannel) {
			processControlChannel(sound);
		} else {
			sendMessage(sound, message, restoring);
		}

		_state.state = kStateReady;
	}
}

void Sci0SoundManager::processControlChannel(Sci0Sound &sound) {
	const uint8 programNo = sound.consume();
	if (programNo == kSetLoop) {
		_state.loopPosition = sound.position - 2;
	} else {
		_state.cue = programNo;
	}
}

void Sci0SoundManager::sendMessage(Sci0Sound &sound, const uint8 message, const bool restoring) {
	const uint8 channelNo = message & 0xf;
	const MidiMessageType command = MidiMessageType(message & 0xf0);
	const bool isMapped = _hardwareChannels[channelNo].isMapped();

	switch (command) {
	case kProgramChange: {
		const uint8 programNo = sound.consume();
		if (isMapped) {
			_driver->programChange(channelNo, programNo);
		}
		break;
	}
	case kNoteOn: {
		const uint8 note = sound.consume();
		const uint8 velocity = sound.consume();
		if (isMapped && !restoring) {
			if (velocity == 0) {
				_driver->noteOff(channelNo, note, velocity);
			} else {
				_driver->noteOn(channelNo, note, velocity);
			}
		}
		break;
	}
	case kNoteOff: {
		const uint8 note = sound.consume();
		const uint8 velocity = sound.consume();
		if (isMapped && !restoring) {
			_driver->noteOff(channelNo, note, velocity);
		}
		break;
	}
	case kControllerChange: {
		const uint8 controllerNo = sound.consume();
		const uint8 value = sound.consume();

		switch (controllerNo) {
		case kReverbModeController:
			_driver->setReverbMode(value);
			return;
		case kResetPositionOnPauseController:
			_state.resetPositionOnPause = (value != 0);
			return;
		case kCueController:
			_state.cue += value;
			sound.signal = _state.cue;
			return;
		}

		if (isMapped) {
			_driver->controllerChange(channelNo, controllerNo, value);
		}
		break;
	}
	case kPitchBend: {
		const uint8 lsb = sound.consume();
		const uint8 msb = sound.consume();
		if (isMapped) {
			_driver->pitchBend(channelNo, convert7To16(lsb, msb));
		}
		break;
	}
	case kChannelPressure: {
		const uint8 pressure = sound.consume();
		if (isMapped) {
			_driver->channelPressure(channelNo, pressure);
		}
		break;
	}
	case kKeyPressure: {
		const uint8 note = sound.consume();
		const uint8 pressure = sound.consume();
		if (isMapped) {
			_driver->keyPressure(channelNo, note, pressure);
		}
		break;
	}
	case kSysEx:
		while (sound.consume() != kEndOfSysEx);
		break;
	}
}

void Sci0SoundManager::processFade(Sci0Sound &sound) {
	--_state.fadeTicksLeftInStep;
	if (_state.fadeTicksLeftInStep > 0) {
		return;
	}

	--_state.currentFadeVolume;
	if (_state.currentFadeVolume) {
		_driver->setMasterVolume(_state.currentFadeVolume);
		_state.fadeTicksLeftInStep = 8 + _state.fadeTicksPerStep;
		_state.fadeTicksPerStep += 2;
	} else {
		sound.numLoops = 0;
		stop(sound);
	}
}

#pragma mark -
#pragma mark Effects

void Sci0SoundManager::setMasterVolumeImpl(const uint8 volume) {
	_masterVolume = volume;
	setSoundVolumes(volume);
}

void Sci0SoundManager::setSoundVolumes(const uint8 volume) {
	for (SoundsList::iterator sound = _sounds.begin(); sound != _sounds.end(); ++sound) {
		sound->volume = volume;
		if (sound->state == kStateActive) {
			_driver->setMasterVolume(volume);
		}
	}
}

Sci0PlayStrategy Sci0SoundManager::initSound(Sci0Sound &sound) {
	_hardwareChannels = {};
	_state = {};

	enum {
		kMidi = 0,
		kSignedSample = 1,
		kUnsignedSample = 2
	};

	assert(sound.resource);
	SciSpan<const byte> data = *sound.resource;

	if (data[0] == kSignedSample || data[0] == kUnsignedSample) {
		uint16 sampleOffset = data.getUint16BEAt(0x1f);
		if (!sampleOffset) {
			sampleOffset = kHeaderSize;
		}

		// Technically this is not fully accurate; in SSCI, if there is an
		// offset, then it uses that offset directly without scanning past
		// markers
		--sampleOffset;

		_samplePlayer.load(data.subspan(sampleOffset), sound.volume, sound.numLoops);
		return kStrategyAsync;
	}

	if (data[0] != kMidi) {
		return kStrategyAbort;
	}

	sound.position = kHeaderSize;
	sound.signal = Kernel::kNoSignal;

	uint8 instrumentMask, percussionMask;
	_driver->getChannelMasks(instrumentMask, percussionMask);

	SciSpan<const byte> channelTable = data.subspan(1, _hardwareChannels.size() * (_soundVersion == SCI_VERSION_0_EARLY ? 1 : 2));
	for (uint i = 0; i < _hardwareChannels.size(); ++i) {
		if (_soundVersion == SCI_VERSION_0_EARLY) {
			if (!instrumentMask || (*channelTable & instrumentMask)) {
				_hardwareChannels[i].channelNo = i;
				_hardwareChannels[i].numVoices = *channelTable >> 4;
			}
			++channelTable;
		} else {
			bool isValid;
			if (i == kPercussionChannel) {
				isValid = (!percussionMask || (channelTable[0] & percussionMask));
			} else {
				isValid = (!instrumentMask || (channelTable[1] & instrumentMask));
			}

			if (isValid) {
				_hardwareChannels[i].channelNo = i;
				_hardwareChannels[i].numVoices = channelTable[0] & 0x7f;
			}

			channelTable += 2;
		}
	}

	if (((data[kHeaderSize] & 0xf0) != kControllerChange ||
		data[kHeaderSize + 1] != kReverbModeController)) {
		_driver->setReverbMode(kUseDefaultReverb);
	}

	_driver->setMasterVolume(sound.volume);

	return kStrategyAsync;
}

void Sci0SoundManager::processEndOfTrack(Sci0Sound &sound) {
	sound.signal = Kernel::kFinished;
	sound.position = _state.loopPosition;
	_state.rest = 0;
	_state.state = kStateBlocked;
}

void Sci0SoundManager::play(Sci0Sound &sound) {
	Sci0Sound &activeSound = activate(sound);
	activeSound.strategy = initSound(activeSound);
	switch (activeSound.strategy) {
	case kStrategySync:
		// TODO: Only implement sync strategy if it is actually needed
		error("Sync strategy not implemented");
	case kStrategyAsync:
		advancePlayback(activeSound, false);
		break;
	case kStrategyAbort:
		_resMan.unlockResource(activeSound.resource);
		activeSound.resource = nullptr;
		break;
	default:
		break;
	}
}

Sci0Sound &Sci0SoundManager::activate(Sci0Sound &sound) {
	assert(!sound.resource);
	sound.resource = _resMan.findResource(ResourceId(kResourceTypeSound, sound.resourceNo), true);
	assert(sound.resource);
	return finishActivation(sound);
}

void Sci0SoundManager::pause(Sci0Sound &sound) {
	if (sound.state == kStateActive) {
		stopAllChannels(true);
		if (_state.resetPositionOnPause) {
			sound.position = _state.loopPosition;
			_state.rest = 0;
			_state.state = kStateBlocked;
		}
		_resMan.unlockResource(sound.resource);
		sound.resource = nullptr;
	}

	sound.state = kStateBlocked;
	writeSelectorValue(_segMan, sound.soundObj, SELECTOR(state), kStateBlocked);
}

void Sci0SoundManager::stop(Sci0Sound &sound) {
	stopAllChannels(false);
	processEndOfTrack(sound);
}

void Sci0SoundManager::stopAllChannels(const bool pauseOnly) {
	for (uint i = 0; i < _hardwareChannels.size(); ++i) {
		if (!_hardwareChannels[i].isMapped()) {
			continue;
		}

		_driver->controllerChange(i, kAllNotesOffController, 0);
		_driver->controllerChange(i, kDamperPedalController, 0);
		_driver->controllerChange(i, kModulationController, 0);

		if (!pauseOnly) {
			_driver->pitchBend(i, 0x2000);
		}
	}
}

void Sci0SoundManager::resume(Sci0Sound &sound) {
	assert(!sound.resource);
	sound.resource = _resMan.findResource(ResourceId(kResourceTypeSound, sound.resourceNo), true);

	uint16 position = sound.position;
	if (initSound(sound) != kStrategyAsync) {
		return;
	}
	do {
		advancePlayback(sound, true);
		if (_state.resetPositionOnPause && _state.loopPosition != kHeaderSize) {
			_state.rest = 0;
			_state.state = kStateBlocked;
			position = _state.loopPosition;
		}
		if (sound.signal == Kernel::kNoSignal) {
			position = sound.position;
		}
	} while (sound.position < position);

	finishActivation(sound);
	_numServerSuspensions = 0;
}

void Sci0SoundManager::fade(Sci0Sound &sound) {
	_state.currentFadeVolume = sound.volume;
	if (sound.volume == 0) {
		sound.numLoops = 0;
		stop(sound);
	}
}

Sci0Sound &Sci0SoundManager::finishActivation(Sci0Sound &sound) {
	sound.state = kStateActive;
	writeSelectorValue(_segMan, sound.soundObj, SELECTOR(state), kStateActive);

	SoundsList::iterator it = findSoundIterator<ByRegT>(sound.soundObj);
	assert(it != _sounds.end());
	if (it != _sounds.begin()) {
		const Sci0Sound copy(*it);
		_sounds.erase(it);
		_sounds.insert_at(0, copy);
	}
	return _sounds.front();
}

#pragma mark -
#pragma mark Playback management

void Sci0SoundManager::pauseAll(const bool pause) {
	Common::StackLock lock(_mutex);
	if (pause) {
		_lastNumServerSuspensions = _numServerSuspensions;
		kernelPause(true);
	} else if (!_lastNumServerSuspensions) {
		kernelPause(false);
	}
}

#pragma mark -
#pragma mark Kernel

void Sci0SoundManager::kernelInit(const reg_t soundObj) {
	const GuiResourceId resourceNo = readSelectorValue(_segMan, soundObj, SELECTOR(number));
	if (!_resMan.testResource(ResourceId(kResourceTypeSound, resourceNo))) {
		return;
	}

	Common::StackLock lock(_mutex);

	Sci0Sound *sound = findSound<ByRegT>(soundObj);
	const int16 priority = readSelectorValue(_segMan, soundObj, SELECTOR(priority));
	if (!sound) {
		SoundsList::iterator it = findSoundIterator<ByLowerPriority>(priority);
		sound = &*_sounds.insert(it, Sci0Sound(soundObj));
		_numServerSuspensions = 0;
	} else if (sound->resource) {
		_resMan.unlockResource(sound->resource);
		sound->resource = nullptr;
	}

	sound->resourceNo = resourceNo;
	sound->numLoops = readSelectorValue(_segMan, soundObj, SELECTOR(loop));
	sound->priority = readSelectorValue(_segMan, soundObj, SELECTOR(priority));
	sound->volume = isSoundEnabled() ? getMasterVolume() : 0;
	sound->strategy = kStrategyNone;
	sound->state = kStateReady;
	writeSelector(_segMan, soundObj, SELECTOR(handle), soundObj);
	writeSelectorValue(_segMan, soundObj, SELECTOR(state), kStateReady);
}

void Sci0SoundManager::kernelPlay(const reg_t soundObj, const bool) {
	Common::StackLock lock(_mutex);

	Sci0Sound *sound = findSound<ByRegT>(soundObj);
	if (!sound) {
		return;
	}

	Sci0Sound *activeSound = findSoundByState<kStateActive>();

	if (activeSound) {
		if (activeSound->priority < sound->priority) {
			sound->state = kStateBlocked;
			writeSelectorValue(_segMan, sound->soundObj, SELECTOR(state), kStateBlocked);
			return;
		} else {
			pause(*activeSound);
		}
	}

	play(*sound);
}

void Sci0SoundManager::kernelDispose(const reg_t soundObj) {
	if (soundObj.isNull()) {
		return;
	}

	Common::StackLock lock(_mutex);
	kernelStop(soundObj);
	_sounds.erase(findSoundIterator<ByRegT>(soundObj));
	_numServerSuspensions = 0;
}

void Sci0SoundManager::kernelStop(const reg_t soundObj) {
	Common::StackLock lock(_mutex);
	// SSCI checked for a null soundObj but this would result in reading garbage
	// memory from the stack later when checking the sound state
	Sci0Sound *sound = findSound<ByRegT>(soundObj);
	assert(sound);
	const bool isActiveSound = (sound->state == kStateActive);
	sound->state = kStateNotReady;
	writeSelectorValue(_segMan, soundObj, SELECTOR(state), kStateNotReady);
	sound->signal = Kernel::kFinished;
	writeSelectorValue(_segMan, soundObj, SELECTOR(signal), Kernel::kFinished);
	if (isActiveSound) {
		stop(*sound);
		_resMan.unlockResource(sound->resource);
		sound->resource = nullptr;
		Sci0Sound *nextSound = findSoundByState<kStateBlocked>();
		if (nextSound) {
			// In SSCI0late/SQ3, if an attempt to load the sound resource
			// failed, the engine would skip playing the sound and set a flag,
			// and the game scripts would follow up later on trying to play the
			// sound with an unconditional call to kDoSoundRetry.
			// Since we have no recoverable resource load failures, this
			// functionality is superfluous so is omitted.
			if (nextSound->strategy == kStrategyNone) {
				play(*nextSound);
			} else {
				resume(*nextSound);
			}
		}
	}
	_numServerSuspensions = 0;
}

bool Sci0SoundManager::kernelPause(const bool shouldPause) {
	Common::StackLock lock(_mutex);
	// TODO: This is weird and kind of broken since any time a sound is paused
	// in this way it will be unpaused the next time any one of a myriad of
	// other calls occurs
	const int prevNumSuspensions = _numServerSuspensions;

	Sci0Sound *sound = findSoundByState<kStateActive>();
	if (shouldPause && sound) {
		pause(*sound);
	} else if (!shouldPause && !sound) {
		Sci0Sound *nextSound = findSoundByState<kStateBlocked>();
		if (nextSound) {
			activate(*nextSound);
		}
	}

	_numServerSuspensions = shouldPause ? 1 : 0;
	return (prevNumSuspensions == 0);
}

void Sci0SoundManager::kernelUpdate(const reg_t soundObj) {
	Common::StackLock lock(_mutex);
	Sci0Sound *sound = findSound<ByRegT>(soundObj);
	if (sound) {
		sound->numLoops = readSelectorValue(_segMan, soundObj, SELECTOR(loop));
		sound->priority = readSelectorValue(_segMan, soundObj, SELECTOR(priority));
	}
	_numServerSuspensions = 0;
}

void Sci0SoundManager::kernelFade(const reg_t soundObj) {
	Common::StackLock lock(_mutex);
	Sci0Sound *sound = findSound<ByRegT>(soundObj);
	assert(sound);
	if (sound->state == kStateActive) {
		fade(*sound);
	}
}

#pragma mark -
#pragma mark Debugging

void Sci0SoundManager::debugPrintPlaylist(Console &con) const {
	int i = 0;
	for (SoundsList::const_iterator sound = _sounds.begin(); sound != _sounds.end(); ++sound) {
		con.debugPrintf("%2d: ", i);
		debugPrintSound(con, *sound);
	}
}

void Sci0SoundManager::debugPrintSound(Console &con, const uint index) const {
	if (index >= _sounds.size()) {
		con.debugPrintf("Index out of range\n");
		return;
	}

	debugPrintSound(con, _sounds[index]);
}

void Sci0SoundManager::debugPrintChannelMap(Console &con) const {
	for (uint i = 0; i < _hardwareChannels.size(); ++i) {
		const HardwareChannel &hwChannel = _hardwareChannels[i];
		con.debugPrintf("%2d: ", i);
		if (hwChannel.isMapped()) {
			con.debugPrintf("ch %2d vo %2d\n", hwChannel.channelNo, hwChannel.numVoices);
		} else {
			con.debugPrintf("unmapped\n");
		}
	}
}

void Sci0SoundManager::debugPlaySound(Console &con, const GuiResourceId resourceNo, const bool exclusive) {
	con.debugPrintf("TODO: SCI0 debug play sound\n");
}

void Sci0SoundManager::debugStopAll() {
	reset();
}

void Sci0SoundManager::debugPrintSound(Console &con, const Sci0Sound &sound) const {
	const char *state;
	switch (sound.state) {
	case kStateNotReady:
		state = "not ready";
		break;
	case kStateBlocked:
		state = "blocked";
		break;
	case kStateActive:
		state = "active";
		break;
	case kStateReady:
		state = "ready";
		break;
	default:
		state = "invalid";
	}

	con.debugPrintf("%04x:%04x, sound %d, %s\n",
					PRINT_REG(sound.soundObj), sound.resourceNo, state);
	con.debugPrintf("    priority %d, loops %d, position %d\n",
					sound.priority, sound.numLoops, sound.position);
	con.debugPrintf("    volume %d, signal %d, effect %u\n",
					sound.volume, sound.signal, sound.effect);
}

} // End of namespace Sci
