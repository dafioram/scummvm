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
			_coinReturn = &emplaceHotspot(true, 441, 204, 477, 249);
			_coinReturn->setMouseUpHandler(self(useCoinReturn));

			if (hasCoinInReturn()) {
				emplaceCel(false, 11111, 1, 0, roomBottom);
			} else {
				_coinReturn->disable();
			}
		}
	}

private:
	void vend(const int button) {
		if (_secondSelection) {
			removeChild(*_firstSelection);
			removeChild(*_secondSelection);
			if (_amount) {
				removeChild(*_amount);
			}
			_batteriesSelected = _buttonCOn = _button2On = false;
			_selection = 0;
			_firstSelection = _secondSelection = _amount = nullptr;
		}

		GLCel *&target = !_firstSelection ? _firstSelection : _secondSelection;

		sound().play(11103, false, 80);
		target = &emplaceCel(false, 11110, button, 0, roomBottom);
		target->show();
		if (button == 2) {
			_buttonCOn = true;
		}
		if (button == 5) {
			_button2On = true;
		}
		if (target == _firstSelection) {
			_selection += button * 10;
		} else {
			_selection += button;

			if (_button2On && _buttonCOn) {
				_batteriesSelected = true;
			}

			// SSCI did not do this, this just avoids needing twice the cases
			if (_selection / 10 > _selection % 10) {
				_selection = (_selection % 10) * 10 + _selection / 10;
			}

			int16 loopNo = 0;
			switch (_selection) {
			case 4:
			case 28:
				loopNo = 3;
				break;
			case 6: loopNo = 17; break;
			case 8: loopNo = 7; break;
			case 15: loopNo = 15; break;
			case 16: loopNo = 14; break;
			case 18: loopNo = 12; break;
			case 24: loopNo = 11; break;
			case 25: loopNo = 10; break;
			case 35:
			case 37:
				loopNo = 4;
				break;
			case 38: loopNo = 13; break;
			}

			if (loopNo) {
				_amount = &emplaceCel(false, 11112, loopNo, 0, roomBottom);
				_amount->show();
			}
		}
	}

	void useCoinSlot(GLEvent &, GLTarget &) {
		const auto item = inventory().getCurrentItem();
		if (item != S2Inventory::Change2_50 &&
			item != S2Inventory::Change2_75 &&
			item != S2Inventory::Quarter) {
			return;
		}

		inventory().unselectItem(false);
		switch (item) {
		case S2Inventory::Quarter:
			flags().set(kGameFlag146);
			sound().play(11408, false, 80);
			break;

		case S2Inventory::Change2_50:
			flags().set(kGameFlag147);
			for (auto i = 1; i <= 3; ++i) {
				sound().play(11408, false, 80, false, nullptr, i);
			}
			break;

		case S2Inventory::Change2_75:
			flags().set(kGameFlag148);
			for (auto i = 1; i <= 5; ++i) {
				sound().play(11408, false, 80, false, nullptr, i);
			}
			break;

		default:
			break;
		}

		if (item == S2Inventory::Change2_75 && _batteriesSelected) {
			inventory().setState(S2Inventory::Change2_75, S2InventoryState::Used);
			flags().clear(kGameFlag148);
			setScript(self(dispenseBatteries));
		} else {
			if (!_coin) {
				_coin = &emplaceCel(false, 11111, 1, 0, roomBottom);
				_coin->show();
				getPlane().getCast().removeEventHandler(*_coin);
			}
			_coinReturn->enable();
		}
	}

	void dispenseBatteries(GLScript &script, const int state) {
		switch (state) {
		case 0:
			script.setSeconds(2);
			break;

		case 1: {
			user().setIsHandsOn(false);
			inventory().setState(S2Inventory::Batteries, S2InventoryState::Placed);
			auto &cel = emplaceCel(false, 11111, 0, 0, roomBottom);
			cel.show();
			_cycler.reset(new GLEndCycler(cel, script));
			break;
		}

		case 2:
			script.setSeconds(2);
			break;

		case 3:
			_cycler.reset();
			_script.reset();
			user().setIsHandsOn(true);
			room().setNextRoomNo(11150);
			break;
		}
	}

	void useCoinReturn(GLEvent &, GLTarget &) {
		if (cursor().hasInventory()) {
			return;
		}

		if (flags().get(kGameFlag147)) {
			getCoins(kGameFlag147, S2Inventory::Change2_50);
		} else if (flags().get(kGameFlag148)) {
			getCoins(kGameFlag148, S2Inventory::Change2_75);
		} else if (flags().get(kGameFlag146)) {
			getCoins(kGameFlag146, S2Inventory::Quarter);
		}
	}

	void getCoins(const GameFlag flag, const S2Inventory item) {
		flags().clear(flag);
		sound().play(11106, false, 80);
		cursor().endHighlight();
		cursor().getItem(inventory().getSmallCel(item));
		inventory().setCurrentItem(item);

		if (!flags().get(kGameFlag146) &&
			!flags().get(kGameFlag147) &&
			!flags().get(kGameFlag148)) {

			removeChild(*_coin);
			_coin = nullptr;
			_coinReturn->disable();
		}
	}

	bool hasCoinInReturn() {
		return (flags().get(kGameFlag146) ||
				flags().get(kGameFlag147) ||
				flags().get(kGameFlag148));
	}

	bool _batteriesSelected = false;
	bool _buttonCOn = false;
	bool _button2On = false;
	int _selection = 0;
	GLCel *_firstSelection = nullptr;
	GLCel *_secondSelection = nullptr;
	GLCel *_amount = nullptr;
	GLCel *_coin = nullptr;
	S2Hotspot *_coinReturn = nullptr;
};

#undef self
#define self(name) this, &S2MotelDesk::name

class S2MotelDesk : public S2SubRoom {
public:
	using S2SubRoom::S2SubRoom;

	virtual void init(const int roomNo) override {
		_parent._exitSoundNo = 11100;
		room().drawPic(11350);
		emplaceExit(true, 11999, 64, 0, 575, 80, S2Cursor::kBackCel);
		_topLeft = &emplaceHotspot(true, 94, 164, 307, 249);
		_topLeft->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			disableDrawers();
			_leftDrawerIndex = 1;
			setScript(self(leftDrawer));
		});
		_bottomLeft = &emplaceHotspot(true, 141, 260, 308, 310);
		_bottomLeft->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			disableDrawers();
			_leftDrawerIndex = 2;
			setScript(self(leftDrawer));
		});
		_topRight = &emplaceHotspot(true, 337, 163, 551, 250);
		_topRight->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			disableDrawers();
			_rightDrawerIndex = 1;
			score().doEvent(kScore77);
			setScript(self(rightDrawer));
		});
		_bottomRight = &emplaceHotspot(true, 334, 257, 494, 308);
		_bottomRight->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			disableDrawers();
			_rightDrawerIndex = 2;
			setScript(self(rightDrawer));
		});

		_bible = &emplaceHotspot(true, 170, 231, 264, 337);
		_bible->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			_bible->disable();
			score().doEvent(kScore78);
			sound().play(11108, false, 80);
			room().setNextRoomNo(11351);
		});
		_bible->disable();

		_closeLeft = &emplaceHotspot(true, 144, 310, 304, 383);
		_closeLeft->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			setScript(self(leftDrawer));
		});
		_closeLeft->disable();
		_closeTopRight = &emplaceHotspot(true, 339, 285, 444, 350);
		_closeTopRight->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			setScript(self(rightDrawer));
		});
		_closeTopRight->disable();
		_closeBottomRight = &emplaceHotspot(true, 339, 347, 507, 371);
		_closeBottomRight->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			setScript(self(rightDrawer));
		});
		_closeBottomRight->disable();

		_key = &emplaceHotspot(true, 376, 309, 429, 334);
		_key->setMouseUpHandler([&](GLEvent &, GLTarget &target) {
			if (inventory().setState(S2Inventory::kInv6, S2InventoryState::Taken)) {
				_key->disable();
				sound().play(12107, false, 100);
				inventory().addItem(S2Inventory::kInv6);
				_parent._cel->setLoop(4);
				_parent._cel->setCel(3, true);
			}
		});
		_key->disable();

		if (roomNo == 11355) {
			_parent._cel.reset(new GLCel(getPlane(), 11350, 0, 3, { 64, 384 }));
			_parent._cel->show();
			disableDrawers();
			_bible->enable();
			_closeLeft->enable();
		}
	}

private:
	S2Hotspot *_topLeft;
	S2Hotspot *_topRight;
	S2Hotspot *_bottomLeft;
	S2Hotspot *_bottomRight;
	S2Hotspot *_bible;
	S2Hotspot *_closeLeft;
	S2Hotspot *_closeTopRight;
	S2Hotspot *_closeBottomRight;
	S2Hotspot *_key;

	int _leftDrawerIndex;
	int _rightDrawerIndex;

	void enableDrawers() {
		_topLeft->enable();
		_topRight->enable();
		_bottomLeft->enable();
		_bottomRight->enable();
	}

	void disableDrawers() {
		_topLeft->disable();
		_topRight->disable();
		_bottomLeft->disable();
		_bottomRight->disable();
	}

	void leftDrawer(GLScript &script, const int state) {
		switch (state) {
		case 0:
			user().setIsHandsOn(false);
			if (_parent._cel) {
				_leftDrawerIndex = 0;
				_cycler.reset(new GLEndBackCycler(*_parent._cel, script));
				sound().play(11100, false, 80);
				_closeLeft->disable();
				_bible->disable();
			} else if (_leftDrawerIndex) {
				int16 loopNo = _leftDrawerIndex == 1 ? 0 : 2;
				_parent._cel.reset(new GLCel(getPlane(), 11350, loopNo, 0, { 64, 384 }));
				_parent._cel->show();
				_cycler.reset(new GLEndCycler(*_parent._cel, script));
				sound().play(11101, false, 80);
			}
			break;

		case 1:
			getPlane().getCast().removeEventHandler(*_parent._cel);
			if (!_leftDrawerIndex) {
				_parent._cel.reset();
				enableDrawers();
				user().setIsHandsOn(true);
			} else if (_leftDrawerIndex == 1) {
				_bible->enable();
				_closeLeft->enable();
			} else {
				_closeLeft->enable();
			}

			_cycler.reset();
			_script.reset();
			user().setIsHandsOn(true);
			break;
		}
	}

	void rightDrawer(GLScript &script, const int state) {
		switch (state) {
		case 0:
			user().setIsHandsOn(false);
			if (_parent._cel) {
				_rightDrawerIndex = 0;
				_cycler.reset(new GLEndBackCycler(*_parent._cel, script));
				sound().play(11100, false, 80);
				_closeTopRight->disable();
				_closeBottomRight->disable();
				_key->disable();
			} else if (_rightDrawerIndex) {
				int16 loopNo;
				if (_rightDrawerIndex == 2) {
					loopNo = inventory().isPlaced(S2Inventory::kInv6) ? 3 : 4;
				} else {
					loopNo = 1;
				}

				_parent._cel.reset(new GLCel(getPlane(), 11350, loopNo, 0, roomBottom));
				_cycler.reset(new GLEndCycler(*_parent._cel, script));
				sound().play(11101, false, 80);
			}
			break;

		case 1:
			getPlane().getCast().removeEventHandler(*_parent._cel);
			if (!_rightDrawerIndex) {
				_parent._cel.reset();
				enableDrawers();
			} else if (_rightDrawerIndex == 1) {
				_closeTopRight->enable();
			} else {
				if (inventory().isPlaced(S2Inventory::kInv6)) {
					_key->enable();

					// There was an extra hotspot here for some reason for
					// closing the drawer, it is omitted since the drawer can
					// be closed just fine without it
				}
				_closeBottomRight->enable();
			}

			_cycler.reset();
			_script.reset();
			user().setIsHandsOn(true);
			break;
		}
	}
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

		if (inventory().isPlaced(S2Inventory::Gum)) {
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

		if (inventory().isTaken(S2Inventory::Gum)) {
			auto &cel = emplaceCel(false, 11140, 2, 0, roomBottom);
			cel.show();
			getPlane().getCast().remove(cel);
		} else {
			emplaceHotspot(true, 420, 303, 465, 345).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
				if (inventory().setState(S2Inventory::Gum, S2InventoryState::Taken)) {
					sound().play(11115, false, 80);
					inventory().addItem(S2Inventory::Gum);
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
		if (inventory().isPlaced(S2Inventory::Gum)) {
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
				if (inventory().isInUse(S2Inventory::Gum)) {
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
		addPanoramaExit(11350, 362, 413, 625, 510, S2Cursor::kHighlightCel);
		addPanoramaExit(11332, 778, 331, 821, 369, S2Cursor::kHighlightCel);
		addPanoramaExit(11311, 1713, 279, 1739, 311, S2Cursor::kHighlightCel);

		if (inventory().isPlaced(S2Inventory::Gum)) {
			emplaceSprite(false, 11301, GLPoint(1172, 331));
			addPanoramaExit(11301, 1170, 333, 1241, 365, S2Cursor::kHighlightCel);
		}
		break;

	case 11301:
		if (inventory().setState(S2Inventory::Gum, S2InventoryState::Taken)) {
			sound().play(11115, false, 80);
			inventory().addItem(S2Inventory::Gum);
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
		setSubRoom<S2MotelDesk>(roomNo);
		break;

	case 11351:
		room().drawPic(11351);
		emplaceExit(true, 11355, S2Cursor::kBackCel);
		emplaceExit(true, 11352, 145, 26, 444, 349, S2Cursor::kHighlightCel);
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
		setSubRoom<S2MotelDesk>(roomNo);
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
	if (room().getNextRoomNo() != 11999) {
		_cel.reset();
	}

	_cycler.reset();

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

} // End of namespace Sci
