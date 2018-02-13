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

#ifndef SCI_THROTTLER_H
#define SCI_THROTTLER_H

#include "common/scummsys.h"

class Engine;
class OSystem;

namespace Sci {
class EventManager;
class GfxFrameout;

class TimeManager {
public:
	TimeManager(OSystem &system, Engine &engine, EventManager &eventMan);

	inline void reset() {
		_throttleLastTime = 0;
	}

	inline uint32 getTickCount() const {
		return _engine.getTotalPlayTime() * 60 / 1000;
	}

	inline void setTickCount(const uint32 ticks) {
		return _engine.setTotalPlayTime(ticks * 1000 / 60);
	}

	/**
	 * Enables throttling on the next call to `throttle`.
	 */
	inline void enableNextThrottle() { _throttleNextCall = true; }

	/**
	 * Conditionally delays execution so that at least `ms` milliseconds are
	 * guaranteed to elapse between calls to `throttle` as long as
	 * `enableNextThrottle` has been called since the last call to `throttle`.
	 */
	void throttle(const uint32 ms, const bool enableNext = false);

	/**
	 * Delays execution for `ms` milliseconds, polling the event system
	 * periodically to prevent the game from appearing non-responsive to the OS.
	 */
	void sleep(const uint32 ms) const;

	/**
	 * Delays execution for `ticks` ticks, polling the event system
	 * periodically to prevent the game from appearing non-responsive to the OS.
	 */
	inline void sleepTicks(const uint32 ticks) const {
		sleep(ticks * 1000 / 60);
	}

#ifdef ENABLE_SCI32
	void attachRenderer(GfxFrameout *frameout) { _gfxFrameout = frameout; }
#endif

private:
	bool _throttleNextCall;
	uint32 _throttleLastTime;

	OSystem &_system;
	Engine &_engine;
	EventManager &_eventMan;
#ifdef ENABLE_SCI32
	GfxFrameout *_gfxFrameout;
#endif
};
}

#endif
