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
	SoundManager(resMan, segMan, features, guestAdditions) {

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
#pragma mark Kernel

void Sci0SoundManager::kernelInit(const reg_t soundObj) {
	const ResourceId id(kResourceTypeSound, readSelectorValue(_segMan, soundObj, SELECTOR(number)));
	if (!_resMan.testResource(id)) {
		return;
	}

	Sci0Sound *sound = findSoundByRegT(soundObj);
	if (!sound) {
		_sounds.push_back(Sci0Sound(soundObj));
		sound = &_sounds.back();
	}

	sound->resource = _resMan.findResource(id, true);
	sound->numLoops = readSelectorValue(_segMan, soundObj, SELECTOR(loop));
	sound->priority = readSelectorValue(_segMan, soundObj, SELECTOR(priority));
	sound->volume = _driver->getMasterVolume();
	sound->strategy = kStrategyNone;
	sound->state = kStateReady;
	writeSelectorValue(_segMan, soundObj, SELECTOR(state), kStateReady);
}

} // End of namespace Sci
