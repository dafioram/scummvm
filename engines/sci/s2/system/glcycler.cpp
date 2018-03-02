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

#include "sci/s2/panorama_sprite.h"
#include "sci/s2/system/glcel.h"
#include "sci/s2/system/glcue.h"
#include "sci/s2/system/glcycler.h"
#include "sci/time.h"

namespace Sci {

template <class CelT>
TimeManager *AbsGLCycler<CelT>::_timeManager = nullptr;
template <class CelT>
GLExtras *AbsGLCycler<CelT>::_extras = nullptr;

template <class CelT>
AbsGLCycler<CelT>::~AbsGLCycler() {
	if (_isCycling) {
		stop();
	}

	release();
}

template <class CelT>
int AbsGLCycler<CelT>::add(CelT &cel, const bool shouldStart) {
	_cels.push_back(&cel);
	_timings.push_back(_timeManager->getTickCount() + cel.getCycleSpeed());
	cel.setCycler(this);
	if (_cels.size() == 1 && shouldStart) {
		start();
	}
	return _cels.size();
}

template <class CelT>
void AbsGLCycler<CelT>::start() {
	_isCycling = true;
	_isFinished = false;
	_extras->push_front(this);
}

template <class CelT>
void AbsGLCycler<CelT>::start(GLObject &caller) {
	_caller = &caller;
	start();
}

template <class CelT>
void AbsGLCycler<CelT>::stop() {
	_isCycling = false;
	_extras->remove(this);
}

template <class CelT>
void AbsGLCycler<CelT>::cycleForward(const bool forward) {
	_direction = forward ? 1 : -1;
}

template <class CelT>
void AbsGLCycler<CelT>::doIt() {
	_numCyclesCompleted = 0;
	if (_cels.size()) {
		const uint32 now = _timeManager->getTickCount();
		for (uint i = 0; i < _cels.size(); ++i) {
			if (now >= _timings[i]) {
				// SSCI did not try to correct for lag in this calculation
				const auto ticksToNextMove = MAX(0, _cels[i]->getCycleSpeed() - int(now - _timings[i]));
				_timings[i] = now + ticksToNextMove;
				_cels[i]->setCel(nextCel(*_cels[i]), true);
			}
		}
	}
	if (_numCyclesCompleted == _cels.size()) {
		_isFinished = true;
		done();
	}
}

template <class CelT>
int16 AbsGLCycler<CelT>::nextCel(CelT &client) {
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

template <class CelT>
void AbsGLCycler<CelT>::done() {
	stop();
	release();
	if (_caller) {
		new GLCue(_caller);
		_caller = nullptr;
	}
}

template <class CelT>
void AbsGLCycler<CelT>::release() {
	for (auto &cel : _cels) {
		cel->setCycler(nullptr);
	}
	_cels.clear();
	_timings.clear();
}

template <class CelT>
int16 AbsGLEndCycler<CelT>::nextCel(CelT &client) {
	auto cel = client.getCel() + this->getDirection();
	if (cel < 0) {
		cel = 0;
		this->incrementCycle();
	}
	const auto lastCel = client.getLastCel();
	if (cel > lastCel) {
		cel = lastCel;
		this->incrementCycle();
	}
	return cel;
}

template <class CelT>
int16 AbsGLEndBackCycler<CelT>::nextCel(CelT &client) {
	const auto cel = client.getCel();
	this->cycleForward(false);
	if (cel == 0) {
		this->incrementCycle();
		return cel;
	} else {
		return cel + this->getDirection();
	}
}

template <class CelT>
int16 AbsGLEndForwardCycler<CelT>::nextCel(CelT &client) {
	const auto cel = client.getCel();
	if (cel >= client.getLastCel()) {
		this->incrementCycle();
		return cel;
	} else {
		return cel + this->getDirection();
	}
}

template <class CelT>
int16 AbsGLEndForwardBackwardCycler<CelT>::nextCel(CelT &client) {
	auto cel = client.getCel();
	if (cel == 0 && !this->cycleForward()) {
		this->incrementCycle();
		this->cycleForward(true);
	} else if (cel == client.getLastCel()) {
		this->cycleForward(false);
		cel += this->getDirection();
	} else {
		cel += this->getDirection();
	}
	return cel;
}

template <class CelT>
int16 AbsGLPingPongCycler<CelT>::nextCel(CelT &client) {
	int16 lastCel = client.getLastCel();
	int16 cel = client.getCel();
	if (lastCel != 0) {
		if (cel == 0) {
			this->cycleForward(true);
		} else if (cel == lastCel) {
			this->cycleForward(false);
		}

		return cel + this->getDirection();
	}

	// In SSCI this would return garbage memory if lastCel was 0
	return cel;
}

template <class CelT>
int16 AbsGLStartResetCycler<CelT>::nextCel(CelT &client) {
	auto cel = client.getCel();
	if (cel == 0) {
		this->incrementCycle();
		return client.getLastCel();
	} else {
		this->cycleForward(false);
		return cel + this->getDirection();
	}
}

template class AbsGLCycler<GLCel>;
template class AbsGLEndCycler<GLCel>;
template class AbsGLEndBackCycler<GLCel>;
template class AbsGLEndForwardCycler<GLCel>;
template class AbsGLPingPongCycler<GLCel>;
template class AbsGLEndForwardBackwardCycler<GLCel>;

template class AbsGLCycler<S2PanoramaSprite>;
template class AbsGLEndForwardCycler<S2PanoramaSprite>;
template class AbsGLEndForwardBackwardCycler<S2PanoramaSprite>;
template class AbsGLStartResetCycler<S2PanoramaSprite>;

} // End of namespace Sci
