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

#include "sci/s2/system/gltimer.h"
#include "sci/time.h"

namespace Sci {

TimeManager *GLTimer::_timeManager = nullptr;

GLTimer::GLTimer(GLObject *const cuee, GLObject *const cuer, const int data, void *const dataPointer) :
	GLCue(cuee, cuer, data, dataPointer),
	_type(Type::None) {}

bool GLTimer::check() {
	bool shouldCue;
	switch (_type) {
	case Type::Cycles:
		shouldCue = (--_cyclesUntilCue == 0);
		break;
	case Type::Ticks:
		shouldCue = (_timeManager->getTickCount() >= _tickToCueAt);
		break;
	case Type::Seconds:
		shouldCue = (_timeManager->getSystemDate() >= _secondToCueAt);
		break;
	default:
		error("Invalid timer type %d", _type);
	}

	if (shouldCue) {
		_type = Type::None;
	}

	return shouldCue;
}

void GLTimer::doIt() {
	if (_type != Type::None) {
		GLCue::doIt();
	}
}

void GLTimer::setCycles(const uint32 numCycles) {
	assert(_type == Type::None);
	_type = Type::Cycles;

	// Since this is CPU speed dependent, it should be used only as an mechanism
	// for firing the immediately on the next event loop
	// TODO: Disable frame throttling for the next frame so there is not a FOUC?
	assert(numCycles == 1);

	_cyclesUntilCue = numCycles;
}

void GLTimer::setTicks(const uint32 numTicks) {
	assert(_type == Type::None);
	_type = Type::Ticks;
	_tickToCueAt = _timeManager->getTickCount() + numTicks;
}

void GLTimer::setSeconds(const uint32 numSeconds) {
	assert(_type == Type::None);
	_type = Type::Seconds;
	_secondToCueAt = _timeManager->getSystemDate() + numSeconds;
}

} // End of namespace Sci
