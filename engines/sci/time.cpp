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

#include "common/system.h"
#include "engines/engine.h"
#include "sci/sci.h"
#include "sci/time.h"
#include "sci/event.h"
#ifdef ENABLE_SCI32
#include "sci/graphics/frameout.h"
#endif

namespace Sci {

TimeManager::TimeManager(OSystem &system, Engine &engine, EventManager &eventMan) :
	_system(system),
	_engine(engine),
	_eventMan(eventMan),
	_throttleNextCall(false),
	_throttleLastTime(0)
#ifdef ENABLE_SCI32
	, _gfxFrameout(nullptr)
#endif
	{}

void TimeManager::throttle(const uint32 ms, const bool enableNext) {
	if (_throttleNextCall) {
		const uint32 now = _system.getMillis();
		const uint32 delta = now - _throttleLastTime;

		if (delta < ms) {
			sleep(ms - delta);
			_throttleLastTime = _system.getMillis();
		} else {
			_throttleLastTime = now;
		}
	}

	_throttleNextCall = enableNext;
}

void TimeManager::sleep(const uint32 ms) const {
	if (!ms) {
		return;
	}

	const uint32 endTime = _system.getMillis() + ms;

	for (;;) {
		// let backend process events and update the screen
		_eventMan.getSciEvent(kSciEventPeek);

		// There is no point in waiting any more if we are just waiting to quit
		if (_engine.shouldQuit()) {
			return;
		}

#ifdef ENABLE_SCI32
		// If a game is in a wait loop, kFrameOut is not called, but mouse
		// movement is still occurring and the screen needs to be updated to
		// reflect it
		if (_gfxFrameout) {
			_gfxFrameout->updateScreen();
		}
#endif
		const uint32 now = _system.getMillis();
		if (now + 10 < endTime) {
			_system.delayMillis(10);
		} else {
			if (now < endTime) {
				_system.delayMillis(endTime - now);
			}
			break;
		}
	}
}

} // End of namespace Sci
