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

class S2CheckInRoom : public S2SubRoom {
public:
	using S2SubRoom::S2SubRoom;
	virtual void init(const int) override {
		room().drawPic(1910);
		interface().resetButtons();
		// In SSCI the half-screen check happened on every step of the game
		// script, but this would cause a null pointer dereference if the game
		// started fullscreen and then switched to halfscreen by keyboard
		// shortcut, so the entire check is just moved here to the room init
		_isWacky = (_game.getSaveGameName() == "WACKY" &&
					movie().getUseHalfScreen());
		sound().play(31002, true, 70);
		setScript(this, &S2CheckInRoom::checkInScript);
	}

	virtual void dispose(const int) override {
		sound().fade(31002, 0, 15, 12, true);
		movie().stopRobot(false);
	}

private:
	void checkInScript(GLScript &script, const int state) {
		switch (state) {
		case 0: {
			auto &hotspot = emplaceHotspot(true, 224, 120, 464, 360);
			hotspot.setMouseUpHandler(this, &S2CheckInRoom::interact);

			_charles = &emplaceCel(false, 1910, 0, 0, roomBottom, 200);
			_charles->show();

			if (_isWacky) {
				auto &wackyCel = emplaceCel(false, 1910, 3, 0, roomBottom, 200);
				wackyCel.setCycleSpeed(12);
				_wackyFace = &wackyCel;
				getPlane().getCast().remove(wackyCel);
				_wackyCycler.reset(new GLPingPongCycler());
				_wackyCycler->add(wackyCel, false);
			}

			script.setSeconds(5);
			break;
		}
		case 1:
			if (movie().getUseHalfScreen()) {
				script.setSeconds(1);
			} else {
				playRobot(script, 1900);
			}
			break;
		case 2:
			script.setSeconds(5);
			script.setState(0);
			break;
		case 3:
			_charles->setLoop(1, true);
			if (movie().getUseHalfScreen()) {
				if (_isWacky) {
					_wackyFace->show();
					_wackyCycler->start();
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
			_charles->hide();
			if (_isWacky) {
				_wackyCycler->stop();
				_wackyFace->hide();
			}
			room().drawPic(1920);
			_deskItem.reset(new GLCel(getPlane(), 1920, 0, 0, roomBottom, 203));
			_deskItem->show();
			break;
		case 6:
			_charles->hide();
			room().drawPic(1920);
			_deskItem.reset(new GLCel(getPlane(), 1920, 0, 1, roomBottom, 203));
			_deskItem->show();
			break;
		case 7:
			_charles->show();
			room().drawPic(1910);
			if (_isWacky) {
				_wackyFace->show();
				_wackyCycler->start();
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
				_wackyCycler->stop();
				_wackyFace->setLoop(3);
				_wackyCycler->start();
			}
			break;
		case 10:
			playRobotOrSound(script, 1907, 41116, 0);
			break;
		case 11:
			_charles->hide();
			if (_isWacky) {
				_wackyCycler->stop();
				_wackyFace->hide();
			}
			flushEvents();
			room().drawPic(1920);
			_deskItem.reset(new GLCel(getPlane(), 1920, 1, 0, roomBottom, 202));
			_deskItem->show();
			break;
		case 12:
			_charles->show();
			room().drawPic(1910);
			if (_isWacky) {
				_wackyFace->show();
				_wackyCycler->start();
			}
			playRobotOrSound(script, 1909, 41122, 0);
			break;
		case 13:
			if (_isWacky) {
				_wackyCycler.reset();
			}
			if (!movie().getUseHalfScreen()) {
				movie().stopRobot(false);
			}
			_script.reset();
			room().setNextRoomNo(1020);
			break;
		}
	}

	void interact(GLEvent &event, GLTarget &) {
		if (event.getType() != kSciEventMouseRelease) {
			return;
		}

		if (_deskItem) {
			_deskItem.reset();
		}

		if (movie().getUseHalfScreen()) {
			sound().stop(_soundNo);
		} else {
			movie().stopRobot(false);
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
			sound().play(12409, false, 100);
			resetState(6);
			break;
		case 8:
		case 9:
			if (inventory().isInUse(S2Inventory::FortyDollars)) {
				inventory().unselectItem(false);
				resetState(10);
			} else {
				resetState(8);
			}
			break;
		case 11:
			sound().play(11118, false, 100);
			inventory().addItem(S2Inventory::Change2_50);
			resetState(12);
			break;
		}

		flushEvents();
	}

	void playRobot(GLScript &script, const uint16 robotNo) {
		movie().initRobot(robotNo, getPlane(), 200, roomTop);
		movie().setRobotCaller(script);
		movie().setRobotClient(*_charles);
		movie().playRobot(false, true, false);
	}

	void playRobotOrSound(GLScript &script, const uint16 robotNo, const uint16 soundNo, const int16 wackyLoop) {
		if (movie().getUseHalfScreen()) {
			if (wackyLoop && _isWacky) {
				_wackyCycler->stop();
				_wackyFace->setLoop(wackyLoop);
				_wackyCycler->start();
			}
			_soundNo = soundNo;
			sound().play(_soundNo, false, Audio32::kMaxVolume, false, &script);
		} else {
			playRobot(script, robotNo);
		}

		interface().putText(soundNo);
	}

	int _soundNo = 0;
	bool _useAltMovie = false;
	bool _isWacky = false;
	GLCel *_charles;
	GLCel *_wackyFace;
	Common::ScopedPtr<GLCel> _deskItem;
	Common::ScopedPtr<GLCycler> _wackyCycler;
};

void S2Room1000::init(const int roomNo) {
	switch (roomNo) {
	case 1000:
		room().drawPan(10400);
		room().drawPic(1, true);
		setScript(this, &S2Room1000::logoScript);
		break;
	case 1010:
		room().drawPic(2);
		setScript(this, &S2Room1000::openingScript);
		break;
	case 1015:
		setSubRoom<S2CheckInRoom>();
		_activeSubRoom->init(roomNo);
		break;
	case 1020:
		room().drawPic(2);
		interface().putText(0);
		setScript(this, &S2Room1000::dreamMovieScript);
		break;
	case 1500:
	case 1600:
		warning("TODO: Room %d", roomNo);
		break;
	}
}

void S2Room1000::logoScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		movie().play(1000, nullptr, GLPoint(0, 0));
		script.setTicks(1);
		break;
	case 1:
		room().loadGlobalRoom(4000, true);
		break;
	}
}

void S2Room1000::openingScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		movie().play(1010, nullptr, GLPoint(64, 0));
		script.setTicks(1);
		break;
	case 1:
		room().setNextRoomNo(1015);
		break;
	}
}

void S2Room1000::dreamMovieScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		movie().play(1020, nullptr, GLPoint(64, 0));
		script.setTicks(1);
		break;
	case 1:
		room().setNextRoomNo(10400);
		break;
	}
}

} // End of namespace Sci
