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

#include "sci/graphics/frameout.h"
#include "sci/s2/game.h"
#include "sci/s2/kernel.h"
#include "sci/s2/movie_manager.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/glrobot.h"

namespace Sci {

S2MovieManager::S2MovieManager(S2Kernel &kernel, S2Game &game) :
	_kernel(kernel),
	_game(game),
	_robotIsInitialized(false),
	_preventSkip(false),
	_useHalfScreen(false),
	_frameNo(5) {
	GLVmdMovie::init(&kernel.graphicsManager._video.getVMDPlayer());
}

void S2MovieManager::initRobot(const uint16 robotNo, AbsGLPlane &plane, const int16 priority, const GLPoint &position) {
	_robot = GLRobot(robotNo, plane, priority, position);
	_robotIsInitialized = true;
}

void S2MovieManager::playRobot(const bool isModal, const bool hideClient, const bool keepLastFrame) {
	if (_robotIsInitialized) {
		_robot.start(isModal, hideClient, keepLastFrame);
		if (_game.getRoomManager().getCurrentGlobalRoomNo()) {
			pauseRobot();
		}
	}
}

void S2MovieManager::pauseRobot() {
	if (_robotIsInitialized) {
		_robot.pause();
	}
}

void S2MovieManager::resumeRobot() {
	if (_robotIsInitialized) {
		_robot.resume();
	}
}

void S2MovieManager::stopRobot(const bool shouldCue) {
	if (_robotIsInitialized) {
		_robot.terminate(shouldCue);
		_robotIsInitialized = false;
	}
}

void S2MovieManager::setRobotCaller(GLObject &caller) {
	if (_robotIsInitialized) {
		_robot.setCaller(caller);
	}
}

void S2MovieManager::setRobotClient(GLCel &cel) {
	if (_robotIsInitialized) {
		_robot.setClient(cel);
	}
}

void S2MovieManager::play(const uint16 movieNo, Captioner captioner, const GLPoint &position, const bool forceDoublePixels, const bool keepRoom) {
	_game.getPhoneManager().cancelCall();
	auto roomNo = _game.getSoundManager().getRoomNo();
	_game.getRoomManager().setLastSoundRoomNo(roomNo);
	_game.getSoundManager().deleteAmbient(roomNo);
	_game.getSoundManager().stop();

	if (!keepRoom) {
		_game.getRoomManager().deactivateRoom();
	}

	if (!captioner) {
		// TODO: A big switch
	}

	_movie = GLVmdMovie(movieNo);

	{
		GLTransparentPlane plane(Common::Rect(_kernel.graphicsManager.getScriptWidth(), _kernel.graphicsManager.getScriptHeight()), 4);

		// HACK: Allow HQ video as long as there is no captioning (since the
		// captions need to composite on top of the video); normally this was
		// always 200
		const int16 planePriority = captioner ? 200 : 0;

		_kernel.graphicsManager._video.getVMDPlayer().setPlane(planePriority, plane.getId());

		if ((keepRoom && forceDoublePixels) || !_useHalfScreen) {
			const int flags = VMDPlayer::kPlayFlagDoublePixels | VMDPlayer::kPlayFlagBlackLines;
			_movie.setPosition(position, VMDPlayer::PlayFlags(flags), true);
		} else {
			// TODO: position was not const in SSCI and got updated directly, is
			// this a desirable side-effect?
			const GLPoint newPosition(192, 98);
			_movie.setPosition(newPosition, VMDPlayer::kPlayFlagNone, true);
		}

		// Ignored cache size set here
		_movie.showCursor(false);

		int playFlags = VMDPlayer::kEventFlagNone;
		if (!_preventSkip) {
			playFlags |= VMDPlayer::kEventFlagMouseDown;
		}

		if (captioner && _game.getInterface().getIsCaptioningOn()) {
			VMDPlayer::EventFlags result;
			playFlags |= VMDPlayer::kEventFlagToFrame;
			do {
				captioner(*this);
				_movie.setMovieEvent(VMDPlayer::EventFlags(playFlags), _frameNo);
				result = _player.play(_movie);
			} while (result == VMDPlayer::kEventFlagEnd && _frameNo != 9999);
		} else {
			playFlags |= VMDPlayer::kEventFlagEnd;
			_movie.setMovieEvent(VMDPlayer::EventFlags(playFlags), _frameNo);
			_player.play(_movie);
		}

		_player.close();
		_game.getInterface().putText(0);
		_game.getSoundManager().createAmbient(_game.getRoomManager().getLastSoundRoomNo());
	}

	if (!keepRoom) {
		_game.getRoomManager().activateRoom();
	}

	_kernel.eventManager.flushEvents();
	_movie.showCursor(true);
	_frameNo = 5;
	_kernel.graphicsManager.kernelFrameOut(true);
}

} // End of namespace Sci
