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

#include "sci/event.h"
#include "sci/graphics/frameout.h"
#include "sci/s2/system/glcue.h"
#include "sci/s2/system/glevent.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/glrobot.h"
#include "sci/s2/system/glscreen_item.h"
#include "sci/video/robot_decoder.h"

namespace Sci {

GfxFrameout *GLRobot::_graphicsManager = nullptr;
RobotDecoder *GLRobot::_robotPlayer = nullptr;

GLRobot::GLRobot() :
	GLObject(),
	_state(0) {}

GLRobot::GLRobot(const uint16 robotNo, AbsGLPlane &plane, const int16 priority, const GLPoint &position) :
	GLObject(),
	_robotNo(robotNo),
	_plane(&plane),
	_priority(priority),
	_position(position),
	_caller(nullptr),
	_client(nullptr),
	_isDirty(false),
	_state(kDisposeAfterUse) {
	setNeedsDoIt(true);
	init();
}

GLRobot::~GLRobot() {
	if ((_state & (kStarted | kFinished)) == kStarted) {
		terminate();
	}
}

bool GLRobot::getIsPlaying() const {
	return _robotPlayer->getStatus() != RobotDecoder::kRobotStatusUninitialized;
}

void GLRobot::start(const bool isModal, const bool hideClient, const bool keepLastFrame) {
	if ((_state & kFinished) || _isDirty) {
		init();
	}

	_robotPlayer->showFrame(0, RobotDecoder::kUnspecified, RobotDecoder::kUnspecified, RobotDecoder::kUnspecified);

	_state |= kStarted;
	_signal = 0;
	// broken state AND-NOT here in SSCI
	if (keepLastFrame) {
		_state |= kKeepLastFrame;
	}
	if (hideClient && _client) {
		_state |= kHideClient;
		_client->hide();
	}
	_plane->getCast().add(*this);
	_robotPlayer->resume();
	if (isModal) {
		GLEvent event;
		while (_robotPlayer->getStatus() != RobotDecoder::kRobotStatusEnd) {
			_graphicsManager->kernelFrameOut(true);

			// Checks to bypass the robot on quit did not exist in SSCI
			event.refresh(kSciEventQuit | kSciEventPeek);
			if (event.getType() == kSciEventQuit) {
				break;
			}
		}
		done();
	}
}

void GLRobot::pause() {
	if (getIsPlaying()) {
		_robotPlayer->pause();
		_state |= kPaused;
	}
}

void GLRobot::resume() {
	if (getIsPlaying() & (_state & kPaused)) {
		_robotPlayer->resume();
		_state &= ~kPaused;
	}
}

void GLRobot::terminate(const bool shouldCue) {
	_robotPlayer->close();
	cleanUp(shouldCue);
}

void GLRobot::doIt() {
	if ((_state & (kStarted | kPaused)) == kStarted) {
		_signal = _robotPlayer->getCue();
		if (_signal == -1) {
			done();
		} else if (_caller && _signal) {
			new GLCue(_caller);
		}
	}
}

void GLRobot::init() {
	_robotPlayer->open(_robotNo, _plane->getId(), _priority, _position.x, _position.y, 128);
	_isDirty = false;
}

void GLRobot::done() {
	// TODO: This looks broken in SSCI.
	if (_state & ~kKeepLastFrame) {
		_robotPlayer->close();
	}

	cleanUp(true);
}

void GLRobot::cleanUp(const bool shouldCue) {
	_state |= kFinished;

	if ((_state & kHideClient) && _client) {
		_client->show();
	}

	if (shouldCue && _caller) {
		_signal = -1;
		new GLCue(_caller);
	}

	_plane->getCast().remove(*this);
}

} // End of namespace Sci
