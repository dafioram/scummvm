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

	// This value normally comes from the static data in the data segment
	// of the interpreter executable
	_driver->setMasterVolume(12);

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
		restore(*sound);
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

	error("TODO");
}

#pragma mark -
#pragma mark Effects

void Sci0SoundManager::setMasterVolume(const uint8 volume) {
	Common::StackLock lock(_mutex);
	SoundManager::setMasterVolume(volume);
	setSoundVolumes(volume);
}

void Sci0SoundManager::setSoundVolumes(const uint8 volume) {
	for (SoundsList::iterator sound = _sounds.begin(); sound != _sounds.end(); ++sound) {
		sound->volume = volume;
		// TODO: SSCI called sound driver to set volume on the active sound,
		// passing the sound object, but this seems unnecessary since master
		// volume is already held by the driver instead of the interpreter in
		// this implementation
	}
}

void Sci0SoundManager::play(Sci0Sound &sound) {
	activate(sound);

	// TODO: driver code goes here
	debug("TODO: play");
	sound.strategy = kStrategyAsync;

	switch (sound.strategy) {
	case kStrategySync:
		// TODO: Only implement sync strategy if it is actually needed
		error("Sync strategy not implemented");
	case kStrategyAsync:
		soundServer();
		break;
	case kStrategyAbort:
		_resMan.unlockResource(sound.resource);
		sound.resource = nullptr;
		break;
	default:
		break;
	}
}

void Sci0SoundManager::activate(Sci0Sound &sound) {
	assert(!sound.resource);
	sound.resource = _resMan.findResource(ResourceId(kResourceTypeSound, sound.resourceNo), true);
	finishActivation(sound);
}

void Sci0SoundManager::pause(Sci0Sound &sound) {
	if (sound.state == kStateActive) {
		// TODO: driver pause code goes here
		debug("TODO: pause");
		_resMan.unlockResource(sound.resource);
		sound.resource = nullptr;
	}

	sound.state = kStateBlocked;
	writeSelectorValue(_segMan, sound.soundObj, SELECTOR(state), kStateBlocked);
}

void Sci0SoundManager::stop(Sci0Sound &sound) {
	// TODO: driver stop code goes here
	debug("TODO: stop");
}

void Sci0SoundManager::restore(Sci0Sound &sound) {
	assert(!sound.resource);
	sound.resource = _resMan.findResource(ResourceId(kResourceTypeSound, sound.resourceNo), true);
	// TODO: driver restore code goes here
	debug("TODO: restore");
	finishActivation(sound);
	_numServerSuspensions = 0;
}

void Sci0SoundManager::fade(Sci0Sound &sound) {
	// TODO: driver fade code goes here
	debug("TODO: fade");
}

void Sci0SoundManager::finishActivation(Sci0Sound &sound) {
	sound.state = kStateActive;
	writeSelectorValue(_segMan, sound.soundObj, SELECTOR(state), kStateActive);

	SoundsList::iterator it = findSoundIterator<ByRegT>(sound.soundObj);
	assert(it != _sounds.end());
	if (it != _sounds.begin()) {
		const Sci0Sound copy(*it);
		_sounds.erase(it);
		_sounds.insert_at(0, copy);
	}
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
	const ResourceId id(kResourceTypeSound, readSelectorValue(_segMan, soundObj, SELECTOR(number)));
	if (!_resMan.testResource(id)) {
		return;
	}

	Common::StackLock lock(_mutex);

	Sci0Sound *sound = findSound<ByRegT>(soundObj);
	const int16 priority = readSelectorValue(_segMan, soundObj, SELECTOR(priority));
	if (!sound) {
		SoundsList::iterator it = findSound<ByLowerPriority>(priority);
		_sounds.insert(it, Sci0Sound(soundObj));
		sound = &*it;
		_numServerSuspensions = 0;
	}

	sound->resourceNo = id.getNumber();
	sound->numLoops = readSelectorValue(_segMan, soundObj, SELECTOR(loop));
	sound->priority = readSelectorValue(_segMan, soundObj, SELECTOR(priority));
	sound->volume = isSoundEnabled() ? getMasterVolume() : 0;
	sound->strategy = kStrategyNone;
	sound->state = kStateReady;
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
		Sci0Sound *nextSound = findSoundByState<kStateBlocked>();
		if (nextSound) {
			if (nextSound->strategy == kStrategyNone) {
				play(*nextSound);
			} else {
				restore(*nextSound);
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
	con.debugPrintf("TODO: SCI0 channel output information\n");
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
