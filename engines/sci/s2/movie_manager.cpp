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
	_frameNo(5),
	_shouldHypnotise(false) {
	GLVmdMovie::init(&kernel.graphicsManager._video.getVMDPlayer());
}

void S2MovieManager::saveLoadWithSerializer(Common::Serializer &s) {
	s.syncAsByte(_useHalfScreen);
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

void S2MovieManager::play(const uint16 movieNo, const GLPoint &position, const bool forceDoublePixels, const bool keepRoom) {
	_game.getPhoneManager().cancelCall();
	auto roomNo = _game.getSoundManager().getRoomNo();
	_game.getRoomManager().setLastSoundRoomNo(roomNo);
	_game.getSoundManager().deleteAmbient(roomNo);
	_game.getSoundManager().stop();

	if (!keepRoom) {
		_game.getRoomManager().deactivateRoom();
	}

	auto captioner = S2Captions::get(movieNo);

	_movie = GLVmdMovie(movieNo);

	{
		GLTransparentPlane plane(Common::Rect(_kernel.graphicsManager.getScriptWidth(), _kernel.graphicsManager.getScriptHeight()), 4);

		const bool isCaptioned = captioner && _game.getInterface().getIsCaptioningOn();

		// HACK: Allow HQ video as long as there is no captioning (since the
		// captions need to composite on top of the video); normally this was
		// always 200
		const int16 planePriority = isCaptioned ? 200 : 0;

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
			// SSCI used mouse down, which did not correctly flush the mouse up
			// event (and the game mostly uses mouse up events for action
			// triggering)
			playFlags |= VMDPlayer::kEventFlagMouseUp;
		}

		if (isCaptioned) {
			VMDPlayer::EventFlags result;
			playFlags |= VMDPlayer::kEventFlagToFrame;
			do {
				captioner(*this, _game.getInterface());
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
	_kernel.graphicsManager.frameOut(true);
}

void S2MovieManager::play(const uint16 tapeId, const bool blockHypnotism) {
	_shouldHypnotise = !blockHypnotism;
	_game.getRoomManager().setLastSoundRoomNo(_game.getSoundManager().getRoomNo());
	_game.getSoundManager().deleteAmbient(_game.getSoundManager().getRoomNo());
	_game.getSoundManager().stop();

	uint16 movieNo;
	switch (tapeId) {
	case 0:
		if (_shouldHypnotise) {
			_game.getScoringManager().doEvent(kScore5);
			if (!_game.getFlags().get(kGameFlag31)) {
				_game.getFlags().set(kGameFlag31);
				if (_game.getInventoryManager().hasPrayerStick()) {
					_game.getFlags().set(kGameFlag32);
				}
				movieNo = 3002;
			} else if (!_game.getFlags().get(kGameFlag33)) {
				_game.getFlags().set(kGameFlag33);
				_game.getFlags().set(kGameFlag32);
				movieNo = 3003;
			} else if (!_game.getFlags().get(kGameFlag34)) {
				_game.getFlags().set(kGameFlag34);
				if (_game.getInventoryManager().hasPrayerStick()) {
					_game.getFlags().set(kGameFlag35);
				}
				movieNo = 3001;
			} else {
				_game.getFlags().set(kGameFlag34);
				_game.getFlags().set(kGameFlag35);
				movieNo = 3001;
			}
		} else if (!_game.getFlags().get(kGameFlag75)) {
			movieNo = 2003;
		} else if (!_game.getFlags().get(kGameFlag77)) {
			movieNo = 2001;
		} else if (!_game.getFlags().get(kGameFlag74)) {
			movieNo = 2004;
		} else if (!_game.getFlags().get(kGameFlag79)) {
			movieNo = 2000;
		} else if (!_game.getFlags().get(kGameFlag76)) {
			movieNo = 2006;
		} else if (!_game.getFlags().get(kGameFlag78)) {
			movieNo = 2005;
		} else if (!_game.getFlags().get(kGameFlag72) && _game.getFlags().get(kGameFlag106)) {
			movieNo = 2002;
		} else if (_game.getFlags().get(kGameFlag15) && !_game.getFlags().get(kGameFlag73)) {
			movieNo = 2008;
		} else if (!_game.getFlags().get(kGameFlag80) && _game.getFlags().get(kGameFlag69)) {
			movieNo = 2007;
		} else {
			for (;;) {
				movieNo = _game.getRandomNumber(2000, 2012);
				// In SSCI the check for movie 2007 read uninitialised stack
				// memory. It is not totally clear what these checks were for,
				// seems like they were supposed to be matching parts of the
				// earlier preconditions for these videos so they did not play
				// unless they played once before, so that is what we do
				if ((movieNo != 2007 || _game.getFlags().get(kGameFlag80)) &&
					(movieNo != 2008 || _game.getFlags().get(kGameFlag73))) {
					break;
				}
			}
		}

		// SSCI also selected captioner here but there is no reason to do this
		// since it can be selected in the main video playback function
		switch (movieNo) {
		case 2000:
			_game.getFlags().set(kGameFlag79);
			break;
		case 2001:
			_game.getFlags().set(kGameFlag77);
			break;
		case 2002:
			_game.getFlags().set(kGameFlag72);
			break;
		case 2003:
			_game.getFlags().set(kGameFlag75);
			break;
		case 2004:
			_game.getFlags().set(kGameFlag74);
			break;
		case 2005:
			_game.getFlags().set(kGameFlag78);
			break;
		case 2006:
			_game.getFlags().set(kGameFlag76);
			break;
		case 2007:
			_game.getFlags().set(kGameFlag80);
			break;
		case 2008:
			_game.getFlags().set(kGameFlag73);
			break;
		}
		break;
	case 1:
		_game.getFlags().set(kGameFlag81);
		_shouldHypnotise = false;
		movieNo = 4000;
		break;
	case 2:
		_game.getFlags().set(kGameFlag82);
		_shouldHypnotise = false;
		movieNo = 4010;
		break;
	default:
		error("Unknown video tape ID %d", tapeId);
	}

	if (_shouldHypnotise) {
		_kernel.graphicsManager._palette.setCycle(195, 197, 1, 10);
		_kernel.graphicsManager._palette.doCycle(195, 1);
	}

	play(movieNo, GLPoint(159, 70), true, true);

	if (_shouldHypnotise) {
		_kernel.graphicsManager._palette.cycleOff(195);

		int nextRoom;
		switch (_game.getRoomManager().getCurrentAmbientRoomNo()) {
		case 10:
		case 11:
			nextRoom = 6310;
			break;
		case 13:
			nextRoom = 6100;
			break;
		case 14:
			nextRoom = 6250;
			break;
		case 15:
			nextRoom = 6190;
			break;
		case 16:
			nextRoom = 6240;
			break;
		case 17:
			nextRoom = 6120;
			break;
		case 18:
		case 19:
			nextRoom = 6270;
			break;
		}

		if (_game.getFlags().get(kGameFlag35)) {
			if (_someHypnotise) {
				_game.getRoomManager().setNextRoomNo(28700);
			} else {
				_game.getRoomManager().setNorah(59004, 6540);
				_game.getRoomManager().setNextRoomNo(6666);
			}
		} else if (_game.getFlags().get(kGameFlag34)) {
			_game.getRoomManager().setNorah(59003, 6540);
			_game.getRoomManager().setNextRoomNo(6666);
		} else if (_game.getFlags().get(kGameFlag33)) {
			_game.getRoomManager().setNextRoomNo(6250);
		} else if (_game.getFlags().get(kGameFlag32)) {
			_game.getRoomManager().setNorah(59002, nextRoom);
			_game.getRoomManager().setNextRoomNo(6666);
		} else {
			_game.getRoomManager().setNorah(59001, nextRoom);
			_game.getRoomManager().setNextRoomNo(6666);
		}
	} else {
		_game.getRoomManager().setNextRoomNo(_game.getRoomManager().getPreviousRoomNo());
	}
}

} // End of namespace Sci
