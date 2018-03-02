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
#include "sci/s2/system/glmover.h"
#include "sci/time.h"

namespace Sci {

GLBresen::GLBresen(GLPoint &start, const GLPoint &end, const GLPoint &step) :
	_current(&start),
	_end(end),
	_step(1, 1),
	_extra(1, 1) {

	auto delta = end - start;
	if (delta.x < 0) {
		delta.x = -delta.x;
		_step.x = _extra.x = -1;
	}
	if (delta.y < 0) {
		delta.y = -delta.y;
		_step.y = _extra.y = -1;
	}

	int numStepsX = delta.x / step.x;
	int numStepsY = delta.y / step.y;
	if (delta.x % step.x) ++numStepsX;
	if (delta.y % step.y) ++numStepsY;

	if (numStepsX >= numStepsY) {
		_step.x *= step.x;
		_step.y *= delta.y / numStepsX;
		_extra.x = 0;
		_remainder = delta.y % numStepsX;
		_correction = _stepsLeft = numStepsX;
	} else {
		_step.y *= step.y;
		_step.x *= delta.x / numStepsY;
		_extra.y = 0;
		_remainder = delta.x % numStepsY;
		_correction = _stepsLeft = numStepsY;
	}

	// TODO: SSCI did not adjust the correction by 1 here so would correct error
	// on the wrong steps; is -1 an appropriate fix for all cases?
	_error = -(_correction - 1) / 2;
}

bool GLBresen::doMove() {
	if (--_stepsLeft) {
		*_current += _step;
		_error += _remainder;
		if (_error >= 0) {
			_error -= _correction;
			*_current += _extra;
		}
		return *_current == _end;
	} else {
		*_current = _end;
		return true;
	}
}

template <class CelT>
TimeManager *AbsGLMover<CelT>::_timeManager = nullptr;
template <class CelT>
GLExtras *AbsGLMover<CelT>::_extras = nullptr;

template <class CelT>
AbsGLMover<CelT>::AbsGLMover(CelT &client, const GLPoint &target) :
	_client(&client),
	_position(client.getPosition()) {
	start(target);
}

template <class CelT>
AbsGLMover<CelT>::AbsGLMover(CelT &client, const GLPoint &target, GLObject &caller) :
	_client(&client),
	_position(client.getPosition()),
	_caller(&caller) {
	start(target);
}

template <class CelT>
AbsGLMover<CelT>::~AbsGLMover() {
	// SSCI did different stuff in its destructor, not including properly
	// cleaning up the mover like this
	stop();
}

template <class CelT>
void AbsGLMover<CelT>::start(const GLPoint &target) {
	_end = target;
	_bresen = GLBresen(_position, target, _client->getStepSize());
	_nextTick = _timeManager->getTickCount() + _client->getMoveSpeed();
	_extras->push_front(this);
}

template <class CelT>
void AbsGLMover<CelT>::doIt() {
	if (_timeManager->getTickCount() >= _nextTick && doStep()) {
		nextDest();
	}
}

template <class CelT>
bool AbsGLMover<CelT>::doStep() {
	const auto done = _bresen.doMove();
	// SSCI did not try to correct for lag in this calculation
	const auto now = _timeManager->getTickCount();
	const auto ticksToNextMove = MAX(0, _client->getMoveSpeed() - int(now - _nextTick));
	_nextTick = now + ticksToNextMove;
	_client->setPosition(_position, true);
	return done;
}

template <class CelT>
void AbsGLMover<CelT>::nextDest() {
	done();
}

template <class CelT>
void AbsGLMover<CelT>::done() {
	stop();
	if (_caller) {
		new GLCue(_caller, this);
		_caller = nullptr;
	}
}

template <class CelT>
void AbsGLMover<CelT>::stop() {
	_extras->remove(this);
}

template class AbsGLMover<GLCel>;

} // End of namespace Sci
