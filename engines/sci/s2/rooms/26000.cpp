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

#include "sci/s2/rooms/26000.h"

namespace Sci {

#define self(name) this, &S2FinalPuzzle::name

class S2FinalPuzzle : public S2SubRoom {
	constexpr GLPoint calcPoint(const int x, const int y) {
		return GLPoint(157 + 26 * x, 263 + 26 * y);
	}

public:
	using S2SubRoom::S2SubRoom;

	void init(const int) override {
		room().drawPic(5999);

		_outerRing = &emplaceCel(false, 5999, 0, 0, roomBottom, 500);
		_outerRing->setCycleSpeed(1);
		_outerRing->show();
		_innerRing = &emplaceCel(false, 5999, 1, 0, roomBottom, 500);
		_innerRing->setCycleSpeed(1);
		_innerRing->show();
		GLPoint position(157, 263);
		for (auto y = 0; y < _grid.size(); ++y) {
			auto &row = _grid[y];
			for (auto x = 0; x < row.size(); ++x) {
				if (row[x]) {
					_cels[y][x] = nullptr;
				} else {
					const auto state = _game.getRandomNumber(0, 1);
					_grid[y][x] = state;
					auto &cel = emplaceCel(false, 5999, 3, state * 2, position);
					cel.show();
					cel.setSelectHandler([this, x, y](GLEvent &event, GLTarget &target) {
						if (event.getType() == kSciEventMouseRelease) {
							toggleMirror(static_cast<GLCel &>(target), x, y);
						}
					});
					cel.forceUpdate();
					_cels[y][x] = &cel;
				}
				position.x += 26;
			}
			position.x = 157;
			position.y += 26;
		}

		for (auto i = 0; i < 3; ++i) {
			_crystals[i] = &emplaceChild<GLScreenItem>(5999, 5, i, roomBottom, 450);
		}

		_someLifeMultiple = interface().getLife() / 16;
		user().setIsHandsOn(true);
		setScript(self(start));
		// SSCI used an extra boolean to pass state into drawBeam even though
		// that state was only ever triggered from this caller, so the
		// consequent is moved up to here instead
		sound().play(12626, false, 100);
		drawBeam();
	}

private:
	void start(GLScript &script, const int state) {
		switch (state) {
		case 0:
			_cycler.reset(new GLEndCycler());
			_cycler->add(*_outerRing);
			_cycler->start(script);
			sound().play(12625, false, 100);
			break;

		case 1:
			_cycler.reset(new GLEndCycler());
			_cycler->add(*_innerRing);
			_cycler->start(script);
			sound().play(12627, true, 100);
			break;

		case 2:
			sound().stop(12627);
			_cycler.reset();
			getPlane().getCast().removeEventHandler(*_outerRing);
			getPlane().getCast().removeEventHandler(*_innerRing);
			_innerRing->setLoop(6);
			_innerRing->setPriority(400, true);
			user().setIsHandsOn(true);
			sound().play(12609);
			sound().play(_soundNo, false, Audio32::kMaxVolume, false, &script);
			interface().putText(_soundNo);
			break;

		case 3:
		case 5:
		case 6:
		case 7:
		case 9:
		case 10:
		case 11:
		case 13:
		case 14:
		case 15:
			sound().stop(12609);
			sound().play(12609);
			interface().changeLife(-_someLifeMultiple);
			script.setTicks(245 - state * 15);
			break;

		case 4:
		case 8:
		case 12:
		case 16:
			_soundNo += 2;
			sound().play(_soundNo, false, Audio32::kMaxVolume, false, &script);
			interface().putText(_soundNo);
			interface().changeLife(-_someLifeMultiple);
			break;

		case 17:
			sound().stop(_soundNo);
			sound().play(12609);
			_script.reset();
			static_cast<S2Room26000 &>(_parent).setFinalSequence(80);
			break;
		}
	}

	void drawBeam() {
		enum Direction { kLeft, kUp, kRight, kDown };

		for (auto &crystal : _crystals) {
			crystal->hide();
		}

		Direction currentDirection = kRight;
		int segmentNo = 0, y = 3, x = -1;
		for (bool isDone = false; !isDone; ) {
			if (segmentNo >= _beam.size()) {
				_beam.emplace_back(getPlane(), 5999, 4, 0);
			}
			auto &segment = _beam[segmentNo++];
			switch (currentDirection) {
			case kLeft:
				--x;
				break;
			case kUp:
				--y;
				break;
			case kRight:
				++x;
				break;
			case kDown:
				++y;
				break;
			}

			if (x < 0 || x >= _grid[0].size() || y < 0 || y >= _grid.size() ||
				_grid[y][x] == -1) {

				isDone = true;
				switch (currentDirection) {
				case kLeft:
				case kRight:
					segment.setCel(9);
					break;
				case kUp:
				case kDown:
					segment.setCel(8);
					break;
				}
			} else {
				switch (_grid[y][x]) {
				case 0:
					switch (currentDirection) {
					case kLeft:
						currentDirection = kDown;
						segment.setCel(2);
						break;
					case kUp:
						currentDirection = kRight;
						segment.setCel(2);
						break;
					case kRight:
						currentDirection = kUp;
						segment.setCel(6);
						break;
					case kDown:
						currentDirection = kLeft;
						segment.setCel(6);
						break;
					}
					break;

				case 1:
					switch (currentDirection) {
					case kLeft:
						currentDirection = kUp;
						segment.setCel(0);
						break;
					case kUp:
						currentDirection = kLeft;
						segment.setCel(4);
						break;
					case kRight:
						currentDirection = kDown;
						segment.setCel(4);
						break;
					case kDown:
						currentDirection = kRight;
						segment.setCel(0);
						break;
					}
					break;

				case 2:
				case 3:
				case 4: {
					auto i = _grid[y][x] - 2;
					_crystals[i]->show();
				}
					// fall through
				case 5:
					switch (currentDirection) {
					case kLeft:
					case kRight:
						segment.setCel(9);
						break;
					case kUp:
					case kDown:
						segment.setCel(8);
						break;
					}
					break;
				}
			}

			segment.setPriority(100);
			segment.setPosition(calcPoint(x, y), true);
			segment.show();
		}

		if (y == 3 && x == 4 && currentDirection == kRight) {
			if (segmentNo >= _beam.size()) {
				_beam.emplace_back(getPlane(), 5999, 4, 0);
			}
			auto &segment = _beam[segmentNo++];
			segment.setPriority(550);
			segment.setCel(5);
			if (!_crystals[0]->getIsVisible()) {
				segment.setPosition(GLPoint(252, 263 + 26 * y), true);
				segment.show();
			} else if (!_crystals[1]->getIsVisible()) {
				segment.setPosition(GLPoint(262, 263 + 26 * y), true);
				segment.show();
			} else if (!_crystals[2]->getIsVisible()) {
				segment.setPosition(GLPoint(272, 263 + 26 * y), true);
				segment.show();
			} else {
				segment.setCel(9);
				segment.setPosition(calcPoint(x, y), true);
				segment.show();

				{
					if (segmentNo >= _beam.size()) {
						_beam.emplace_back(getPlane(), 5999, 4, 0);
					}
					auto &nextSegment = _beam[segmentNo++];
					++x;
					nextSegment.setCel(9);
					nextSegment.setPriority(550);
					nextSegment.setPosition(calcPoint(x, y), true);
					nextSegment.show();
				}

				{
					if (segmentNo >= _beam.size()) {
						_beam.emplace_back(getPlane(), 5999, 4, 0);
					}
					auto &nextSegment = _beam[segmentNo];
					++x;
					nextSegment.setCel(4);
					nextSegment.setPriority(550);
					nextSegment.setPosition(calcPoint(x, y), true);
				}

				user().setIsHandsOn(false);
				sound().stop(_soundNo);
				sound().play(12608, false, Audio32::kMaxVolume, false, nullptr, make_reg(0, 1));
				setScript(self(finished));
			}
		}

		for (; segmentNo < _beam.size(); ++segmentNo) {
			_beam[segmentNo].hide();
		}
	}

	void finished(GLScript &script, const int state) {
		switch (state) {
		case 0: {
			auto &cel = emplaceCel(false, 5999, 2, 0, roomBottom, 600);
			cel.setCycleSpeed(1);
			cel.show();
			_cycler.reset(new GLCycler());
			_cycler->add(cel, true);
			// SSCI stopped the _soundNo sound here, but it was already stopped
			// by the caller, so is omitted
			score().doEvent(kScore228);
			// SSCI passed a soundNode for this sound, but it will never be
			// playing already, so this is omitted
			sound().play(12628);
			script.setSeconds(3);
			break;
		}

		case 1:
			sound().play(12628);
			_cycler.reset();
			user().setIsHandsOn(true);
			_script.reset();
			room().setNextRoomNo(26901);
			break;
		}
	}

	void toggleMirror(GLCel &cel, const int x, const int y) {
		if (_grid[y][x]) {
			cel.setCel(0, true);
			_grid[y][x] = 0;
		} else {
			cel.setCel(2, true);
			_grid[y][x] = 1;
		}

		drawBeam();
	}

	GLCel *_outerRing;
	GLCel *_innerRing;

	uint16 _soundNo = 42658;
	int _someLifeMultiple;

	Common::Array<GLScreenItem> _beam;
	Common::FixedArray<GLScreenItem *, 3> _crystals;
	Common::FixedArray<Common::FixedArray<GLCel *, 13>, 4> _cels;
	Common::FixedArray<Common::FixedArray<int8, 13>, 4> _grid = {{
		{{ -1, -1, -1, 0,  0,  2,  2, 0, 0, -1, -1, -1, -1 }},
		{{ -1,  0,  0, 0,  0,  0,  0, 5, 0,  3,  0, -1, -1 }},
		{{  0,  0,  0, 0,  5,  0,  0, 0, 0,  0,  0,  0, -1 }},
		{{  0,  0,  0, 0, -1, -1, -1, 0, 4,  0,  0,  0,  0 }}
	}};
};

#undef self
#define self(name) this, &S2Room26000::name

void S2Room26000::init(const int roomNo) {
	switch (roomNo) {
	case 26100:
		sound().createAmbient(26);
		enterFrom(50710, 826, 12602);
		room().drawPan(26100);
		addPanoramaExit(26200, 1328, 209, 1448, 347);
		addPanoramaExit(26400, 566, 223, 661, 334);
		addPanoramaExit(26110, 0, 148, 106, 473);
		addPanoramaExit(26110, 1893, 148, 2047, 473);
		break;

	case 26110:
		room().drawPic(26110);
		enter(roomNo, 12601, 12602, true);
		emplaceExit(true, 50700, 263, 68, 433, 365);
		break;

	case 26200: {
		enterFromRelative(26100, -300, 300);
		room().drawPan(26200);
		addPanoramaExit(26100, 231, 213, 365, 379);
		addPanoramaExit(26300, 984, 213, 1113, 365);
		break;
	}

	case 26300:
		sound().createAmbient(26);
		enterFromRelative(26200, -300, 300);
		room().drawPan(26300);
		if (room().getPreviousRoomNo() == 26300) {
			flags().set(kGameFlag204);
			sound().play(12606);
		}
		addPanoramaExit(26200, 1820, 211, 1939, 389);
		addPanoramaExit(26600, 557, 166, 632, 408);
		if (flags().get(kGameFlag204)) {
			addPanoramaExit(26310, 1248, 166, 1417, 473);
		}
		_keepPanoramaSprites = false;
		break;

	case 26310:
		room().drawPic(26310);
		enter(roomNo, 12605, 12606, true);
		emplaceExit(true, 47800, 263, 68, 433, 365);
		break;

	case 26400:
		enterFromRelative(26100, 300, -300);
		room().drawPan(26400);
		addPanoramaExit(26100, 1719, 211, 1870, 387);
		addPanoramaExit(26500, 1006, 211, 1143, 375);
		break;

	case 26500:
		enterFromRelative(26400, 300, 0);
		room().drawPan(26500);
		addPanoramaExit(26400, 47, 182, 210, 404);
		addPanoramaExit(26600, 1395, 162, 1467, 416);
		_keepPanoramaSprites = false;
		break;

	case 26600:
		room().drawPan(26600);
		if (inventory().isUsed(S2Inventory::kInv29)) {
			emplaceSprite(false, 26613, GLPoint(0, 389));
		}
		if (cursor().hasPrayerStick()) {
			_flag = GameFlag(kGameFlag102 + int(inventory().getPrayerStickId()) - 1);
			if (_flag == kGameFlag106) {
				_flag = kGameFlag114;
			}
		}
		drawPrayerSticks();
		setPrayerStickFlags();
		addPanoramaExit(26700, 0, 229, 244, 511);
		addPanoramaExit(26700, 1959, 229, 2047, 511);
		addPanoramaExit(26500, 582, 74, 674, 444);
		addPanoramaExit(26300, 1456, 94, 1559, 455);
		addPanoramaExit(26601, 871, 338, 890, 369, S2Cursor::kHighlightCel);
		addPanoramaExit(26602, 893, 338, 911, 375, S2Cursor::kHighlightCel);
		addPanoramaExit(26603, 917, 338, 934, 383, S2Cursor::kHighlightCel);
		addPanoramaExit(26604, 942, 350, 961, 389, S2Cursor::kHighlightCel);
		addPanoramaExit(26605, 967, 357, 990, 399, S2Cursor::kHighlightCel);
		addPanoramaExit(26606, 997, 366, 1019, 403, S2Cursor::kHighlightCel);
		addPanoramaExit(26607, 1105, 354, 1130, 397, S2Cursor::kHighlightCel);
		addPanoramaExit(26608, 1136, 346, 1159, 395, S2Cursor::kHighlightCel);
		addPanoramaExit(26609, 1166, 339, 1187, 393, S2Cursor::kHighlightCel);
		addPanoramaExit(26610, 1193, 340, 1212, 391, S2Cursor::kHighlightCel);
		addPanoramaExit(26611, 1217, 341, 1234, 387, S2Cursor::kHighlightCel);
		addPanoramaExit(26612, 1240, 341, 1259, 381, S2Cursor::kHighlightCel);
		_keepPanoramaSprites = true;
		break;

	case 26601:
	case 26602:
	case 26603:
	case 26604:
	case 26605:
	case 26606:
	case 26607:
	case 26608:
	case 26609:
	case 26610:
	case 26611:
	case 26612: {
		struct RoomInfo {
			GLPoint position;
			S2PrayerStick id;
		}

		static constexpr rooms[] = {
			{ { 861, 304 }, S2PrayerStick::Arrowhead },
			{ { 891, 307 }, S2PrayerStick::Claw },
			{ { 918, 307 }, S2PrayerStick::Bird },
			{ { 943, 310 }, S2PrayerStick::Hourglass },
			{ { 970, 316 }, S2PrayerStick::Jack },
			{ { 1001, 313 }, S2PrayerStick::Cow },
			{ { 1097, 319 }, S2PrayerStick::Star },
			{ { 1139, 319 }, S2PrayerStick::Spider },
			{ { 1167, 316 }, S2PrayerStick::Rain },
			{ { 1195, 317 }, S2PrayerStick::Butterfly },
			{ { 1218, 314 }, S2PrayerStick::Stairs },
			{ { 1240, 312 }, S2PrayerStick::Shield }
		};

		const auto index = roomNo - 26601;
		const auto &info = rooms[index];

		if (cursor().hasPrayerStick()) {
			flags().clear(kGameFlag217);
			if (inventory().hasPrayerStick(info.id)) {
				flags().set(GameFlag(kGameFlag115 + index));
				emplaceSprite(false, roomNo, info.position);
				inventory().dropPrayerStick();
				if (interface().getLife() != 100) {
					interface().changeLife(25);
				}
				score().doEvent(kScore3);
				sound().play(10006, false, 120);

				setPrayerStickFlags();
				if (flags().get(kGameFlag18)) {
					room().newRoom(26750);
				} else {
					room().newRoom(26600);
				}
			} else {
				const auto id = inventory().getPrayerStickId();
				inventory().dropPrayerStick();
				flags().clear(_flag);
				_prayerScript.reset(new GLScript(self(losePrayerStick), 0, int(id)));
				room().newRoom(26600);
			}
		} else {
			room().newRoom(26600);
		}
		break;
	}

	case 26700:
		room().drawPan(26700);
		if (inventory().isUsed(S2Inventory::kInv29)) {
			emplaceSprite(false, 26713, GLPoint(983, 412));
		}
		addPanoramaExit(26600, 878, 181, 1075, 325);
		addPanoramaExit(26730, 990, 341, 1070, 375, S2Cursor::kHighlightCel);
		addPanoramaExit(26720, 903, 353, 1197, 509, S2Cursor::kHighlightCel);
		_keepPanoramaSprites = false;
		drawPrayerSticks();
		break;

	case 26720: {
		room().drawPic(26720);

		S2Exit *exit;
		if (!inventory().isUsed(S2Inventory::kInv29)) {
			emplaceHotspot(true, 229, 132, 461, 365).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
				if (!flags().get(kGameFlag18) ||
					!flags().get(kGameFlag199) ||
					!inventory().isInUse(S2Inventory::kInv29) ||
					!inventory().isTaken(S2Inventory::Amulet)) {

					return;
				}

				_cel.reset(new GLCel(getPlane(), 26720, 0, 0, roomBottom));
				_cel->show();
				getPlane().getCast().remove(*_cel);
				inventory().setState(S2Inventory::kInv29, S2InventoryState::Used);
				removeChild(static_cast<S2Hotspot &>(target));
				interface().disableButtons();
				removeChild(*exit);
				user().setIsHandsOn(false);
				setScript(self(lookAtSand));
			});
		} else {
			auto &cel = emplaceCel(false, 26720, 0, 0, roomBottom);
			cel.show();
			getPlane().getCast().remove(cel);
		}

		exit = &emplaceExit(true, 26700, S2Cursor::kBackCel);
		break;
	}

	case 26721: {
		room().drawPic(26720);
		sound().deleteAmbient(26);
		auto &cel = emplaceCel(false, 26720, 0, 0, roomBottom);
		cel.show();
		getPlane().getCast().remove(cel);
		// CD swap code was here
		emplaceExit(true, 26850, 64, 0, 575, 383, S2Cursor::kBackCel);
		break;
	}

	case 26730:
		room().drawPic(26730);
		emplaceExit(true, 26700, S2Cursor::kBackCel);
		emplaceHotspot(true, 236, 150, 381, 294).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
			useTapePlayer(target);
		});
		break;

	case 26750:
		room().drawPic(55555);
		setScript(self(summonSpirit), room().getPreviousRoomNo() == 26730 ? 3 : 0);
		break;

	case 26800:
		room().drawPic(55555);
		// SSCI used two scripts with everything the same except the sound
		// number, so we pass the sound number as data instead to one script
		setScript(self(amuletSandMismatch), 0, 42642);
		break;

	case 26801:
		room().drawPic(55555);
		// SSCI used two scripts with everything the same except the sound
		// number, so we pass the sound number as data instead to one script
		if (inventory().isInUse(S2Inventory::Amulet)) {
			setScript(self(amuletSandMismatch), 0, 42641);
		} else {
			emplaceHotspot(true, 302, 98, 358, 162).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
				// Hotspot removal moved up from callee
				removeChild(static_cast<S2Hotspot &>(target));
				setScript(self(amuletSandMismatch), 0, 42641);
			});
		}
		break;

	case 26850:
		_numTimesFakeGiven = 0;
		_fakeIsInAltar = false;
		setScript(self(finalSequence));
		break;

	case 26900:
		setSubRoom<S2FinalPuzzle>(roomNo);
		break;

	case 26901: {
		room().drawPic(26730);
		user().setIsHandsOn(true);
		score().doEvent(kScore228);
		emplaceHotspot(true, 236, 150, 381, 294).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
			if (inventory().isInUse(S2Inventory::Ianyi)) {
				inventory().unselectItem(false);
				removeChild(static_cast<S2Hotspot &>(target));
				setScript(self(endgame));
			}
		});

		_cel.reset(new GLCel(getPlane(), 26730, 1, 0, roomBottom));
		_cel->show();
		_cel->setCycleSpeed(18);
		_cycler.reset(new GLPingPongCycler());
		_cycler->add(*_cel, true);
		setScript(self(takeIanyi));
		break;
	}

	case 26902:
		room().drawPic(55555);
		break;

	case 26999:
		setScript(self(cancelScript));
		break;
	}
}

void S2Room26000::dispose(const int roomNo) {
	const auto nextRoomNo = room().getNextRoomNo();
	if (nextRoomNo != 26999) {
		_cel.reset();
	}

	if (nextRoomNo < 26600 || nextRoomNo > 26612) {
		_keepPanoramaSprites = false;
	}

	if (!_keepPanoramaSprites) {
		_sticks = {};
	}

	_norahCycler.reset();

	switch (roomNo) {
	case 26600:
		_mover.reset();
		if (_prayerStick) {
			room().getPanorama().removeSprite(*_prayerStick);
		}
		_prayerStick.reset();
		break;

	case 26730:
		sound().stop(_spiritSoundNo);
		break;
	}

	S2Room::dispose(roomNo);
}

void S2Room26000::doIt() {
	if (room().getCurrentRoomNo() != 26850 || !_script || !_spirit) {
		return;
	}

	const auto state = _script->getState();
	if (state != 9 && state != 12 && state != 212) {
		return;
	}

	const auto currentItem = inventory().getCurrentItem();
	if (currentItem != S2Inventory::Amulet && currentItem != S2Inventory::FakeAmulet) {
		return;
	}

	setScript(self(finalSequence), 20);
}

void S2Room26000::cancelScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		_cycler.reset(new GLEndBackCycler());
		_cycler->add(*_cel);
		_cycler->start(script);
		sound().play(_exitSoundNo, false, 100);
		break;

	case 1:
		// Cycler was not reset until state 2, which would result in UAF
		_cycler.reset();
		_cel.reset();
		script.setTicks(10);
		break;

	case 2:
		_script.reset();
		switch (room().getPreviousRoomNo()) {
		case 26310:
			room().setNextRoomNo(26300);
			user().setIsHandsOn(true);
			break;

		case 26110:
			room().setNextRoomNo(26100);
			user().setIsHandsOn(true);
			break;

		default:
			error("Stuck in cancel room 26999");
		}
	}
}

void S2Room26000::takeIanyi(GLScript &script, const int state) {
	switch (state) {
	case 0:
		// SSCI waited for two cycles, which makes no difference
		script.setCycles(1);
		break;

	case 1:
		cursor().endHighlight();
		user().setIsHandsOn(true);
		cursor().getItem(inventory().getSmallCel(S2Inventory::Ianyi));
		inventory().setCurrentItem(S2Inventory::Ianyi);
		_script.reset();
		break;
	}
}

void S2Room26000::lookAtSand(GLScript &script, const int state) {
	switch (state) {
	case 0:
		sound().play(12604, false, 100, false, &script);
		break;

	case 1:
		room().setNextRoomNo(26721);
		_script.reset();
		break;
	}
}

void S2Room26000::useTapePlayer(GLTarget &target) {
	if (inventory().isInUse(S2Inventory::CompleteTapePlayer)) {
		inventory().unselectItem(true);
		user().setIsHandsOn(false);
		// Hotspot removal moved up from callee
		removeChild(static_cast<S2Hotspot &>(target));
		setScript(self(tapePlayer));
	}
}

void S2Room26000::tapePlayer(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_spiritSoundNo = 20019;
		sound().play(_spiritSoundNo, false, Audio32::kMaxVolume, false, &script);
		break;

	case 1:
		if (flags().get(kGameFlag18)) {
			room().setNextRoomNo(26750);
			_script.reset();
		} else {
			_cel.reset(new GLCel(getPlane(), 26730, 0, 0, roomBottom));
			_cel->show();
			_cycler.reset(new GLPingPongCycler());
			_cycler->add(*_cel);
			_spiritSoundNo = _game.getRandomNumber(42631, 42635);
			sound().play(_spiritSoundNo, false, 80, false, &script);
			interface().putText(_spiritSoundNo);
		}
		break;

	case 2:
		_cycler.reset();
		_cel.reset();
		emplaceHotspot(true, 236, 150, 381, 294).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
			useTapePlayer(target);
		});
		user().setIsHandsOn(true);
		_script.reset();
		break;
	}
}

void S2Room26000::amuletSandMismatch(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_cel.reset(new GLCel(getPlane(), 64001, 0, 0, GLPoint(250, 383)));
		_cel->show();
		_cycler.reset(new GLPingPongCycler());
		_cycler->add(*_cel, true);
		sound().play(script.getData(), false, 80, false, &script);
		interface().putText(script.getData());
		break;

	case 1:
		_cycler.reset();
		_cel.reset();
		room().setNextRoomNo(26700);
		_script.reset();
		break;
	}
}

void S2Room26000::summonSpirit(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_cel.reset(new GLCel(getPlane(), 64001, 0, 0, GLPoint(250, 383)));
		_cel->show();
		_cycler.reset(new GLPingPongCycler());
		_cycler->add(*_cel, true);
		sound().play(42636, false, 80, false, &script);
		interface().putText(42636);
		break;

	case 1: {
		_cycler.reset();
		_cel.reset();
		S2Exit *exit;
		emplaceHotspot(true, 250, 335, 417, 383).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
			if (inventory().isInUse(S2Inventory::CompleteTapePlayer)) {
				inventory().unselectItem(true);
				// hands off was done here but it is already done on the next
				// cue
				script.cue();
				removeChild(static_cast<S2Hotspot &>(target));
				removeChild(*exit);
			}
		});
		exit = &emplaceExit(true, 26700, S2Cursor::kBackCel);
		break;
	}

	case 2:
		user().setIsHandsOn(false);
		// In SSCI, exit & hotspot were removed here from the previous cue
		_spiritSoundNo = 20019;
		sound().play(_spiritSoundNo, false, Audio32::kMaxVolume, false, &script);
		interface().putText(_spiritSoundNo);
		break;

	case 3:
		_cel.reset(new GLCel(getPlane(), 64002, 0, 0, GLPoint(250, 383)));
		_cel->show();
		_cycler.reset(new GLEndCycler());
		_cycler->add(*_cel);
		_cycler->start(script);
		sound().play(12613);
		break;

	case 4: {
		_cycler.reset();
		_cel.reset(new GLCel(getPlane(), 64003, 0, 0, GLPoint(250, 383)));
		_cel->show();
		_cycler.reset(new GLPingPongCycler());
		_cycler->add(*_cel, true);
		uint16 soundNo;
		if (inventory().isNormal(S2Inventory::kInv33)) {
			soundNo = 42637;
		} else if (inventory().isTaken(S2Inventory::kInv33)) {
			if (inventory().isTaken(S2Inventory::kInv29)) {
				soundNo = 42643;
			} else {
				soundNo = 42642;
			}
		} else {
			soundNo = _game.getRandomNumber(42638, 42640);
		}
		sound().play(soundNo, false, 80, false, &script);
		interface().putText(soundNo);
		break;
	}

	case 5:
		_cycler.reset();
		_cel.reset(new GLCel(getPlane(), 64002, 0, 0, GLPoint(250, 383)));
		_cel->setCel(_cel->getLastCel());
		_cel->show();
		_cycler.reset(new GLEndBackCycler());
		_cycler->add(*_cel);
		_cycler->start(script);
		sound().play(12614);
		break;

	case 6:
		_cycler.reset();
		_cel.reset();
		flags().set(kGameFlag199);
		room().setNextRoomNo(26700);
		user().setIsHandsOn(true);
		_script.reset();
		break;
	}
}

void S2Room26000::drawPrayerSticks() {
	struct Position {
		GLPoint alternate;
		GLPoint standard;
	};

	static constexpr Position positions[] = {
		{ { 861, 304 }, { 959, 270 } },
		{ { 891, 307 }, { 969, 270 } },
		{ { 918, 307 }, { 979, 270 } },
		{ { 943, 310 }, { 986, 270 } },
		{ { 970, 316 }, { 994, 270 } },
		{ { 1001, 313 }, { 1005, 270 } },
		{ { 1097, 319 }, { 1034, 270 } },
		{ { 1139, 319 }, { 1045, 270 } },
		{ { 1167, 316 }, { 1054, 270 } },
		{ { 1195, 317 }, { 1060, 270 } },
		{ { 1218, 314 }, { 1069, 270 } },
		{ { 1240, 312 }, { 1074, 270 } }
	};

	const auto useAltSprite = room().getCurrentRoomNo() == 26600;
	for (int i = kGameFlag115; i <= kGameFlag126; ++i) {
		const auto index = i - kGameFlag115;
		if (flags().get(GameFlag(i)) && !_sticks[index]) {
			uint16 spriteNo = 26601 + index;
			GLPoint position = positions[index].alternate;
			if (!useAltSprite) {
				spriteNo += 100;
				position = positions[index].standard;
			}
			emplaceSprite(false, spriteNo, position);
			_sticks[index] = true;
		}
	}
}

void S2Room26000::setPrayerStickFlags() {
	int numFlags = 0;
	for (int i = kGameFlag115; i <= kGameFlag126; ++i) {
		if (flags().get(GameFlag(i))) {
			++numFlags;
		}
	}
	switch (numFlags) {
	case 12:
		if (flags().get(kGameFlag0)) {
			inventory().setState(S2Inventory::kInv33, S2InventoryState::Placed);
		}
		flags().set(kGameFlag18);
		break;

	case 8:
		flags().set(kGameFlag17);
		break;

	case 3:
		flags().set(kGameFlag16);
		break;

	case 1:
		flags().set(kGameFlag15);
		flags().set(kGameFlag31);
		flags().set(kGameFlag32);
		flags().set(kGameFlag33);
	}
}

void S2Room26000::losePrayerStick(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		_prayerStick.reset(new S2PanoramaSprite(26650 + script.getData(), GLPoint(1022, 225), 0, 1, true));
		room().getPanorama().addSprite(*_prayerStick, true);
		_prayerStick->setMoveSpeed(15);
		sound().play(42630, false, 80);
		interface().putText(42630);
		script.setCycles(1);
		break;

	case 1:
		_mover.reset(new S2PanoramaJump(*_prayerStick, GLPoint(1072, 225), script, 8));
		break;

	case 2:
		if (sound().isPlaying(42630)) {
			_mover.reset(new S2PanoramaJump(*_prayerStick, GLPoint(1022, 225), script, -8));
			script.setState(0);
		} else {
			_mover.reset();
			script.setCycles(1);
		}
		break;

	case 3:
		room().getPanorama().removeSprite(*_prayerStick);
		_prayerStick.reset();
		user().setIsHandsOn(true);
		_script.reset();
		break;
	}
}

void S2Room26000::endgame(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		_cycler.reset(new GLEndBackCycler());
		_cel->setLoop(0);
		_cel->setCel(6, true);
		_cycler->add(*_cel);
		_cycler->start(script);
		sound().play(42670);
		interface().putText(52618);
		script.setSeconds(3);
		break;

	case 1:
		_cycler.reset();
		_cel.reset();
		break;

	case 2:
		room().drawPic(2);
		movie().play(54172, nullptr, roomTop);
		script.setCycles(1);
		break;

	case 3:
		room().drawPic(60666);

		for (auto i = 0; i < _endCels.size(); ++i) {
			_endCels[i] = &emplaceCel(false, 60666, i, 0, roomBottom);
			_endCels[i]->show();
			_endCels[i]->setCycleSpeed(12);
		}

		// fall through
	case 4:
	case 5:
	case 6:
	case 7: {
		auto &cycler = _endCyclers[state - 3];
		cycler.reset(new GLEndCycler());
		cycler->add(*_endCels[state - 3]);
		cycler->start(script);
		sound().play(12631, false, Audio32::kMaxVolume, false, nullptr, make_reg(0, state - 2));
		break;
	}

	case 8:
		script.setTicks(20);
		break;

	case 9:
		for (auto &cycler : _endCyclers) {
			cycler.reset();
		}
		for (auto &cel : _endCels) {
			removeChild(*cel);
		}
		_cel.reset(new GLCel(getPlane(), 60667, 0, 0, roomBottom));
		_cel->show();
		_cel->setCycleSpeed(18);
		_cycler.reset(new GLEndCycler());
		_cycler->add(*_cel);
		_cycler->start(script);
		sound().play(12630);
		break;

	case 10:
		_cycler.reset();
		_cel.reset();
		room().drawPic(2);
		interface().changeLife(100, true);
		movie().play(54371, nullptr, GLPoint(154, 75), true, true);
		user().setIsHandsOn(true);
		_globalRoomNo = 4240;
		_script.reset();
		room().loadGlobalRoom(_globalRoomNo, true);
		break;
	}
}

void S2Room26000::finalSequence(GLScript &script, const int state) {
	user().setIsHandsOn(true);
	switch (state) {
	case 0:
		room().drawPic(55555);
		// In SSCI the placement of the cel is off
		_max = &emplaceCel(false, 55555, 0, 5, GLPoint(436, 290));
		_max->show();
		getPlane().getCast().removeEventHandler(*_max);
		user().setIsHandsOn(false);
		if (_debugFastForward) {
			script.setCycles(1);
		} else {
			playRobot(script, 52131, 52650);
		}
		_clickedMaxOnce = false;
		break;

	case 1:
		user().setIsHandsOn(false);
		_pool = &emplaceCel(false, 64002, 0, 0, GLPoint(250, 383));
		_pool->show();
		getPlane().getCast().removeEventHandler(*_pool);
		if (_debugFastForward) {
			_pool->setCel(_pool->getLastCel(), true);
			script.setCycles(1);
		} else {
			_cycler.reset(new GLEndCycler(*_pool, script));
			sound().play(12613);
		}
		break;

	case 2:
		user().setIsHandsOn(false);
		_cycler.reset();
		removeChild(*_pool);
		_pool = nullptr;
		_spirit = &emplaceCel(false, 64003, 0, 0, GLPoint(250, 383));
		_spirit->show();
		getPlane().getCast().removeEventHandler(*_spirit);
		_cycler.reset(new GLPingPongCycler(*_spirit, true));
		if (_debugFastForward) {
			script.setCycles(1);
		} else {
			playRobot(script, 52251, 52630);
		}
		break;

	case 3:
		user().setIsHandsOn(false);
		_norah = &emplaceCel(false, 60000, 0, 0, GLPoint(96, 287));
		_norah->show();
		getPlane().getCast().removeEventHandler(*_norah);
		if (_debugFastForward) {
			_norah->setCel(_norah->getLastCel(), true);
			script.setCycles(1);
		} else {
			_norahCycler.reset(new GLEndCycler(*_norah, script));
			playRobot(script, 52371, 52601);
		}
		break;

	case 4:
		user().setIsHandsOn(false);
		// SSCI recreated the entire cel object instead of just resetting its
		// view state
		_norah->setCelRes({ 60002, 0, 0 }, true);
		_norahCycler.reset(new GLPingPongCycler(*_norah, true));
		if (_debugFastForward) {
			script.setCycles(1);
		}
		break;

	case 5:
		user().setIsHandsOn(false);
		_max->setCel(0);
		if (_debugFastForward) {
			_max->forceUpdate();
		} else {
			movie().play(52531, nullptr, roomTop);
		}
		script.setCycles(1);
		break;

	case 6:
		user().setIsHandsOn(false);
		if (_debugFastForward) {
			script.setCycles(1);
		} else {
			playRobot(script, 52671, 52653);
		}
		addFinalHotspots();
		script.setState(211);
		break;

	case 212:
		if (_debugFastForward) {
			script.setCycles(1);
		} else {
			script.setSeconds(7);
		}
		// fall through
	case 12:
		user().setIsHandsOn(true);
		break;

	case 213:
		user().setIsHandsOn(false);
		_norahCycler.reset();
		_norah->setCelRes({ 60011, 0, 0 }, true);
		if (_debugFastForward) {
			_norah->setCel(_norah->getLastCel(), true);
			script.setCycles(1);
		} else {
			_norahCycler.reset(new GLEndCycler(*_norah, script));
			playRobot(script, 52871, 52603);
		}
		break;

	case 214:
		user().setIsHandsOn(false);
		_norah->setCelRes({ 60002, 0, 0 }, true);
		_norahCycler.reset(new GLPingPongCycler(*_norah, true));
		if (_debugFastForward) {
			script.setCycles(1);
		}
		break;

	case 215:
		user().setIsHandsOn(true);
		getPlane().getCast().remove(*_max);
		getPlane().getCast().remove(*_spirit);
		getPlane().getCast().remove(*_norah);
		script.setState(9);
		if (_debugFastForward) {
			script.setCycles(1);
		} else {
			script.setSeconds(10);
		}
		break;

	case 10:
		user().setIsHandsOn(false);
		_norah->setCelRes({ 60013, 0, 0 }, true);
		if (_debugFastForward) {
			_norah->setCel(_norah->getLastCel(), true);
			script.setCycles(1);
		} else {
			_norahCycler.reset(new GLEndCycler(*_norah, script));
			playRobot(script, 52971, 52604);
		}
		break;

	case 11:
		user().setIsHandsOn(false);
		_norah->setCelRes({ 60002, 0, 0 }, true);
		_norahCycler.reset(new GLPingPongCycler(*_norah, true));
		if (_debugFastForward) {
			script.setCycles(1);
		}
		break;

	case 13:
		user().setIsHandsOn(false);
		_norah->setCelRes({ 60010, 0, 0 }, true);
		_norahCycler.reset(new GLPingPongCycler(*_norah, true));
		_spiritSoundNo = _game.getRandomNumber(52605, 52607);
		if (_debugFastForward) {
			script.setCycles(1);
		} else {
			sound().play(_spiritSoundNo, false, Audio32::kMaxVolume, false, &script);
			interface().putText(_spiritSoundNo);
		}
		break;

	case 14:
		user().setIsHandsOn(false);
		_norah->setCelRes({ 60002, 0, 0 }, true);
		_norahCycler.reset(new GLPingPongCycler(*_norah, true));
		script.setState(11);
		script.setCycles(1);
		break;

	case 15:
		user().setIsHandsOn(false);
		_spirit->setCelRes({ 64014, 0, 0 }, true);
		_cycler.reset(new GLPingPongCycler(*_spirit, true));
		_spiritSoundNo = _game.getRandomNumber(42645, 42647);
		if (_debugFastForward) {
			script.setCycles(1);
		} else {
			sound().play(_spiritSoundNo, false, Audio32::kMaxVolume, false, &script);
			interface().putText(_spiritSoundNo);
		}
		break;

	case 16:
		user().setIsHandsOn(false);
		_spirit->setCelRes({ 64003, 0, 0 }, true);
		_cycler.reset(new GLPingPongCycler(*_spirit, true));
		script.setState(11);
		script.setCycles(1);
		break;

	case 17:
		user().setIsHandsOn(false);
		if (_debugFastForward) {
			script.setCycles(1);
		} else {
			playRobot(script, 53171, 52656);
		}
		script.setState(11);
		break;

	case 20:
		user().setIsHandsOn(true);
		_altarHotspot = &emplaceHotspot(true, 302, 95, 362, 168);
		_altarHotspot->setMouseUpHandler(self(clickedAltar));
		removeChild(*_spiritHotspot);
		_spiritHotspot = nullptr;
		_spirit->setCelRes({ 64002, 0, 0 });
		_spirit->setCel(_spirit->getLastCel(), true);
		if (_debugFastForward) {
			_spirit->setCel(0, true);
			script.setCycles(1);
		} else {
			_cycler.reset(new GLEndBackCycler(*_spirit, script));
			sound().play(12614);
		}
		script.setState(200);
		break;

	case 21:
		user().setIsHandsOn(true);
		if (!_altarHotspot) {
			_altarHotspot = &emplaceHotspot(true, 302, 95, 362, 168);
			_altarHotspot->setMouseUpHandler(self(clickedAltar));
		}
		if (!_maxHotspot) {
			_maxHotspot = &emplaceHotspot(true, 433, 28, 534, 295);
			_maxHotspot->setMouseUpHandler(self(clickedMax));
		}
		break;

	case 22:
		user().setIsHandsOn(false);
		playRobot(script, 55371, 52608);
		break;

	case 23:
		user().setIsHandsOn(false);
		clearFinalScreen();
		room().drawPic(2);
		movie().play(53431);
		interface().changeLife(1, true);
		movie().play(53511);
		_globalRoomNo = 4210;
		script.setState(999);
		script.setCycles(1);
		break;

	case 24:
		user().setIsHandsOn(true);
		break;

	case 25:
		user().setIsHandsOn(false);
		playRobot(script, 55471, 52668);
		script.setState(199);
		break;

	case 26:
		user().setIsHandsOn(false);
		playRobot(script, 54571, 52669);
		script.setState(20);
		break;

	case 30:
		if (_amulet) {
			removeChild(*_amulet);
			_amulet = nullptr;
		}
		removeFinalHotspots();
		_norah->setCelRes({ 60020, 0, 0 }, true);
		_norahCycler.reset(new GLPingPongCycler(*_norah, true));
		assert(!_spirit);
		_spirit = &emplaceCel(false, 64002, 0, 0, GLPoint(250, 383));
		_spirit->show();
		getPlane().getCast().remove(*_spirit);
		_cycler.reset(new GLEndCycler(*_spirit, script));
		sound().play(12613);
		break;

	case 31:
		_spirit->setCelRes({ 64003, 0, 0 }, true);
		_cycler.reset(new GLPingPongCycler(*_spirit, true));
		user().setIsHandsOn(false);
		playRobot(script, 53671, 52661);
		break;

	case 32:
		movie().play(53861);
		_norah->setCelRes({ 60001, 0, 0 }, true);
		_norahCycler.reset(new GLPingPongCycler(*_norah, true));
		script.setState(132);
		script.setSeconds(5);
		break;

	case 133:
		user().setIsHandsOn(false);
		_norahCycler.reset();
		_norah->setCelRes({ 60002, 0, 0 }, true);
		playRobot(script, 53971, 52612, _norah);
		script.setState(32);
		break;

	case 33:
		_spirit->setCelRes({ 64002, 0, 0 }, true);
		_cycler.reset(new GLEndBackCycler(*_spirit, script));
		break;

	case 34:
		clearFinalScreen();
		room().drawPic(2);
		movie().play(54171);
		movie().play(54371, nullptr, GLPoint(154, 75), true, true);
		_globalRoomNo = 4220;
		script.setState(999);
		script.setCycles(1);
		break;

	case 35:
		_norah->setCelRes({ 60001, 0, 0 }, true);
		_norahCycler.reset(new GLPingPongCycler(*_norah, true));
		_spiritSoundNo = _game.getRandomNumber(52616, 52617);
		sound().play(_spiritSoundNo, false, Audio32::kMaxVolume, false, &script);
		interface().putText(_spiritSoundNo);
		break;

	case 36:
		inventory().addItem(S2Inventory::FakeAmulet);
		_norah->setCelRes({ 60002, 0, 0 }, true);
		_norahCycler.reset(new GLPingPongCycler(*_norah, true));
		script.setState(20);
		script.setCycles(1);
		break;

	case 40:
		clearFinalScreen();
		room().drawPic(2);
		movie().play(54771);
		movie().play(54971);
		interface().changeLife(1, true);
		movie().play(55171);
		_globalRoomNo = 4230;
		script.setState(999);
		script.setCycles(1);
		break;

	case 50:
		_amulet = &emplaceCel(false, 55555, 2, 0, roomBottom, 20);
		_amulet->show();
		sound().play(12124);
		getPlane().getCast().remove(*_amulet);
		user().setIsHandsOn(false);
		if (_debugFastForward) {
			script.setCycles(1);
		} else {
			playRobot(script, 55261, -1);
		}
		script.setState(51);
		_max->setCel(4);
		_maxHotspot->setPoints(278, 67, 381, 288);
		removeChild(*_altarHotspot);
		_altarHotspot = nullptr;
		break;

	case 51:
		user().setIsHandsOn(false);
		if (_debugFastForward) {
			script.setCycles(1);
		} else {
			playRobot(script, 55361, 52674);
		}
		break;

	case 52:
		_amulet->setLoop(2);
		_amulet->setCel(1, true);
		user().setIsHandsOn(false);
		playRobot(script, 54671, 52675);
		_poolHotspot = &emplaceHotspot(true, 257, 339, 408, 383);
		_poolHotspot->setMouseUpHandler(self(clickedPool));
		break;

	case 53:
		getPlane().getCast().removeEventHandler(*_max);
		script.setState(59);
		script.setSeconds(_clickedMaxOnce ? 3 : 5);
		break;

	case 253:
		_spirit->setCelRes({ 64001, 0, 0 }, true);
		_cycler.reset(new GLPingPongCycler(*_spirit, true));
		script.setSeconds(2);
		break;

	case 254:
		_cycler.reset();
		removeChild(*_spirit);
		_spirit = nullptr;
		script.setState(52);
		script.setCycles(1);
		break;

	case 55:
		sound().play(52619, false, Audio32::kMaxVolume, false, &script);
		interface().putText(52619);
		script.setState(52);
		break;

	case 56:
		user().setIsHandsOn(false);
		playRobot(script, 55561, 52676);
		script.setState(52);
		break;

	case 60:
		_max->setCel(3);
		user().setIsHandsOn(false);
		playRobot(script, 55761, 52677);

		if (_altarHotspot) {
			removeChild(*_altarHotspot);
			_altarHotspot = nullptr;
		}

		removeChild(*_maxHotspot);
		_maxHotspot = nullptr;

		_altarHotspot = &emplaceHotspot(true, 297, 90, 367, 173);
		_altarHotspot->setMouseUpHandler(self(clickedAltar));
		script.setState(260);
		break;

	case 261:
	case 263:
	case 265:
	case 267:
		script.setSeconds(5);
		break;

	case 262:
		if (!inventory().isTaken(S2Inventory::FakeAmulet)) {
			user().setIsHandsOn(false);
			playRobot(script, 55861, 52678);
		}
		break;

	case 264:
		if (!inventory().isTaken(S2Inventory::FakeAmulet)) {
			user().setIsHandsOn(false);
			playRobot(script, 55961, 11072);
		}
		break;

	case 266:
		if (inventory().isTaken(S2Inventory::FakeAmulet)) {
			script.setState(65);
			script.setCycles(1);
		} else {
			auto robotNo = _game.getRandomNumber(56061, 56063);
			if (robotNo == 56063) {
				robotNo = 56066;
			}
			user().setIsHandsOn(false);
			playRobot(script, robotNo, robotNo - 3381);
		}
		break;

	case 268:
		script.setState(66);
		script.setCycles(1);
		break;

	case 65:
		_max->setCel(0);
		user().setIsHandsOn(false);
		playRobot(script, 56161, 56161);
		if (_maxHotspot) {
			removeChild(*_maxHotspot);
		}
		_maxHotspot = &emplaceHotspot(true, 433, 28, 634, 295);
		_maxHotspot->setMouseUpHandler(self(clickedMax));
		if (_norahHotspot) {
			removeChild(*_norahHotspot);
		}
		_norahHotspot = &emplaceHotspot(true, 96, 36, 216, 290);
		_norahHotspot->setMouseUpHandler(self(clickedNorah));
		if (_altarHotspot) {
			removeChild(*_altarHotspot);
		}
		_altarHotspot = &emplaceHotspot(true, 302, 95, 362, 168);
		_altarHotspot->setMouseUpHandler(self(clickedAltar));
		if (_poolHotspot) {
			removeChild(*_poolHotspot);
			_poolHotspot = nullptr;
		}
		script.setState(165);
		script.setSeconds(10);
		break;

	case 166:
		removeChild(*_amulet);
		_amulet = nullptr;
		user().setIsHandsOn(false);
		script.setState(20);
		break;

	case 66:
		_max->setCel(0);
		user().setIsHandsOn(false);
		playRobot(script, 56661, 52692);
		if (_poolHotspot) {
			removeChild(*_poolHotspot);
			_poolHotspot = nullptr;
		}
		script.setState(166);
		script.setSeconds(6);
		break;

	case 167:
		removeChild(*_amulet);
		_amulet = nullptr;
		script.setState(20);
		break;

	case 67:
		_amulet->setLoop(2);
		_amulet->setCel(1, true);
		user().setIsHandsOn(false);
		playRobot(script, 56861, 52694);
		if (_poolHotspot) {
			removeChild(*_poolHotspot);
			_poolHotspot = nullptr;
		}
		script.setSeconds(5);
		break;

	case 68:
		user().setIsHandsOn(true);
		removeChild(*_amulet);
		_amulet = nullptr;
		_max->setCel(0);
		_fakeIsInAltar = false;
		break;

	case 69:
		user().setIsHandsOn(false);
		script.setState(20);
		script.setCycles(1);
		break;

	case 70:
		score().doEvent(kScore229);
		if (_cycler) {
			_cycler.reset();
			removeChild(*_spirit);
			_spirit = nullptr;
		}
		_max->setCel(0);
		user().setIsHandsOn(false);
		playRobot(script, 57061, -1);
		break;

	case 71:
		_spirit = &emplaceCel(false, 64002, 0, 0, GLPoint(250, 383));
		_spirit->show();
		getPlane().getCast().remove(*_spirit);
		_cycler.reset(new GLEndCycler(*_spirit, script));
		sound().play(12613);
		break;

	case 72:
		clearFinalScreen();
		room().drawPic(5999);
		_max = &emplaceCel(false, 59999, 0, 0, GLPoint(436, 290));
		_max->show();
		getPlane().getCast().remove(*_max);
		_spirit = &emplaceCel(false, 59999, 1, 0, GLPoint(250, 383));
		_spirit->show();
		getPlane().getCast().remove(*_spirit);
		_norah = &emplaceCel(false, 59999, 2, 0, GLPoint(96, 287));
		_norah->show();
		getPlane().getCast().remove(*_norah);
		setSubRoom<S2FinalPuzzle>(26850);
		break;

	case 73:
		break;

	case 80:
		sound().stop(_spiritSoundNo);
		clearFinalScreen();
		room().drawPic(2);
		movie().play(57471);
		movie().play(54971);
		interface().changeLife(1, true);
		movie().play(55171);
		_globalRoomNo = 4230;
		script.setState(999);
		script.setCycles(1);
		break;

	case 85:
		sound().stop(_spiritSoundNo);
		_activeSubRoom.reset();
		room().drawPic(55555);
		inventory().addItem(S2Inventory::Ianyi);
		_max->setCelRes({ 55555, 0, 0 }, true);
		_spirit->setCelRes({ 64003, 0, 0 }, true);
		_norah->setCelRes({ 60002, 0, 0 }, true);
		_cycler.reset(new GLPingPongCycler(*_spirit, true));
		_norahCycler.reset(new GLPingPongCycler(*_norah, true));
		addFinalHotspots();
		_poolHotspot = &emplaceHotspot(true, 257, 339, 408, 383);
		_poolHotspot->setMouseUpHandler(self(clickedPool));
		script.setCycles(1);
		break;

	case 86:
		script.setSeconds(5);
		break;

	case 87:
		script.setState(89);
		script.setCycles(1);
		break;

	case 90:
		script.setState(_game.getRandomNumber(90, 92));
		script.setCycles(1);
		break;

	case 91: {
		const auto soundNo = _game.getRandomNumber(42667, 42669);
		sound().play(soundNo, false, Audio32::kMaxVolume, false, &script);
		interface().putText(soundNo);
		script.setState(85);
		break;
	}

	case 92: {
		const auto robotNo = _game.getRandomNumber(57771, 57772);
		_max->setCel(0);
		user().setIsHandsOn(false);
		playRobot(script, robotNo, robotNo - 5074);
		script.setState(85);
		break;
	}

	case 93: {
		const auto soundNo = _game.getRandomNumber(52622, 52623);
		sound().play(soundNo, false, Audio32::kMaxVolume, false, &script);
		interface().putText(soundNo);
		script.setState(85);
		break;
	}

	case 94:
		sound().play(52625, false, Audio32::kMaxVolume, false, &script);
		interface().putText(52625);
		break;

	case 95:
		clearFinalScreen();
		room().drawPic(2);
		movie().play(57471);
		movie().play(54971);
		interface().changeLife(1, true);
		movie().play(55171);
		_globalRoomNo = 4230;
		script.setState(999);
		script.setCycles(1);
		break;

	case 96:
		clearFinalScreen();
		room().drawPic(2);
		movie().play(53431);
		interface().changeLife(1, true);
		movie().play(53511);
		_globalRoomNo = 4210;
		script.setState(999);
		script.setCycles(1);
		break;

	case 97:
		_spirit->setCelRes({ 64002, 0, 0 }, true);
		_cycler.reset(new GLEndBackCycler(*_spirit, script));
		break;

	case 98:
		clearFinalScreen();
		room().drawPic(2);
		interface().changeLife(100, true);
		movie().play(54171);
		movie().play(54371, nullptr, { 154, 75 }, true, true);
		_globalRoomNo = 4240;
		script.setState(999);
		script.setCycles(1);
		break;

	case 200:
		inventory().addItem(S2Inventory::FakeAmulet);
		script.setState(20);
		script.setCycles(1);
		break;

	case 201:
		if (_spirit) {
			_cycler.reset();
			removeChild(*_spirit);
			_spirit = nullptr;
		}
		script.setState(20);
		script.setCycles(1);
		break;

	case 367:
		user().setIsHandsOn(false);
		assert(!_spirit);
		_spirit = &emplaceCel(false, 64002, 0, 0, GLPoint(250, 383));
		_spirit->show();
		getPlane().getCast().removeEventHandler(*_spirit);
		_cycler.reset(new GLEndCycler(*_spirit, script));
		sound().play(12613);
		break;

	case 368:
		user().setIsHandsOn(false);
		removeChild(*_amulet);
		_amulet = nullptr;
		_cycler.reset(new GLEndBackCycler(*_spirit, script));
		sound().play(12614);
		break;

	case 369:
		user().setIsHandsOn(false);
		script.setState(20);
		script.setCycles(1);
		break;

	case 1000:
		user().setIsHandsOn(true);
		cursor().goHandsOn();
		_script.reset();
		room().loadGlobalRoom(_globalRoomNo, true);
		break;

	default:
		warning("Empty state %d", state);
	}
}

void S2Room26000::addFinalHotspots() {
	_maxHotspot = &emplaceHotspot(true, 433, 28, 534, 295);
	_maxHotspot->setMouseUpHandler(self(clickedMax));
	_spiritHotspot = &emplaceHotspot(true, 274, 63, 381, 288);
	_spiritHotspot->setMouseUpHandler(self(clickedSpirit));
	_norahHotspot = &emplaceHotspot(true, 96, 36, 216, 290);
	_norahHotspot->setMouseUpHandler(self(clickedNorah));
}

void S2Room26000::removeFinalHotspots() {
	if (_maxHotspot) {
		removeChild(*_maxHotspot);
		_maxHotspot = nullptr;
	}
	if (_altarHotspot) {
		removeChild(*_altarHotspot);
		_altarHotspot = nullptr;
	}
	if (_spiritHotspot) {
		removeChild(*_spiritHotspot);
		_spiritHotspot = nullptr;
	}
	if (_norahHotspot) {
		removeChild(*_norahHotspot);
		_norahHotspot = nullptr;
	}
	if (_poolHotspot) {
		removeChild(*_poolHotspot);
		_poolHotspot = nullptr;
	}
}

void S2Room26000::clearFinalScreen() {
	_activeSubRoom.reset();
	_cycler.reset();
	_norahCycler.reset();
	removeFinalHotspots();
	movie().stopRobot(false);

	if (_max) {
		removeChild(*_max);
		_max = nullptr;
	}
	if (_spirit) {
		removeChild(*_spirit);
		_spirit = nullptr;
	}
	if (_norah) {
		removeChild(*_norah);
		_norah = nullptr;
	}
	if (_amulet) {
		removeChild(*_amulet);
		_amulet = nullptr;
	}
}

void S2Room26000::playRobot(GLScript &script, const uint16 robotNo, const int textNo, GLCel *client) {
	movie().initRobot(robotNo, getPlane(), 100, roomTop);
	movie().setRobotClient(client ? *client : *_max);
	movie().setRobotCaller(script);
	movie().playRobot();
	if (textNo != -1) {
		interface().putText(textNo);
	}
}

void S2Room26000::clickedMax(GLEvent &, GLTarget &) {
	const auto state = _script->getState();
	if (state == 9 || state == 12 || state == 212) {
		resetState(17);
	} else if (state == 21) {
		if (inventory().getCurrentItem() == S2Inventory::Amulet) {
			inventory().setState(S2Inventory::Amulet, S2InventoryState::Used);
			resetState(22);
		} else if (inventory().getCurrentItem() == S2Inventory::FakeAmulet) {
			inventory().setState(S2Inventory::FakeAmulet, S2InventoryState::Used);
			if (++_numTimesFakeGiven > 3) {
				resetState(26);
			} else {
				resetState(25);
			}
		}
	} else if (state == 53 || state == 59) {
		if (_clickedMaxOnce) {
			resetState(60);
		} else {
			_clickedMaxOnce = true;
			resetState(56);
		}
	} else if (state == 86 && inventory().getCurrentItem() == S2Inventory::Ianyi) {
		inventory().setState(S2Inventory::Ianyi, S2InventoryState::Used);
		_script->setState(95);
	}
}

void S2Room26000::clickedSpirit(GLEvent &, GLTarget &) {
	const auto state = _script->getState();
	if (state == 9 || state == 12 || state == 212) {
		resetState(15);
	} else if (state == 86 && inventory().getCurrentItem() == S2Inventory::Ianyi) {
		inventory().setState(S2Inventory::Ianyi, S2InventoryState::Used);
		_script->setState(94);
	}
}

void S2Room26000::clickedNorah(GLEvent &, GLTarget &) {
	const auto state = _script->getState();
	if (state == 9 || state == 12 || state == 212) {
		resetState(13);
	} else if (state == 21) {
		if (inventory().getCurrentItem() == S2Inventory::Amulet) {
			inventory().setState(S2Inventory::Amulet, S2InventoryState::Used);
			resetState(30);
		} else if (inventory().getCurrentItem() == S2Inventory::FakeAmulet) {
			inventory().setState(S2Inventory::FakeAmulet, S2InventoryState::Used);
			resetState(35);
		}
	} else if (state == 53) {
		if (inventory().getCurrentItem() == S2Inventory::Amulet) {
			resetState(55);
		}
	} else if (state == 86 && inventory().getCurrentItem() == S2Inventory::Ianyi) {
		_script->setState(93);
	}
}

void S2Room26000::clickedAltar(GLEvent &, GLTarget &) {
	const auto state = _script->getState();
	if (state == 21) {
		if (inventory().getCurrentItem() == S2Inventory::FakeAmulet) {
			if (_placedFakeAmulet) {
				inventory().unselectItem(false);
				_amulet = &emplaceCel(false, 55555, 1, 0, roomBottom);
				_amulet->show();
				inventory().setState(S2Inventory::FakeAmulet, S2InventoryState::Normal);
				resetState(367);
			} else {
				_placedFakeAmulet = _fakeIsInAltar = true;
				inventory().setState(S2Inventory::FakeAmulet, S2InventoryState::Used);
				resetState(50);
			}
		} else if (inventory().getCurrentItem() == S2Inventory::Amulet) {
			inventory().setState(S2Inventory::Amulet, S2InventoryState::Used);
			_amulet = &emplaceCel(false, 55555, 1, 1, roomBottom);
			_amulet->show();
			getPlane().getCast().remove(*_amulet);
			resetState(40);
		}
	} else if (state >= 261 && state <= 268) {
		if (inventory().getCurrentItem() == S2Inventory::Amulet && !_fakeIsInAltar) {
			inventory().setState(S2Inventory::Amulet, S2InventoryState::Used);
			_amulet->setLoop(1);
			_amulet->setCel(2, true);
			resetState(70);
		} else if (inventory().getCurrentItem() == S2Inventory::FakeAmulet) {
			inventory().setState(S2Inventory::FakeAmulet, S2InventoryState::Used);
			_placedFakeAmulet = _fakeIsInAltar = true;
			resetState(67);
		} else if (!inventory().isTaken(S2Inventory::FakeAmulet) && _fakeIsInAltar) {
			inventory().addItem(S2Inventory::FakeAmulet);
			_amulet->setLoop(2);
			_amulet->setCel(2, true);
			_fakeIsInAltar = false;
		} else {
			sound().play(52620);
			interface().putText(52620);
		}
	}
}

void S2Room26000::clickedPool(GLEvent &, GLTarget &) {
	const auto state = _script->getState();
	if (state == 53) {
		resetState(253);
	} else if (state == 86 && inventory().getCurrentItem() == S2Inventory::Ianyi) {
		inventory().setState(S2Inventory::Ianyi, S2InventoryState::Used);
		_script->setState(96);
	}
}

} // End of namespace Sci
