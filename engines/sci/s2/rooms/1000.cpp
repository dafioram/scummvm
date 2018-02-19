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

#include "common/textconsole.h"
#include "sci/s2/game.h"
#include "sci/s2/hotspot.h"
#include "sci/s2/inventory.h"
#include "sci/s2/rooms/1000.h"
#include "sci/s2/system/glcel.h"
#include "sci/s2/system/glcycler.h"
#include "sci/s2/system/types.h"

namespace Sci {

void S2Room1000::init(const int roomNo) {
	switch (roomNo) {
	case 1000:
		_game.getRoomManager().drawPan(10400);
		_game.getRoomManager().drawPic(1, true);
		setScript(this, &S2Room1000::logoScript);
		break;
	case 1010:
		_game.getRoomManager().drawPic(2);
		setScript(this, &S2Room1000::openingScript);
		break;
	case 1015:
		_game.getRoomManager().drawPic(1910);
		_game.getInterface().resetButtons();
		// In SSCI the half-screen check happened on every step of the game
		// script, but this would cause a null pointer dereference if the game
		// started fullscreen and then switched to halfscreen by keyboard
		// shortcut, so the entire check is just moved here to the room init
		if (_game.getSaveGameName() == "WACKY" &&
			_game.getMovieManager().getUseHalfScreen()) {
			_isWacky = true;
		}
		_game.getSoundManager().play(31002, true, 70);
		setScript(this, &S2Room1000::checkInScript);
		break;
	case 1020:
		_game.getRoomManager().drawPic(2);
		_game.getInterface().putText(0);
		setScript(this, &S2Room1000::dreamMovieScript);
		break;
	case 1500:
	case 1600:
		warning("TODO: Room %d", roomNo);
		break;
	}
}

void S2Room1000::dispose(const int roomNo) {
	switch (roomNo) {
	case 1015:
		_cycler.reset();
		_game.getSoundManager().fade(31002, 0, 15, 12, true);
		_game.getMovieManager().stopRobot(false);
		break;
	}

	S2Room::dispose(roomNo);
}

bool S2Room1000::handleEvent(GLEvent &event) {
	return false;
}

void S2Room1000::logoScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_game.getMovieManager().play(1000, nullptr, GLPoint(0, 0));
		script.setTicks(1);
		break;
	case 1:
		_game.getRoomManager().loadGlobalRoom(4000, true);
		break;
	}
}

void S2Room1000::openingScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_game.getMovieManager().play(1010, nullptr, GLPoint(64, 0));
		script.setTicks(1);
		break;
	case 1:
		_game.getRoomManager().setNextRoomNo(1015);
		break;
	}
}

void S2Room1000::checkInScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		resetHotspot();

		resetCel(0, 1910, 0, 0, GLPoint(64, 383), 200);

		if (_isWacky) {
			auto &wackyCel = resetCel(2, 1910, 3, 0, GLPoint(64, 383), 200);
			wackyCel.hide();
			wackyCel.setCycleSpeed(12);
			getPlane().getCast().remove(wackyCel);
			_cycler.reset(new GLPingPongCycler());
			_cycler->add(wackyCel, false);
		}

		script.setSeconds(5);
		break;
	case 1:
		if (_game.getMovieManager().getUseHalfScreen()) {
			script.setSeconds(1);
		} else {
			playRobot(script, 1900);
		}
		break;
	case 2:
		// TODO: Is this cycle not working?
		script.setSeconds(5);
		script.setState(0);
		break;
	case 3:
		_cels[0]->setLoop(1);
		_cels[0]->forceUpdate();
		if (_game.getMovieManager().getUseHalfScreen()) {
			if (_isWacky) {
				_cels[2]->show();
				_cycler->start();
			}
			script.setSeconds(4);
		} else {
			playRobot(script, 1901);
		}
		break;
	case 4:
		playRobotOrSound(script, 1902, 41101, 2);
		break;
	case 5:
		_cels[0]->hide();
		if (_isWacky) {
			_cycler->stop();
			_cels[2]->hide();
		}
		_game.getRoomManager().drawPic(1920);
		resetCel(1, 1920, 0, 0, GLPoint(64, 383), 203);
		break;
	case 6:
		_cels[0]->hide();
		_game.getRoomManager().drawPic(1920);
		resetCel(1, 1920, 0, 1, GLPoint(64, 383), 203);
		break;
	case 7:
		_cels[0]->show();
		_game.getRoomManager().drawPic(1910);
		if (_isWacky) {
			_cels[2]->show();
			_cycler->start();
		}
		playRobotOrSound(script, 1903, 41109, 0);
		break;
	case 8: {
		uint16 robotNo = 1905;
		uint16 soundNo = 41114;
		if (_useAltMovie) {
			++robotNo;
			++soundNo;
		}
		_useAltMovie = !_useAltMovie;
		playRobotOrSound(script, robotNo, soundNo, 2);
		break;
	}
	case 9:
		script.setState(7);
		script.setSeconds(5);
		if (_isWacky) {
			_cycler->stop();
			_cels[2]->setLoop(3);
			_cycler->start();
		}
		break;
	case 10:
		playRobotOrSound(script, 1907, 41116, 0);
		break;
	case 11:
		_cels[0]->hide();
		if (_isWacky) {
			_cycler->stop();
			_cels[2]->hide();
		}
		flushEvents();
		_game.getRoomManager().drawPic(1920);
		resetCel(1, 1920, 1, 0, GLPoint(64, 383), 202);
		break;
	case 12:
		_cels[0]->show();
		_game.getRoomManager().drawPic(1910);
		if (_isWacky) {
			_cels[2]->show();
			_cycler->start();
		}
		playRobotOrSound(script, 1909, 41122, 0);
		break;
	case 13:
		if (_isWacky) {
			_cycler.reset();
		}
		if (!_game.getMovieManager().getUseHalfScreen()) {
			_game.getMovieManager().stopRobot(false);
		}
		_script.reset();
		_game.getRoomManager().setNextRoomNo(1020);
		break;
	}
}

void S2Room1000::dreamMovieScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_game.getMovieManager().play(1020, nullptr, GLPoint(64, 0));
		script.setTicks(1);
		break;
	case 1:
		_game.getRoomManager().setNextRoomNo(10400);
		break;
	}
}

void S2Room1000::playRobot(GLScript &script, const uint16 robotNo) {
	_game.getMovieManager().initRobot(robotNo, getPlane(), 200, GLPoint(64, 0));
	_game.getMovieManager().setRobotCaller(script);
	_game.getMovieManager().setRobotClient(*_cels[0]);
	_game.getMovieManager().playRobot(false, true, false);
}

void S2Room1000::playRobotOrSound(GLScript &script, const uint16 robotNo, const uint16 soundNo, const int16 wackyLoop) {
	if (_game.getMovieManager().getUseHalfScreen()) {
		if (wackyLoop && _isWacky) {
			_cycler->stop();
			_cels[2]->setLoop(wackyLoop);
			_cycler->start();
		}
		_soundNo = soundNo;
		_game.getSoundManager().play(_soundNo, false, Audio32::kMaxVolume, false, &script);
	} else {
		playRobot(script, robotNo);
	}

	_game.getInterface().putText(soundNo);
}

void S2Room1000::resetHotspot() {
	_hotspots.resize(1);
	_hotspots[0].reset(new S2Hotspot(getPlane(), 224, 120, 464, 360));
	_hotspots[0]->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
		if (event.getType() != kSciEventMouseRelease) {
			return;
		}

		if (_cels.size() > 1) {
			_cels[1].reset();
		}

		if (_game.getMovieManager().getUseHalfScreen()) {
			_game.getSoundManager().stop(_soundNo);
		} else {
			_game.getMovieManager().stopRobot(false);
		}

		switch (_script->getState()) {
		case 0:
		case 1:
		case 2:
			resetState(3);
			break;
		case 3:
		case 4:
		case 6:
		case 7:
		case 10:
		case 12:
			resetState(_script->getState() + 1);
			break;
		case 5:
			_game.getSoundManager().play(12409, false, 100);
			resetState(6);
			break;
		case 8:
		case 9:
			if (_game.getInventoryManager().getState(S2Inventory::FortyDollars) == S2InventoryState::InUse) {
				_game.getInventoryManager().unselectItem(false);
				resetState(10);
			} else {
				resetState(8);
			}
			break;
		case 11:
			_game.getSoundManager().play(11118, false, 100);
			_game.getInventoryManager().addItem(S2Inventory::Change);
			resetState(12);
			break;
		}

		flushEvents();
	});

	_game.getRoomManager().addHotspot(*_hotspots[0]);
}

} // End of namespace Sci
