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

#include "sci/s2/rooms/11000.h"

namespace Sci {

#define self(name) this, &S2VendingMachine::name

class S2VendingMachine : public S2SubRoom {
public:
	using S2SubRoom::S2SubRoom;

	virtual void init(const int) override {
		room().drawPic(11110);
		emplaceExit(true, 11100, 64, 303, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 11100, 64, 0, 445, 80, S2Cursor::kBackCel);
		if (inventory().isTaken(S2Inventory::Batteries) ||
			inventory().isUsed(S2Inventory::Batteries)) {

			auto &cel = emplaceCel(false, 11111, 0, 0, roomBottom);
			cel.show();
			getPlane().getCast().remove(cel);
		} else {
			GLPoint p = { 419, 105 };
			for (auto i = 0; i < 9; ++i) {
				emplaceHotspot(true, p.x, p.y, p.x + 29, p.y + 24).setMouseUpHandler([this, i](GLEvent &, GLTarget &) {
					vend(i);
				});
				if (((i + 1) % 3) == 0) {
					p.x = 419;
					p.y += 27;
				} else {
					p.x += 30;
				}
			}
			emplaceHotspot(true, 466, 5, 496, 61).setMouseUpHandler(self(useCoinSlot));
			auto &coinReturn = emplaceHotspot(true, 441, 204, 477, 249);
			coinReturn.setMouseUpHandler(self(useCoinReturn));

			if (hasCoinInReturn()) {
				emplaceCel(false, 11111, 1, 0, roomBottom);
			} else {
				coinReturn.disable();
			}
		}
	}

private:
	void vend(const int button) {
		warning("TODO: %s", __PRETTY_FUNCTION__);
	}

	void useCoinSlot(GLEvent &, GLTarget &) {
		warning("TODO: %s", __PRETTY_FUNCTION__);
	}

	void useCoinReturn(GLEvent &, GLTarget &) {
		warning("TODO: %s", __PRETTY_FUNCTION__);
	}

	bool hasCoinInReturn() {
		return (flags().get(kGameFlag146) ||
				flags().get(kGameFlag147) ||
				flags().get(kGameFlag148));
	}

	bool _selected = false;
};

#undef self
#define self(name) this, &S2Room11000::name

void S2Room11000::init(const int roomNo) {
	switch (roomNo) {
	case 11100: {
		enterFrom(6311, 1846, 11112);
		room().drawPan(11100);
		sound().createAmbient(11);
		flags().set(kGameFlag91);

		if (inventory().isPlaced(S2Inventory::kInv7)) {
			emplaceSprite(false, 11101, GLPoint(1814, 245));
		}

		addPanoramaExit(11200, 110, 222, 240, 500);
		addPanoramaExit(11101, 860, 103, 1213, 511, S2Cursor::kHighlightCel);

		int exitNo;
		if (inventory().isPlaced(S2Inventory::Batteries)) {
			exitNo = 11150;
		} else {
			exitNo = 11110;
		}
		addPanoramaExit(exitNo, 470, 226, 600, 295, S2Cursor::kHighlightCel);
		addPanoramaExit(11120, 693, 259, 769, 442, S2Cursor::kHighlightCel);
		addPanoramaExit(11130, 1584, 210, 1649, 280, S2Cursor::kHighlightCel);
		addPanoramaExit(11140, 1732, 222, 1851, 267, S2Cursor::kHighlightCel);
		break;
	}

	case 11101:
		room().drawPic(11101);
		enter(11100, 11111, 11112, true);
		emplaceExit(true, 6310, 253, 51, 421, 383);
		break;

	case 11110:
		setSubRoom<S2VendingMachine>(roomNo);
		break;

	case 11120:
		room().drawPic(11120);
		exitBorder(11100);
		break;

	case 11121:
		room().drawPic(11121);
		emplaceExit(true, 11120, S2Cursor::kBackCel);
		score().doEvent(kScore82);
		break;

	case 11130:
		room().drawPic(11130);
		emplaceExit(true, 11100, S2Cursor::kBackCel);
		score().doEvent(kScore79);
		break;

	case 11140:
		room().drawPic(11140);
		phone().cancelCall();
		exitBorder(11141);

		emplaceHotspot(true, 196, 329, 247, 363).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
			sound().play(11102, false, 80);
			score().doEvent(kScore193);
			removeChild(static_cast<S2Hotspot &>(target));
			if (!_cel) {
				setScript(self(openJackBox));
				if (!flags().get(kGameFlag114)) {
					emplaceHotspot(true, 257, 81, 336, 218).setMouseUpHandler([&](GLEvent &, GLTarget &) {
						takePrayerStick();
					});
				}
			}
		});

		if (inventory().isTaken(S2Inventory::kInv7)) {
			auto &cel = emplaceCel(false, 11140, 2, 0, roomBottom);
			cel.show();
			getPlane().getCast().remove(cel);
		} else {
			emplaceHotspot(true, 420, 303, 465, 345).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
				if (inventory().setState(S2Inventory::kInv7, S2InventoryState::Taken)) {
					sound().play(11115, false, 80);
					inventory().addItem(S2Inventory::kInv7);
					removeChild(static_cast<S2Hotspot &>(target));
					auto &cel = emplaceCel(false, 11140, 2, 0, roomBottom);
					cel.show();
					getPlane().getCast().remove(cel);
				}
			});
		}

		if (flags().get(kGameFlag144)) {
			emplaceHotspot(true, 270, 221, 379, 359).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
				if (!flags().get(kGameFlag119)) {
					// Replacement of hotspot moved up from callee
					removeChild(static_cast<S2Hotspot &>(target));
					emplaceHotspot(true, 270, 221, 379, 359).setMouseUpHandler([&](GLEvent &, GLTarget &) {
						setScript(self(jackInBox));
					});

					setScript(self(openJackBox));
				}
			});
		}
		break;

	case 11141:
		if (flags().get(kGameFlag144)) {
			setScript(self(jackInBox));
		} else if (!flags().get(kGameFlag28) &&
				   inventory().hasPrayerStick(S2PrayerStick::Jack)) {

			room().setNextRoomNo(11199);
		} else {
			room().newRoom(11100);
		}
		break;

	case 11150:
		room().drawPic(11150);
		emplaceExit(true, 11100, S2Cursor::kBackCel);
		emplaceHotspot(true, 203, 136, 271, 183).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			if (inventory().setState(S2Inventory::Batteries, S2InventoryState::Taken)) {
				auto &cel = emplaceCel(false, 11150, 0, 0, roomBottom);
				cel.show();
				getPlane().getCast().remove(cel);
				sound().play(11107, false, 120);
				inventory().addItem(S2Inventory::Batteries);
			}
		});
		break;

	case 11199:
		interface().changeLife(-5);
		flags().set(kGameFlag28);
		movie().play(5100);
		room().newRoom(11100);
		break;

	case 11200:
		room().drawPan(11200);
		if (inventory().isPlaced(S2Inventory::kInv7)) {
			emplaceSprite(false, 11201, GLPoint(1349, 246));
		}
		addPanoramaExit(11100, 1084, 211, 1170, 360);
		addPanoramaExit(11300, 1473, 188, 1634, 452);
		addPanoramaExit(11210, 187, 227, 260, 278, S2Cursor::kHighlightCel);
		break;

	case 11210:
		room().drawPic(11210);
		sound().createAmbient(11);
		exitBorder(11200, false);
		if (flags().get(kGameFlag145)) {
			emplaceCel(false, 15342, 0, 0, GLPoint(259, 103), 202).show();
		} else {
			emplaceHotspot(true, 240, 90, 284, 123).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
				if (inventory().isInUse(S2Inventory::kInv7)) {
					flags().set(kGameFlag145);
					score().doEvent(kScore167);
					emplaceCel(false, 15342, 0, 0, GLPoint(259, 103), 202).show();
					removeChild(static_cast<S2Hotspot &>(target));
				}
			});
		}

		emplaceHotspot(true, 214, 282, 431, 319).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			room().setNextRoomNo(11211);
		});
		break;

	case 11211:
		room().drawPic(15341);
		sound().deleteAmbient(11);
		if (flags().get(kGameFlag145)) {
			emplaceCel(false, 15341, 0, 0, roomBottom, 202).show();
		}

		setScript([&](GLScript &script, const int state) {
			switch (state) {
			case 0:
				script.setTicks(1);
				break;

			case 1:
				movie().play(0, flags().get(kGameFlag145));
				break;
			}
		});

		break;

	case 11300:
		room().drawPan(11300);
		enterFrom(11311, -1, 12305);
		addPanoramaExit(11200, 186, 183, 320, 508);
		addPanoramaExit(11330, 666, 308, 755, 363, S2Cursor::kHighlightCel);
		addPanoramaExit(11341, 378, 316, 453, 372, S2Cursor::kHighlightCel);
		addPanoramaExit(11332, 778, 331, 821, 369, S2Cursor::kHighlightCel);
		addPanoramaExit(11311, 1713, 279, 1739, 311, S2Cursor::kHighlightCel);

		if (inventory().isPlaced(S2Inventory::kInv7)) {
			emplaceSprite(false, 11301, GLPoint(1172, 331));
			addPanoramaExit(11301, 1170, 333, 1241, 365, S2Cursor::kHighlightCel);
		}
		break;

	case 11301:
		if (inventory().setState(S2Inventory::kInv7, S2InventoryState::Taken)) {
			sound().play(11115, false, 80);
			inventory().addItem(S2Inventory::kInv7);
		} else {
			emplaceSprite(false, 11301, GLPoint(1171, 330));
		}
		room().newRoom(11300);
		break;

	case 11311:
		room().drawPic(11311);
		emplaceExit(true, 11300, S2Cursor::kBackCel);
		sound().play(12304, false, 100);
		score().doEvent(kScore80);
		break;

	case 11330:
		room().drawPic(11330);
		emplaceExit(true, 11300, S2Cursor::kBackCel);
		emplaceExit(true, 11331, 90, 70, 465, 150, S2Cursor::kHighlightCel);
		break;

	case 11331:
		room().drawPic(11330);
		emplaceCel(false, 11330, 0, 0, roomBottom).show();
		emplaceExit(true, 11300, 222, 0, 575, 80, S2Cursor::kBackCel);
		emplaceExit(true, 11300, 494, 80, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 11300, 232, 303, 492, 380, S2Cursor::kBackCel);
		emplaceExit(true, 11330, 94, 0, 212, 102, S2Cursor::kHighlightCel);
		emplaceExit(true, 11330, 129, 101, 186, 245, S2Cursor::kHighlightCel);
		emplaceExit(true, 11330, 64, 247, 223, 337, S2Cursor::kHighlightCel);
		setUpPhone(11331);
		break;

	case 11332:
		room().drawPic(11332);
		exitBorder(11300);
		setUpAnsweringMachine(11332);
		phone().addAnsweringMachineLight(11332);
		break;

	case 11341:
		room().drawPic(11341);
		sound().play(11113, false, 80);
		emplaceExit(true, 11300, S2Cursor::kBackCel);
		score().doEvent(kScore81);
		break;

	case 11350:
		room().drawPic(11350);
		_exitSoundNo = 11100;
		emplaceExit(true, 11999, 64, 0, 575, 80, S2Cursor::kBackCel);
		emplaceHotspot(true, 94, 164, 307, 249);
		warning("TODO: Finish room %d", roomNo);
		break;

	case 11351:
		room().drawPic(11351);
		emplaceExit(true, 11355, S2Cursor::kBackCel);
		emplaceExit(true, 11352, 145, 26, 444, S2Cursor::kHighlightCel);
		break;

	case 11352:
		room().drawPic(11352);
		if (room().getPreviousRoomNo() == 11353) {
			sound().play(11313, false, 100);
		} else {
			sound().play(11109, false, 80);
		}

		exitBorder(11355, false);
		emplaceExit(true, 11355, 145, 303, 494, 383, S2Cursor::kBackCel);
		emplaceExit(true, 11353, 378, 28, 548, 255, S2Cursor::kHighlightCel);
		break;

	case 11353:
		room().drawPic(11353);
		sound().play(11313, false, 100);
		emplaceExit(true, 11352, S2Cursor::kBackCel);
		break;

	case 11355: {
		room().drawPic(11350);
		_exitSoundNo = 11100;
		sound().play(11110, false, 80);
		emplaceExit(true, 11999, 64, 0, 575, 80, S2Cursor::kBackCel);

		// In SSCI this was off-by-one on the bottom axis
		auto &cel = emplaceCel(true, 11350, 0, 3, roomBottom);
		cel.show();
		getPlane().getCast().remove(cel);

		emplaceHotspot(true, 170, 231, 264, 337).setMouseUpHandler(self(openBible));
		emplaceHotspot(true, 64, 340, 304, 383).setMouseUpHandler(self(closeDrawer));
		break;
   }

	case 11999:
		setScript(self(cancelScript));
		break;

	default:
		warning("Unimplemented room %d", roomNo);
	}
}

void S2Room11000::dispose(const int roomNo) {
	switch (roomNo) {
	case 11140:
		interface().putText(0);
		break;
	case 11331:
		phone().resetPhone();
		break;
	case 11332:
		phone().resetAnsweringMachine();
		phone().removeAnsweringMachineLight();
		break;
	case 11341:
		sound().play(11114, false, 80);
		break;
	case 11350:
		warning("TODO: dispose %d", roomNo);
		break;
	}

	S2PhoneRoom::dispose(roomNo);
}

bool S2Room11000::handleEvent(GLEvent &event) {
	if (room().getCurrentRoomNo() == 11331) {
		S2PhoneRoom::handleEvent(event);
	}
	return false;
}

void S2Room11000::cancelScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		if (_cel) {
			_cycler.reset(new GLEndBackCycler());
			_cycler->add(*_cel);
			_cycler->start(script);
			sound().play(_exitSoundNo, false, 100);
		} else {
			script.setTicks(1);
		}
		break;

	case 1:
		if (_cel) {
			getPlane().getCast().remove(*_cel);
			_cel->hide();
		}
		script.setTicks(10);
		break;

	case 2:
		_cel.reset();
		_cycler.reset();
		_script.reset();

		switch (room().getPreviousRoomNo()) {
		case 11101:
			room().setNextRoomNo(11100);
			user().setIsHandsOn(true);
			break;

		case 11350:
		case 11355:
			room().setNextRoomNo(11300);
			user().setIsHandsOn(true);
			break;

		default:
			error("Stuck in room 11999");
		}
	}
}

void S2Room11000::openJackBox(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		// Conditional removal of hotspot moved up to caller
		if (!_cel) {
			const auto loopNo = flags().get(kGameFlag114) ? 1 : 0;
			_cel.reset(new GLCel(getPlane(), 11140, loopNo, 0, roomBottom));
			_cel->show();
		}
		_cycler.reset(new GLEndCycler());
		_cycler->add(*_cel);
		_cycler->start(script);
		if (!flags().get(kGameFlag144) && !flags().get(kGameFlag106)) {
			sound().play(41150);
			interface().putText(41150);
		} else {
			sound().play(11116, false, 120);
		}
		break;

	case 1:
		getPlane().getCast().remove(*_cel);
		if (!flags().get(kGameFlag114)) {
			//hotspot 3
			emplaceHotspot(true, 236, 117, 407, 183).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
				// Hotspot removal moved up from callee
				removeChild(static_cast<S2Hotspot &>(target));
				takePrayerStick();
			});
		}

		flags().set(kGameFlag45);
		flags().set(kGameFlag144);
		_script.reset();
		_cycler.reset();
		user().setIsHandsOn(true);
		break;
	}
}

void S2Room11000::jackInBox(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		if (!_cel) {
			const auto loopNo = flags().get(kGameFlag114) ? 1 : 0;
			_cel.reset(new GLCel(getPlane(), 11140, loopNo, 10, roomBottom));
		}
		_cel->show();
		_cycler.reset(new GLEndBackCycler());
		_cycler->add(*_cel);
		_cycler->start(script);
		sound().stop(41150);
		sound().play(11117, false, 120);
		break;

	case 1:
		_cel.reset();
		script.setSeconds(1);
		break;

	case 2:
		_script.reset();
		_cycler.reset();
		flags().clear(kGameFlag144);
		if (flags().get(kGameFlag106) && !flags().get(kGameFlag28)) {
			room().setNextRoomNo(11199);
		} else {
			room().setNextRoomNo(11100);
		}
		user().setIsHandsOn(true);
		break;
	}
}

void S2Room11000::takePrayerStick() {
	_cel->setLoop(1);
	_cel->setCel(10, true);
	_cel->show();
	inventory().takePrayerStick(S2PrayerStick::Jack);
	flags().set(kGameFlag106);
	flags().set(kGameFlag114);
}

void S2Room11000::openBible(GLEvent &, GLTarget &target) {
	removeChild(static_cast<S2Hotspot &>(target));
	score().doEvent(kScore78);
	sound().play(11108, false, 80);
	room().setNextRoomNo(11351);
}

void S2Room11000::closeDrawer(GLEvent &, GLTarget &) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

} // End of namespace Sci
