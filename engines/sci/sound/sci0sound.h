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

#ifndef SCI_SOUND_SCI0SOUND_H
#define SCI_SOUND_SCI0SOUND_H

#include "sci/sound/sound.h"

namespace Sci {

enum Sci0PlayStrategy {
	/**
	 * No play strategy is defined.
	 */
	kStrategyNone = -1,

	/**
	 * The caller should synchronously invoke `service` on the driver until
	 * `signal` is kFinished.
	 */
	kStrategySync = 0,

	/**
	 * The caller should invoke `service` once on the driver and then
	 * return.
	 */
	kStrategyAsync = 1,

	/**
	 * Playback did not begin; the caller should clean up immediately.
	 */
	kStrategyAbort = 3
};

enum Sci0PlayState {
	kStateNotReady = 0,
	kStateReady = 1,
	kStateBlocked = 2,
	kStateActive = 3
};

/**
 * A representation of the current state of a sound in SCI0.
 */
struct Sci0Sound {
	/**
	 * The VM sound object used to create this sound.
	 */
	reg_t soundObj;

	/**
	 * The Sound resource.
	 *
	 * @note In SSCI this is a raw pointer to the resource data.
	 */
	Resource *resource;

	/**
	 * The number of times the sound should loop.
	 */
	uint16 numLoops;

	/**
	 * The current position of playback in the sound resource, in bytes.
	 */
	uint16 position;

	/**
	 * The priority of the sound. See `Sci1Sound::priority`.
	 */
	int16 priority;

	/**
	 * The playback strategy for the sound.
	 */
	Sci0PlayStrategy strategy;

	uint16 chain; // TODO: May be unused, replaced by priority

	int16 state;

	uint16 signal;

	/**
	 * The playback volume of the sound, TODO: from 0 to 15?
	 */
	int16 volume;

	uint16 effect; // TODO: Not a clue what this is

	Sci0Sound(const reg_t soundObj_) :
		soundObj(soundObj_),
		resource(nullptr),
		numLoops(0),
		position(0),
		priority(0),
		strategy(kStrategySync),
		chain(0),
		state(0),
		signal(0),
		volume(0),
		effect(0) {}
};

class Sci0SoundManager final : public SoundManager {
public:
	Sci0SoundManager(ResourceManager &resMan, SegManager &segMan, GameFeatures &features, GuestAdditions &guestAdditions);
	~Sci0SoundManager();

private:
	typedef Common::Array<Sci0Sound> SoundsList;

	/**
	 * The list of currently playing & queued sounds.
	 */
	SoundsList _sounds;

#pragma mark -
#pragma mark Save management
public:

#pragma mark -
#pragma mark MIDI server
private:
	static inline void soundServerCallback(void *soundManager) {
		static_cast<Sci0SoundManager *>(soundManager)->soundServer();
	}

	/**
	 * Advances the state of the sound server. Must be called once per tick in
	 * SCI0 for correct sound processing.
	 */
	void soundServer();

#pragma mark -
#pragma mark Kernel
public:
	void kernelInit(const reg_t soundObj) override;

private:
	class SoundByRegT {
		reg_t _key;
	public:
		inline SoundByRegT(const reg_t key) : _key(key) {}
		inline bool operator()(const Sci0Sound &sound) const {
			return sound.soundObj == _key;
		}
	};

	inline const Sci0Sound *findSoundByRegT(const reg_t key) const {
		SoundsList::const_iterator it = Common::find_if(_sounds.begin(), _sounds.end(), SoundByRegT(key));
		if (it == _sounds.end()) {
			return nullptr;
		}
		return &*it;
	}

	inline Sci0Sound *findSoundByRegT(const reg_t key) {
		SoundsList::iterator it = Common::find_if(_sounds.begin(), _sounds.end(), SoundByRegT(key));
		if (it == _sounds.end()) {
			return nullptr;
		}
		return &*it;
	}
};

} // End of namespace Sci

#endif
