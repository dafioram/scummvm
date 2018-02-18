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

#include "sci/s2/system/glcel.h"
#include "sci/s2/system/glcue.h"
#include "sci/s2/system/glcycler.h"
#include "sci/time.h"

namespace Sci {

TimeManager *GLCycler::_timeManager = nullptr;
GLExtras *GLCycler::_extras = nullptr;

int GLCycler::add(GLCel &cel, const bool shouldStart) {
	_cels.push_back(&cel);
	_timings.push_back(_timeManager->getTickCount() + cel.getCycleSpeed());
	cel.setCycler(this);
	if (_cels.size() == 1 && shouldStart) {
		start();
	}
	return _cels.size();
}

void GLCycler::start() {
	_isCycling = true;
	_isComplete = false;
	_extras->push_back(this);
}

void GLCycler::start(GLObject *caller) {
	_caller = caller;
	start();
}

void GLCycler::stop() {
	_isCycling = false;
	_extras->remove(this);
}

void GLCycler::cycleForward(const bool forward) {
	_direction = forward ? 1 : -1;
}

void GLCycler::doIt() {
	_numCyclesCompleted = 0;
	if (_cels.size()) {
		const uint32 now = _timeManager->getTickCount();
		for (uint i = 0; i < _cels.size(); ++i) {
			if (now >= _timings[i]) {
				_timings[i] = now + _cels[i]->getCycleSpeed();
				_cels[i]->setCel(nextCel(*_cels[i]), true);
			}
		}
	}
	if (_numCyclesCompleted == _cels.size()) {
		_isComplete = true;
		done();
	}
}

int16 GLCycler::nextCel(GLCel &client) {
	int16 lastCel = client.getLastCel();
	int16 newCel = client.getCel() + _direction;
	assert(lastCel >= 0);
	if (newCel < 0) {
		newCel = lastCel;
	} else if (newCel > lastCel) {
		newCel = 0;
	}
	return newCel;
}

void GLCycler::done() {
	stop();
	release();
	if (_caller) {
		new GLCue(_caller);
		_caller = nullptr;
	}
}

void GLCycler::release() {
	for (auto &cel : _cels) {
		cel->setCycler(nullptr);
	}
	_cels.clear();
	_timings.clear();
}

int16 GLPingPongCycler::nextCel(GLCel &client) {
	int16 lastCel = client.getLastCel();
	int16 cel = client.getCel();
	if (lastCel != 0) {
		if (cel == 0) {
			cycleForward(true);
		} else if (cel == lastCel) {
			cycleForward(false);
		}

		return cel + getDirection();
	}

	// In SSCI this would return garbage memory if lastCel was 0
	return cel;
}

} // End of namespace Sci
