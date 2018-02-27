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

#include "sci/s2/game.h"
#include "sci/s2/rooms/10000.h"
#include "sci/s2/system/glcycler.h"
#include "sci/s2/system/glpanorama.h"

namespace Sci {

void S2Room10000::init(const int roomNo) {
	switch (roomNo) {
	case 10000:
	case 10100: {
		_game.getSoundManager().createAmbient(10);
		_game.getFlags().set(kGameFlag91);

		if (_game.getRoomManager().getPreviousRoomNo() == 6312) {
			_game.getRoomManager().getPanorama().setPanX(1319);
			_game.getSoundManager().play(11003, false);
		}

		_game.getRoomManager().drawPan(10100);

		auto &panorama = _game.getRoomManager().getPanorama();
		panorama.addExit(10200, 100, 150, 300, 450);
		panorama.addExit(10300, 1350, 200, 1600, 400);
		panorama.addExit(10400, 1600, 150, 1900, 450);
		panorama.addExit(10110, 413, 152, 589, 375, S2Cursor::kHighlightCel);
		break;
	}

	case 10110: {
		_game.getRoomManager().drawPic(10110);
		_cancelSoundNo = 11003;
		setScript([&](GLScript &script, const int state) {
			switch (state) {
			case 0:
				_game.getUser().setIsHandsOn(false);
				_cel.reset(new GLCel(getPlane(), roomNo, 0, 0, roomBottom));
				_cycler.reset(new GLEndCycler());
				_cycler->add(*_cel);
				_cycler->start(script);
				_game.getSoundManager().play(11004, false, 120);
				break;
			case 1:
				getPlane().getCast().remove(*_cel);
				_script.reset();
				_cycler.reset();
				_game.getUser().setIsHandsOn(true);
				break;
			}
		});
		emplaceExit(true, 10999, S2Cursor::kBackCel);
		emplaceExit(true, 6310, 246, 90, 392, 354);
		break;
	}

	case 10120:
		_game.getRoomManager().drawPic(10120);
		emplaceExit(true, 10999, 64, 0, 144, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10999, 495, 0, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10999, 145, 0, 494, 80, S2Cursor::kBackCel);
		emplaceHotspot(true, 178, 281, 453, 353).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
			auto &hotspot = static_cast<S2Hotspot &>(target);
			if (_game.getFlags().get(kUnlockedSuitcase) ||
				_game.getInventoryManager().getState(S2Inventory::kInv2) == S2InventoryState::InUse) {
				_game.getFlags().set(kUnlockedSuitcase);
				_game.getInventoryManager().unselectItem(false);
				_game.getSoundManager().play(11002, false, 100);
				removeChild(hotspot);
				setScript(this, &S2Room10000::suitcaseScript);
			} else {
				_game.getSoundManager().play(10004, false, 100);
			}
		});
		break;

	case 10129: {
		_game.getRoomManager().drawPic(10120);
		_lastRoomNo = _game.getRoomManager().getPreviousRoomNo();
		if (_lastRoomNo >= 10126 && _lastRoomNo <= 10128) {
			_game.getSoundManager().play(11532);
		}
		if (!_cel) {
			int16 loopNo;
			if (_game.getInventoryManager().getState(S2Inventory::kInv5) == S2InventoryState::Placed) {
				loopNo = 1;
			} else {
				loopNo = 0;
			}
			_cel.reset(new GLCel(getPlane(), 10120, loopNo, 3, roomBottom));
		}
		_cel->show();
		emplaceExit(true, 10999, 64, 0, 144, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10999, 495, 0, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10999, 145, 0, 494, 80, S2Cursor::kBackCel);
		emplaceExit(true, 10121, 330, 146, 395, 185, S2Cursor::kHighlightCel);
		emplaceExit(true, 10122, 244, 296, 275, 302, S2Cursor::kHighlightCel);
		emplaceExit(true, 10123, 333, 205, 375, 246, S2Cursor::kHighlightCel);
		emplaceExit(true, 10124, 259, 228, 296, 248, S2Cursor::kHighlightCel);
		emplaceExit(true, 10125, 231, 145, 282, 213, S2Cursor::kHighlightCel);
		emplaceExit(true, 10126, 306, 289, 381, 310, S2Cursor::kHighlightCel);
		emplaceHotspot(true, 180, 127, 413, 139).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			setScript(this, &S2Room10000::suitcaseScript);
		});

		auto &hotspot = emplaceHotspot(true, 365, 272, 408, 292);
		if (_game.getInventoryManager().getState(S2Inventory::kInv5) == S2InventoryState::Placed) {
			hotspot.setMouseUpHandler(this, &S2Room10000::takeTapeFromSuitcase);
		} else {
			hotspot.setMouseUpHandler(this, &S2Room10000::putTapeInSuitcase);
		}
		break;
	}

	case 10121:
	case 10122:
	case 10123:
	case 10124:
	case 10125:
		_game.getRoomManager().drawPic(roomNo);
		_game.getSoundManager().play(11505);
		emplaceExit(true, 10129, S2Cursor::kBackCel);

		switch (roomNo) {
		case 10121:
			_game.getFlags().set(kGameFlag49);
			_game.getScoringManager().doEvent(S2Score::kScore70);
			break;
		case 10122:
			_game.getScoringManager().doEvent(S2Score::kScore76);
			break;
		case 10123:
			_game.getScoringManager().doEvent(S2Score::kScore75);
			break;
		case 10124:
			_game.getScoringManager().doEvent(S2Score::kScore71);
			break;
		case 10125:
			_game.getScoringManager().doEvent(S2Score::kScore74);
			break;
		}
		break;

	case 10126:
	case 10127:
	case 10128: {
		int nextPageNo = roomNo + 1;
		if (nextPageNo == 10129) {
			nextPageNo = 10126;
		}
		_game.getRoomManager().drawPic(roomNo);
		_game.getSoundManager().play(11505);

		emplaceExit(true, 10129, 64, 0, 144, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10129, 495, 0, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, nextPageNo, 156, 32, 537, 343, S2Cursor::kHighlightCel);

		if (roomNo == 10126) {
			_game.getScoringManager().doEvent(S2Score::kScore72);
		} else if (roomNo == 10128) {
			_game.getFlags().set(kGameFlag48);
			_game.getScoringManager().doEvent(S2Score::kScore73);
		}

		break;
	}

	case 10200:
		_game.getRoomManager().drawPan(10200);
		_game.getRoomManager().getPanorama().addExit(10100, 800, 50, 1150, 500);
		break;

	case 10300: {
		_game.getRoomManager().drawPan(10300);
		auto &panorama = _game.getRoomManager().getPanorama();
		panorama.addExit(10100, 440, 200, 555, 387);
		panorama.addExit(10400, 0, 135, 135, 385);
		panorama.addExit(10400, 1900, 135, 2047, 385);
		panorama.addExit(10310, 774, 358, 998, 495, S2Cursor::kHighlightCel);
		panorama.addExit(10320, 1030, 214, 1160, 347, S2Cursor::kHighlightCel);
		panorama.addExit(10120, 567, 285, 723, 364, S2Cursor::kHighlightCel);
		break;
	}

	case 10310: {
		_game.getRoomManager().drawPic(10310);
		emplaceExit(true, 10999, 64, 0, 144, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10999, 495, 0, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10999, 145, 0, 494, 80, S2Cursor::kBackCel);
		addDresserHotspots();
		break;
	}

	case 10320:
		_game.getRoomManager().drawPic(10320);
		_game.getSoundManager().createAmbient(10);
		emplaceExit(true, 10300, 64, 0, 144, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10300, 495, 0, 575, 383, S2Cursor::kBackCel);
		emplaceHotspot(true, 253, 41, 375, 61).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			if (_game.getInventoryManager().getState(S2Inventory::kInv5) == S2InventoryState::InUse) {
				_game.getSoundManager().play(11008, false, 120);
				_game.getInventoryManager().setState(S2Inventory::kInv5, S2InventoryState::Used);
				_videoNo = 1;
				_game.getRoomManager().setNextRoomNo(10321);
			} else if (_game.getInventoryManager().getState(S2Inventory::kInv28) == S2InventoryState::InUse) {
				_game.getInventoryManager().setState(S2Inventory::kInv28, S2InventoryState::Used);
				_videoNo = 2;
				_game.getRoomManager().setNextRoomNo(10321);
			}
		});
		break;

	case 10321:
		_game.getRoomManager().drawPic(2);
		_game.getSoundManager().deleteAmbient(10);
		setScript([&](GLScript &script, const int state) {
			switch (state) {
			case 0:
				_game.getFlags().set(kGameFlag139);
				script.setCycles(1);
				break;
			case 1:
				_game.getMovieManager().play(_videoNo, _game.getFlags().get(kGameFlag139));
				// SSCI did not clean up the video number
				_videoNo = 0;
				_script.reset();
				break;
			}
		});
		break;

	case 10400: {
		_game.getRoomManager().drawPan(10400);
		_game.getSoundManager().createAmbient(10);

		_game.getFlags().set(kGameFlag91);
		_game.getFlags().set(kGameFlag71);

		if (_game.getRoomManager().getPreviousRoomNo() == 1020) {
			_game.getRoomManager().getPanorama().setPanX(717);
			_game.getInterface().resetButtons();
			// TODO: Save to the first new save slot instead of the autosave
			// slot?
			_game.save(0, false);
		}

		auto &panorama = _game.getRoomManager().getPanorama();
		panorama.addExit(10100, 725, 200, 786, 386);
		panorama.addExit(10300, 900, 180, 1171, 386);
		panorama.addExit(10410, 124, 278, 166, 301, S2Cursor::kHighlightCel);
		panorama.addExit(10420, 1859, 281, 1886, 297, S2Cursor::kHighlightCel);
		break;
	}

	case 10410:
		_game.getRoomManager().drawPic(10410);
		emplaceExit(true, 10400, 64, 0, 144, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10400, 495, 0, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10400, 145, 0, 494, 80, S2Cursor::kBackCel);
		emplaceExit(true, 10411, 181, 84, 511, 176, S2Cursor::kHighlightCel);
		_game.getPhoneManager().addAnsweringMachineLight(10411);
		break;

	case 10411:
		_game.getRoomManager().drawPic(10410);
		_cel.reset(new GLCel(getPlane(), 10410, 0, 0, roomBottom));
		_cel->show();
		emplaceExit(true, 10400, 213, 0, 575, 80, S2Cursor::kBackCel);
		emplaceExit(true, 10400, 480, 81, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10400, 176, 308, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10410, 105, 0, 204, 85, S2Cursor::kHighlightCel);
		emplaceExit(true, 10410, 126, 86, 186, 229, S2Cursor::kHighlightCel);
		emplaceExit(true, 10410, 64, 230, 174, 339, S2Cursor::kHighlightCel);

		emplaceHotspot(true, 224, 255, 257, 291).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			_game.getScoringManager().doEvent(kScore11);
			_game.getScoringManager().doEvent(kScore12);
			_game.getScoringManager().doEvent(kScore25);
			_game.getScoringManager().doEvent(kScore26);
			_game.getPhoneManager().pushedMotelMessageButton();
		});

		setUpPhone(10411);
		_game.getPhoneManager().addAnsweringMachineLight(10411);
		break;

	case 10420:
		_game.getRoomManager().drawPic(12420);
		emplaceExit(true, 10400, S2Cursor::kBackCel);
		_cel.reset(new GLCel(getPlane(), 12420, 0, 0, roomBottom));
		_cel->show();
		getPlane().getCast().remove(*_cel);

		// SSCI used a single handler with a switch to pick the right thing to
		// do, and also used a script indirection and took the user hands off
		// to play the radio for some unknown reason; we do not do either of
		// these things
		emplaceHotspot(true, 257, 220, 282, 243).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			// case 0
			if (!_radioIsOn) {
				return;
			}
			_cel->setCel(1, true);
			_cel->show();
			if (_game.getFlags().get(kPlayed101_1)) {
				playRadio(41004);
			} else {
				playRadio(41001);
				_game.getFlags().set(kPlayed101_1);
			}
		});

		emplaceHotspot(true, 307, 222, 332, 247).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			// case 1
			if (!_radioIsOn) {
				return;
			}
			_cel->setCel(2, true);
			_cel->show();
			if (_game.getFlags().get(kPlayed103_2)) {
				playRadio(41004);
			} else {
				playRadio(41002);
				_game.getFlags().set(kPlayed103_2);
			}
		});

		emplaceHotspot(true, 358, 225, 381, 249).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			// case 2
			if (!_radioIsOn) {
				return;
			}
			_cel->setCel(3, true);
			_cel->show();
			playRadio(41004);
		});

		emplaceHotspot(true, 411, 227, 433, 250).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			// case 3
			if (!_radioIsOn) {
				return;
			}
			_cel->setCel(4, true);
			_cel->show();
			if (_game.getFlags().get(kPlayed106_6)) {
				playRadio(41004);
			} else {
				playRadio(41003);
				_game.getFlags().set(kPlayed106_6);
			}
		});

		emplaceHotspot(true, 162, 210, 207, 252).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			_radioIsOn = !_radioIsOn;
			_game.getSoundManager().play(10609);
			_game.getSoundManager().stop(_radioSoundNo);
			_radioSoundNo = 0;
			_game.getInterface().putText(0);
			if (_radioIsOn) {
				_game.getScoringManager().doEvent(kScore60);
				_cel->hide();
				playRadio(41004);
			} else {
				_cel->setCel(0, true);
				_cel->show();
			}
		});
		break;

	case 10999:
		_lastRoomNo = _game.getRoomManager().getPreviousRoomNo();
		switch (_lastRoomNo) {
		case 10110:
			setScript(this, &S2Room10000::cancelScript);
			break;

		case 10120:
		case 10129:
			if (_suitcaseIsOpen) {
				_suitcaseIsOpen = false;
				_cancelSoundNo = 11010;
				setScript(this, &S2Room10000::cancelScript);
			} else {
				_game.getRoomManager().newRoom(10300);
			}
			break;

		case 10310:
			if (_topDrawerIsOpen || _bottomDrawerIsOpen) {
				_topDrawerIsOpen = _bottomDrawerIsOpen = false;
				_cancelSoundNo = 11005;
				setScript(this, &S2Room10000::cancelScript);
			} else {
				_game.getRoomManager().newRoom(10300);
			}
			break;
		}
		break;

	default:
		error("Unimplemented room %d", roomNo);
	}
}

void S2Room10000::dispose(const int roomNo) {
	switch (roomNo) {
	case 10121:
	case 10122:
	case 10123:
	case 10124:
	case 10125:
		_game.getSoundManager().play(11504);
		break;

	case 10410:
		_game.getPhoneManager().removeAnsweringMachineLight();
		break;

	case 10411:
		_game.getPhoneManager().resetPhone();
		_game.getPhoneManager().removeAnsweringMachineLight();
		break;

	case 10420:
		if (_radioIsOn) {
			_game.getSoundManager().play(10609);
			_game.getSoundManager().stop(_radioSoundNo);
			_game.getInterface().putText(0);
			_radioIsOn = false;
		}
		break;
	}

	if (roomNo == 10999 ||
		(_game.getRoomManager().getNextRoomNo() != 10999 &&
		 _game.getRoomManager().getNextRoomNo() != 10120)) {
		_cel.reset();
	}

	_cycler.reset();

	S2Room::dispose(roomNo);
}

bool S2Room10000::handleEvent(GLEvent &event) {
	if (event.getType() == kSciEventKeyDown &&
		_game.getRoomManager().getCurrentRoomNo() == 10411) {

		PhoneRoom::handleEvent(event);
	}

	return false;
}

#pragma mark -
#pragma mark Cancel action

void S2Room10000::cancelScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_game.getUser().setIsHandsOn(true);
		_game.getSoundManager().play(_cancelSoundNo, false, 120);
		// SSCI did not clean up the sound number
		_cancelSoundNo = 0;
		_cycler.reset(new GLEndBackCycler());
		_cycler->add(*_cel, false);
		_cycler->start(script);
		break;

	case 1:
		_cel.reset();
		script.setTicks(10);
		break;

	case 2:
		_script.reset();
		_cycler.reset();

		switch (_lastRoomNo) {
		case 10110:
			_game.getRoomManager().setNextRoomNo(10100);
			_game.getUser().setIsHandsOn(true);
			break;

		case 10120:
		case 10129:
		case 10310:
			_game.getRoomManager().setNextRoomNo(10300);
			_game.getUser().setIsHandsOn(true);
			break;
		}
		break;
	}
}

#pragma mark -
#pragma mark Suitcase

void S2Room10000::suitcaseScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_game.getUser().setIsHandsOn(false);
		if (_suitcaseIsOpen) {
			_cycler.reset(new GLEndBackCycler());
		} else {
			int16 loopNo;
			if (_game.getInventoryManager().getState(S2Inventory::kInv5) == S2InventoryState::Placed) {
				loopNo = 1;
			} else {
				loopNo = 0;
			}
			_cel.reset(new GLCel(getPlane(), 10120, loopNo, 0, roomBottom));
			_cel->show();
			_cycler.reset(new GLEndCycler());
		}
		_cycler->add(*_cel);
		_cycler->start(script);
		_game.getSoundManager().play(11001, false, 100);
		break;

	case 1:
		getPlane().getCast().remove(*_cel);
		if (_suitcaseIsOpen) {
			// SSCI also hid the cel here, but this is not necessary since it
			// is getting destroyed
			_cel.reset();
			_suitcaseIsOpen = false;
			_game.getRoomManager().setNextRoomNo(10120);
		} else {
			getPlane().getCast().remove(*_cel);
			_suitcaseIsOpen = true;
			_game.getRoomManager().setNextRoomNo(10129);
		}
		_script.reset();
		_cycler.reset();
		_game.getUser().setIsHandsOn(true);
		break;
	}
}

void S2Room10000::takeTapeFromSuitcase(GLEvent &, GLTarget &target) {
	if (_game.getInventoryManager().setState(S2Inventory::kInv5, S2InventoryState::Taken)) {
		_cel->setLoop(0, true);
		getPlane().getCast().remove(*_cel);
		_game.getInventoryManager().addItem(S2Inventory::kInv5);
		_game.getSoundManager().play(11009, false, 120);
		// SSCI deleted and recreated the same hotspot just to
		// change its event handler; we just change the handler
		static_cast<S2Hotspot &>(target).setMouseUpHandler(this, &S2Room10000::putTapeInSuitcase);
	}
}

void S2Room10000::putTapeInSuitcase(GLEvent &, GLTarget &target) {
	if (_game.getInventoryManager().getState(S2Inventory::kInv5) == S2InventoryState::InUse) {
		_cel->setLoop(1, true);
		getPlane().getCast().remove(*_cel);
		_game.getInventoryManager().unselectItem(false);
		_game.getInventoryManager().setState(S2Inventory::kInv5, S2InventoryState::Placed);
		// SSCI did not restore the handler when the tape was replaced
		static_cast<S2Hotspot &>(target).setMouseUpHandler(this, &S2Room10000::takeTapeFromSuitcase);
	}
}

#pragma mark -
#pragma mark Dresser

void S2Room10000::openDrawer(GLEvent &, GLTarget &target) {
	auto &hotspot = static_cast<S2Hotspot &>(target);

	int initialState;
	if (&hotspot == _topDrawer) {
		initialState = 0;
	} else {
		initialState = 2;
	}

	if (_topDrawer) {
		_game.getRoomManager().removeHotspot(*_topDrawer);
		removeChild(*_topDrawer);
		_topDrawer = nullptr;
	}

	if (_bottomDrawer) {
		_game.getRoomManager().removeHotspot(*_bottomDrawer);
		removeChild(*_bottomDrawer);
		_bottomDrawer = nullptr;
	}

	setScript(this, &S2Room10000::drawerScript, initialState);
}

void S2Room10000::drawerScript(GLScript &script, const int state) {
	switch (state) {
	// TODO: This could really be split into two functions since the
	// case 0/1 is for drawer 1 and case 2/3 are drawer 2
	case 0:
		_game.getUser().setIsHandsOn(false);
		if (_topDrawerIsOpen) {
			_game.getSoundManager().play(11005, false, 80);
			_cycler.reset(new GLEndBackCycler());
		} else {
			_game.getSoundManager().play(11006, false, 80);
			_cel.reset(new GLCel(getPlane(), 10310, 0, 0, roomBottom));
			_cel->show();
			_cycler.reset(new GLEndForwardCycler());
		}
		_cycler->add(*_cel, false);
		_cycler->start(script);
		break;

	case 1:
		if (_topDrawerIsOpen) {
			// SSCI removed/hid the cel here, but this is not necessary since
			// it is being deleted so this extra code is omitted
			_cel.reset();
			addDresserHotspots();
			_topDrawerIsOpen = false;
		} else {
			getPlane().getCast().remove(*_cel);
			_topDrawerIsOpen = true;
			_topDrawer = &emplaceHotspot(true, 64, 273, 513, 354);
			_topDrawer->setMouseUpHandler(this, &S2Room10000::openDrawer);
		}
		_script.reset();
		_cycler.reset();
		_game.getUser().setIsHandsOn(true);
		break;

	case 2:
		_game.getUser().setIsHandsOn(false);
		if (_bottomDrawerIsOpen) {
			_cycler.reset(new GLEndBackCycler());
			_game.getSoundManager().play(11005, false, 80);
		} else {
			if (!_game.getFlags().get(kGameFlag111) && _game.getFlags().get(kGameFlag2)) {
				_cel.reset(new GLCel(getPlane(), 10310, 1, 0, roomBottom));
				_prayerStickHotspot = &emplaceHotspot(true, 382, 294, 444, 336);
			} else {
				_cel.reset(new GLCel(getPlane(), 10310, 2, 0, roomBottom));
			}

			_cel->show();
			_cycler.reset(new GLEndForwardCycler());
			_game.getSoundManager().play(11006, false, 80);
		}
		_cycler->add(*_cel, false);
		_cycler->start(script);
		break;

	case 3:
		if (_bottomDrawerIsOpen) {
			_cel.reset();
			addDresserHotspots();
			if (_prayerStickHotspot) {
				removeChild(*_prayerStickHotspot);
				_prayerStickHotspot = nullptr;
			}
			_bottomDrawerIsOpen = false;
		} else {
			getPlane().getCast().remove(*_cel);
			_bottomDrawerIsOpen = true;
			_bottomDrawer = &emplaceHotspot(true, 69, 357, 533, 383);
			_bottomDrawer->setMouseUpHandler(this, &S2Room10000::openDrawer);
		}

		_script.reset();
		_cycler.reset();
		_game.getUser().setIsHandsOn(true);
		break;
	}
}

void S2Room10000::addDresserHotspots() {
	_topDrawer = &emplaceHotspot(true, 107, 202, 405, 267);
	_topDrawer->setMouseUpHandler(this, &S2Room10000::openDrawer);
	_bottomDrawer = &emplaceHotspot(true, 155, 286, 376, 337);
	_bottomDrawer->setMouseUpHandler(this, &S2Room10000::openDrawer);
}

#pragma mark -
#pragma mark Radio

void S2Room10000::playRadio(const uint16 soundNo) {
	_game.getSoundManager().stop(_radioSoundNo);
	_radioSoundNo = soundNo;
	if (soundNo == 41004) {
		_game.getSoundManager().play(soundNo, true, 120);
	} else {
		_game.getSoundManager().play(soundNo);
	}
	_game.getInterface().putText(soundNo);
}

} // End of namespace Sci
