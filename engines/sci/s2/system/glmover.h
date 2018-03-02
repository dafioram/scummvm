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

#ifndef SCI_S2_SYSTEM_GLMOVER_H
#define SCI_S2_SYSTEM_GLMOVER_H

#include "sci/s2/system/globject.h"
#include "sci/s2/system/types.h"

namespace Sci {

class GLCel;

// In SSCI this extended GLObject
class GLBresen {
public:
	GLBresen() : _current(nullptr) {}
	GLBresen(GLPoint &start, const GLPoint &end, const GLPoint &step);
	bool doMove();

private:
	GLPoint *_current;
	GLPoint _end;
	GLPoint _step;
	GLPoint _extra;
	int _error;
	int _remainder;
	int _correction;
	int _stepsLeft;
};

// In SSCI this extended GLObject
class GLArc {
public:
	GLArc() : _current(nullptr) {}
	GLArc(GLPoint &start, const GLPoint &end, const GLPoint &step, const int gravity);
	bool doMove();

private:
	GLPoint *_current;
	GLPoint _end;
	GLPoint _step;
	GLPoint _gravity;
	int _stepsLeft;
};

template <class CelT, class MoverT>
class AbsGLMover : public GLObject {
public:
	template <class MT = MoverT>
	AbsGLMover(CelT &client, const GLPoint &target) :
		_client(&client),
		_position(client.getPosition()) {
		start(target);
	}
	template <class MT = MoverT>
	AbsGLMover(CelT &client, const GLPoint &target, GLObject &caller) :
		_client(&client),
		_position(client.getPosition()),
		_caller(&caller) {
		start(target);
	}

	template <class MT = MoverT>
	AbsGLMover(CelT &client, const GLPoint &target, const int gravity) :
		_client(&client),
		_position(client.getPosition()) {
		start(target, gravity);
	}

	template <class MT = MoverT>
	AbsGLMover(CelT &client, const GLPoint &target, GLObject &caller, const int gravity) :
		_client(&client),
		_position(client.getPosition()),
		_caller(&caller) {
		start(target, gravity);
	}

	virtual ~AbsGLMover();

	static void init(TimeManager *timeManager) { _timeManager = timeManager; }
	static void init(GLExtras *extras) { _extras = extras; }

	template <class MT = MoverT>
	void start(const GLPoint &target);
	template <class MT = MoverT>
	void start(const GLPoint &target, const int gravity);

	virtual void doIt() override;

protected:
	virtual void nextDest();

private:
	bool doStep();
	void done();
	void stop();

	static TimeManager *_timeManager;
	static GLExtras *_extras;
	CelT *_client;
	GLObject *_caller = nullptr;
	GLPoint _position;
	GLPoint _end;
	MoverT _bresen;
	GLArc _arc;
	uint32 _nextTick;
};

using GLMover = AbsGLMover<GLCel, GLBresen>;
using GLJump = AbsGLMover<GLCel, GLArc>;

} // End of namespace Sci

#endif
