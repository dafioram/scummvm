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
	 * The caller should synchronously invoke `service` on the driver until
	 * `signal` is kFinished.
	 */
	kSync = 0,

	/**
	 * The caller should invoke `service` once on the driver and then
	 * return.
	 */
	kAsync = 1,

	/**
	 * Playback did not begin; the caller should clean up immediately.
	 */
	kAbort = 3
};

/**
 * A representation of the current state of a sound in SCI0.
 */
struct Sci0Sound {
	// TODO: In SSCI, the first field is a Node, is this relevant?

	/**
	 * The sound resource data.
	 *
	 * @note In SSCI this is a raw pointer to the resource data.
	 */
	Resource *data;

	int16 numLoops;

	/**
	 * The current position of playback in the sound resource, in bytes.
	 */
	int16 position;

	/**
	 * The priority of the sound. See `SciSound::priority`.
	 */
	int16 priority;

	/**
	 * The playback strategy for the sound.
	 */
	Sci0PlayStrategy strategy;

	// uint16 chain; // TODO: May be unused, replaced by priority

	int16 state;

	uint16 signal;

	/**
	 * The playback volume of the sound, TODO: from 0 to 15?
	 */
	int16 volume;

	// uint16 effect; // TODO: Not a clue what this is
};

class Sci0SoundManager final : public SoundManager {

};

} // End of namespace Sci

#endif
