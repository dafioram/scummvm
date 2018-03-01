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
		sound().createAmbient(10);
		flags().set(kGameFlag91);

		if (room().getPreviousRoomNo() == 6312) {
			room().getPanorama().setPanX(1319);
			sound().play(11003, false);
		}

		room().drawPan(10100);

		auto &panorama = room().getPanorama();
		panorama.addExit(10200, 100, 150, 300, 450);
		panorama.addExit(10300, 1350, 200, 1600, 400);
		panorama.addExit(10400, 1600, 150, 1900, 450);
		panorama.addExit(10110, 413, 152, 589, 375, S2Cursor::kHighlightCel);
		break;
	}

	case 10110: {
		room().drawPic(10110);
		_cancelSoundNo = 11003;
		setScript([&](GLScript &script, const int state) {
			switch (state) {
			case 0:
				user().setIsHandsOn(false);
				_cel.reset(new GLCel(getPlane(), roomNo, 0, 0, roomBottom));
				_cycler.reset(new GLEndCycler());
				_cycler->add(*_cel);
				_cycler->start(script);
				sound().play(11004, false, 120);
				break;
			case 1:
				getPlane().getCast().remove(*_cel);
				_script.reset();
				_cycler.reset();
				user().setIsHandsOn(true);
				break;
			}
		});
		emplaceExit(true, 10999, S2Cursor::kBackCel);
		emplaceExit(true, 6310, 246, 90, 392, 354);
		break;
	}

	case 10120:
		room().drawPic(10120);
		exitBorder(10999);
		emplaceHotspot(true, 178, 281, 453, 353).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
			auto &hotspot = static_cast<S2Hotspot &>(target);
			if (flags().get(kUnlockedSuitcase) ||
				inventory().isInUse(S2Inventory::kInv2)) {
				flags().set(kUnlockedSuitcase);
				inventory().unselectItem(false);
				sound().play(11002, false, 100);
				removeChild(hotspot);
				setScript(this, &S2Room10000::suitcaseScript);
			} else {
				sound().play(10004, false, 100);
			}
		});
		break;

	case 10129: {
		room().drawPic(10120);
		_lastRoomNo = room().getPreviousRoomNo();
		if (_lastRoomNo >= 10126 && _lastRoomNo <= 10128) {
			sound().play(11532);
		}
		if (!_cel) {
			int16 loopNo;
			if (inventory().isPlaced(S2Inventory::kInv5)) {
				loopNo = 1;
			} else {
				loopNo = 0;
			}
			_cel.reset(new GLCel(getPlane(), 10120, loopNo, 3, roomBottom));
		}
		_cel->show();
		exitBorder(10999);
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
		if (inventory().isPlaced(S2Inventory::kInv5)) {
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
		room().drawPic(roomNo);
		sound().play(11505);
		emplaceExit(true, 10129, S2Cursor::kBackCel);

		switch (roomNo) {
		case 10121:
			flags().set(kGameFlag49);
			score().doEvent(S2Score::kScore70);
			break;
		case 10122:
			score().doEvent(S2Score::kScore76);
			break;
		case 10123:
			score().doEvent(S2Score::kScore75);
			break;
		case 10124:
			score().doEvent(S2Score::kScore71);
			break;
		case 10125:
			score().doEvent(S2Score::kScore74);
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
		room().drawPic(roomNo);
		sound().play(11505);

		exitBorder(10129, false);
		emplaceExit(true, nextPageNo, 156, 32, 537, 343, S2Cursor::kHighlightCel);

		if (roomNo == 10126) {
			score().doEvent(S2Score::kScore72);
		} else if (roomNo == 10128) {
			flags().set(kGameFlag48);
			score().doEvent(S2Score::kScore73);
		}

		break;
	}

	case 10200:
		room().drawPan(10200);
		room().getPanorama().addExit(10100, 800, 50, 1150, 500);
		break;

	case 10300: {
		room().drawPan(10300);
		auto &panorama = room().getPanorama();
		panorama.addExit(10100, 440, 200, 555, 387);
		panorama.addExit(10400, 0, 135, 135, 385);
		panorama.addExit(10400, 1900, 135, 2047, 385);
		panorama.addExit(10310, 774, 358, 998, 495, S2Cursor::kHighlightCel);
		panorama.addExit(10320, 1030, 214, 1160, 347, S2Cursor::kHighlightCel);
		panorama.addExit(10120, 567, 285, 723, 364, S2Cursor::kHighlightCel);
		break;
	}

	case 10310: {
		room().drawPic(10310);
		exitBorder(10999);
		addDresserHotspots();
		break;
	}

	case 10320:
		room().drawPic(10320);
		sound().createAmbient(10);
		exitBorder(10300, false);
		emplaceHotspot(true, 253, 41, 375, 61).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			if (inventory().isInUse(S2Inventory::kInv5)) {
				sound().play(11008, false, 120);
				inventory().setState(S2Inventory::kInv5, S2InventoryState::Used);
				_videoNo = 1;
				room().setNextRoomNo(10321);
			} else if (inventory().isInUse(S2Inventory::kInv28)) {
				inventory().setState(S2Inventory::kInv28, S2InventoryState::Used);
				_videoNo = 2;
				room().setNextRoomNo(10321);
			}
		});
		break;

	case 10321:
		room().drawPic(2);
		sound().deleteAmbient(10);
		setScript([&](GLScript &script, const int state) {
			switch (state) {
			case 0:
				flags().set(kGameFlag139);
				script.setCycles(1);
				break;
			case 1:
				movie().play(_videoNo, flags().get(kGameFlag139));
				// SSCI did not clean up the video number
				_videoNo = 0;
				_script.reset();
				break;
			}
		});
		break;

	case 10400: {
		room().drawPan(10400);
		sound().createAmbient(10);

		flags().set(kGameFlag91);
		flags().set(kGameFlag71);

		if (room().getPreviousRoomNo() == 1020) {
			room().getPanorama().setPanX(717);
			interface().resetButtons();
			// TODO: Save to the first new save slot instead of the autosave
			// slot?
			_game.save(0, false);
		}

		auto &panorama = room().getPanorama();
		panorama.addExit(10100, 725, 200, 786, 386);
		panorama.addExit(10300, 900, 180, 1171, 386);
		panorama.addExit(10410, 124, 278, 166, 301, S2Cursor::kHighlightCel);
		panorama.addExit(10420, 1859, 281, 1886, 297, S2Cursor::kHighlightCel);
		break;
	}

	case 10410:
		room().drawPic(10410);
		exitBorder(10400);
		emplaceExit(true, 10411, 181, 84, 511, 176, S2Cursor::kHighlightCel);
		phone().addAnsweringMachineLight(10411);
		break;

	case 10411:
		room().drawPic(10410);
		_cel.reset(new GLCel(getPlane(), 10410, 0, 0, roomBottom));
		_cel->show();
		emplaceExit(true, 10400, 213, 0, 575, 80, S2Cursor::kBackCel);
		emplaceExit(true, 10400, 480, 81, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10400, 176, 308, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 10410, 105, 0, 204, 85, S2Cursor::kHighlightCel);
		emplaceExit(true, 10410, 126, 86, 186, 229, S2Cursor::kHighlightCel);
		emplaceExit(true, 10410, 64, 230, 174, 339, S2Cursor::kHighlightCel);

		emplaceHotspot(true, 224, 255, 257, 291).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			score().doEvent(kScore11);
			score().doEvent(kScore12);
			score().doEvent(kScore25);
			score().doEvent(kScore26);
			phone().pushedMotelMessageButton();
		});

		setUpPhone(10411);
		phone().addAnsweringMachineLight(10411);
		break;

	case 10420:
		room().drawPic(12420);
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
			if (flags().get(kPlayed101_1)) {
				playRadio(41004);
			} else {
				playRadio(41001);
				flags().set(kPlayed101_1);
			}
		});

		emplaceHotspot(true, 307, 222, 332, 247).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			// case 1
			if (!_radioIsOn) {
				return;
			}
			_cel->setCel(2, true);
			_cel->show();
			if (flags().get(kPlayed103_2)) {
				playRadio(41004);
			} else {
				playRadio(41002);
				flags().set(kPlayed103_2);
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
			if (flags().get(kPlayed106_6)) {
				playRadio(41004);
			} else {
				playRadio(41003);
				flags().set(kPlayed106_6);
			}
		});

		emplaceHotspot(true, 162, 210, 207, 252).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			_radioIsOn = !_radioIsOn;
			sound().play(10609);
			sound().stop(_radioSoundNo);
			_radioSoundNo = 0;
			interface().putText(0);
			if (_radioIsOn) {
				score().doEvent(kScore60);
				_cel->hide();
				playRadio(41004);
			} else {
				_cel->setCel(0, true);
				_cel->show();
			}
		});
		break;

	case 10999:
		_lastRoomNo = room().getPreviousRoomNo();
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
				room().newRoom(10300);
			}
			break;

		case 10310:
			if (_topDrawerIsOpen || _bottomDrawerIsOpen) {
				_topDrawerIsOpen = _bottomDrawerIsOpen = false;
				_cancelSoundNo = 11005;
				setScript(this, &S2Room10000::cancelScript);
			} else {
				room().newRoom(10300);
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
		sound().play(11504);
		break;

	case 10410:
		phone().removeAnsweringMachineLight();
		break;

	case 10411:
		phone().resetPhone();
		phone().removeAnsweringMachineLight();
		break;

	case 10420:
		if (_radioIsOn) {
			sound().play(10609);
			sound().stop(_radioSoundNo);
			interface().putText(0);
			_radioIsOn = false;
		}
		break;
	}

	if (roomNo == 10999 ||
		(room().getNextRoomNo() != 10999 &&
		 room().getNextRoomNo() != 10120)) {
		_cel.reset();
	}

	_cycler.reset();

	S2Room::dispose(roomNo);
}

bool S2Room10000::handleEvent(GLEvent &event) {
	if (room().getCurrentRoomNo() == 10411) {
		S2PhoneRoom::handleEvent(event);
	}

	return false;
}

#pragma mark -
#pragma mark Cancel action

void S2Room10000::cancelScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(true);
		sound().play(_cancelSoundNo, false, 120);
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
			room().setNextRoomNo(10100);
			user().setIsHandsOn(true);
			break;

		case 10120:
		case 10129:
		case 10310:
			room().setNextRoomNo(10300);
			user().setIsHandsOn(true);
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
		user().setIsHandsOn(false);
		if (_suitcaseIsOpen) {
			_cycler.reset(new GLEndBackCycler());
		} else {
			int16 loopNo;
			if (inventory().isPlaced(S2Inventory::kInv5)) {
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
		sound().play(11001, false, 100);
		break;

	case 1:
		getPlane().getCast().remove(*_cel);
		if (_suitcaseIsOpen) {
			// SSCI also hid the cel here, but this is not necessary since it
			// is getting destroyed
			_cel.reset();
			_suitcaseIsOpen = false;
			room().setNextRoomNo(10120);
		} else {
			getPlane().getCast().remove(*_cel);
			_suitcaseIsOpen = true;
			room().setNextRoomNo(10129);
		}
		_script.reset();
		_cycler.reset();
		user().setIsHandsOn(true);
		break;
	}
}

void S2Room10000::takeTapeFromSuitcase(GLEvent &, GLTarget &target) {
	if (inventory().setState(S2Inventory::kInv5, S2InventoryState::Taken)) {
		_cel->setLoop(0, true);
		getPlane().getCast().remove(*_cel);
		inventory().addItem(S2Inventory::kInv5);
		sound().play(11009, false, 120);
		// SSCI deleted and recreated the same hotspot just to
		// change its event handler; we just change the handler
		static_cast<S2Hotspot &>(target).setMouseUpHandler(this, &S2Room10000::putTapeInSuitcase);
	}
}

void S2Room10000::putTapeInSuitcase(GLEvent &, GLTarget &target) {
	if (inventory().isInUse(S2Inventory::kInv5)) {
		_cel->setLoop(1, true);
		getPlane().getCast().remove(*_cel);
		inventory().unselectItem(false);
		inventory().setState(S2Inventory::kInv5, S2InventoryState::Placed);
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
		room().removeHotspot(*_topDrawer);
		removeChild(*_topDrawer);
		_topDrawer = nullptr;
	}

	if (_bottomDrawer) {
		room().removeHotspot(*_bottomDrawer);
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
		user().setIsHandsOn(false);
		if (_topDrawerIsOpen) {
			sound().play(11005, false, 80);
			_cycler.reset(new GLEndBackCycler());
		} else {
			sound().play(11006, false, 80);
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
		user().setIsHandsOn(true);
		break;

	case 2:
		user().setIsHandsOn(false);
		if (_bottomDrawerIsOpen) {
			_cycler.reset(new GLEndBackCycler());
			sound().play(11005, false, 80);
		} else {
			if (!flags().get(kGameFlag111) && flags().get(kGameFlag2)) {
				_cel.reset(new GLCel(getPlane(), 10310, 1, 0, roomBottom));
				_prayerStickHotspot = &emplaceHotspot(true, 382, 294, 444, 336);
			} else {
				_cel.reset(new GLCel(getPlane(), 10310, 2, 0, roomBottom));
			}

			_cel->show();
			_cycler.reset(new GLEndForwardCycler());
			sound().play(11006, false, 80);
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
		user().setIsHandsOn(true);
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
	sound().stop(_radioSoundNo);
	_radioSoundNo = soundNo;
	if (soundNo == 41004) {
		sound().play(soundNo, true, 120);
	} else {
		sound().play(soundNo);
	}
	interface().putText(soundNo);
}

} // End of namespace Sci
