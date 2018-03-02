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

#include "sci/s2/rooms/6000.h"
#include "sci/s2/system/glmover.h"
#include "sci/s2/transparent_cel.h"

namespace Sci {

#define self(name) this, &S2BakeryDoorPuzzleRoom::name

class S2BakeryDoorPuzzleRoom : public S2SubRoom {
	static constexpr int16 calcX(const int x, const int y) {
		return 198 + x * 39 + y * -3;
	}

	static constexpr int16 calcY(const int x, const int y) {
		return 75 + x * 3 + y * 38;
	}

	static constexpr GLPoint calcPoint(const int x, const int y) {
		return { calcX(x, y), calcY(x, y) };
	}

public:
	using S2SubRoom::S2SubRoom;

	virtual void init(const int) override {
		room().drawPic(6122);
		emplaceExit(true, 6120, 64, 0, 109, 384, S2Cursor::kBackCel);
		emplaceExit(true, 6120, 495, 0, 575, S2Cursor::kBackCel);
		phone().cancelCall();

		const GLPoint handles[][4] = {
			{ { 117, 107 }, { 234, 116 }, { 228, 192 }, { 111, 183 } },
			{ { 111, 183 }, { 228, 192 }, { 222, 268 }, { 105, 259 } },
			{ { 243,   2 }, { 321,   8 }, { 312, 122 }, { 234, 116 } },
			{ { 321,   8 }, { 399,  14 }, { 390, 128 }, { 312, 122 } }
		};

		for (auto i = 0; i < ARRAYSIZE(handles); ++i) {
			emplaceHotspot(true, handles[i]).setMouseUpHandler([this, i](GLEvent &, GLTarget &target) {
				user().setIsHandsOn(false);
				setScript(self(interact), 0, i);
			});
		}

		const GLPoint gears[] = {
			{ 153, 148 }, { 147, 224 }, { 279, 43 }, { 357, 49 }
		};

		for (auto i = 0; i < _gears.size(); ++i) {
			auto &gear = emplaceCel(false, 6122, 5, 0, gears[i]);
			gear.setCycleSpeed(2);
			gear.show();
			_gears[i] = &gear;
		}

		for (auto i = 0; i < _pushers.size(); ++i) {
			auto &axis = _pushers[i];
			for (auto j = 0; j < axis.size(); ++j) {
				int16 loopNo;
				GLPoint position;
				if (!i) {
					position = calcPoint(j % 2, j + 1) + GLPoint( 1, 9 );
					loopNo = 1 + j % 2;
				} else {
					position = calcPoint(j + 1, j % 2) + GLPoint( 9, 1 );
					loopNo = 3 + j % 2;
				}
				auto &cel = emplaceCel(false, 6122, loopNo, 0, position, 0);
				cel.show();
				cel.setMoveSpeed(0);
				cel.setStepSize({ 12, 12 });
				getPlane().getCast().remove(cel);
				axis[j] = &cel;
			}
		}

		for (auto y = 0; y < _pieces.size(); ++y) {
			auto &row = _pieces[y];
			for (auto x = 0; x < row.size(); ++x) {
				if (_layout[y][x]) {
					auto &cel = emplaceCel(false, 6122, 0, 0, calcPoint(x, y));
					cel.show();
					cel.setMoveSpeed(0);
					cel.setStepSize({ 12, 12 });
					row[x] = &cel;
				} else {
					row[x] = nullptr;
				}
			}
		}

		auto j = _game.getRandomNumber(1, 7);
		for (auto i = 0; i < 4; ++i) {
			const auto y = _initialStates[j][i] / 6;
			const auto x = _initialStates[j][i] % 6;
			_circlePieces[i] = _pieces[y][x];
			_circlePieces[i]->setCel(i + 1, true);
		}
	}

private:
	void interact(GLScript &script, const int state) {
		const auto gearIndex = script.getData() % 2;
		const bool isHorizontal = script.getData() < 2;

		// SSCI duplicated this function for horizontal and vertical movements
		// instead of using one function and changing some variables
		GLPoint delta;
		uint pieceRow, pieceColumn;
		bool cycleForward;
		if (isHorizontal) {
			pieceRow = gearIndex * 2 + 1;
			pieceColumn = 0;
			delta = { 39, 3 };
			cycleForward = false;
		} else {
			pieceRow = 0;
			pieceColumn = gearIndex * 2 + 1;
			delta = { -3, 38 };
			cycleForward = true;
		}

		switch (state) {
		case 0:
			_gearCycler.reset(new GLCycler());
			_gearCycler->cycleForward(_pieces[pieceRow][pieceColumn] ? cycleForward : !cycleForward);
			_gearCycler->add(*_gears[script.getData()], true);

			for (auto i = 0; i < 2; ++i) {
				const auto row = pieceRow ? pieceRow + i : 0;
				const auto col = pieceColumn ? pieceColumn + i : 0;
				const auto right = _pieces[row][col] ? 0 : 1;
				for (auto j = 0; j < 6; ++j) {
					const auto innerRow = row ? row : j + right;
					const auto innerCol = row ? j + right : col;
					if (j < 5) {
						_movingPieces[i][j] = _pieces[innerRow][innerCol];
					} else {
						_movingPieces[i][5] = _pushers[row ? 0 : 1][(row ? row : col) - 1];
					}

					auto newPosition = _movingPieces[i][j]->getPosition();
					if (right) {
						newPosition -= delta;
					} else {
						newPosition += delta;
					}

					if (!i && !j) {
						_movers[i][j].reset(new GLMover(*_movingPieces[i][j], newPosition, script));
					} else {
						_movers[i][j].reset(new GLMover(*_movingPieces[i][j], newPosition));
					}
				}
			}

			// In SSCI this sound was played for every iteration of the loop,
			// which is unnecessary since each new play restarts the sound
			sound().play(10602, true, 40);

			break;

		case 1:
			_gearCycler.reset();
			script.setTicks(15);
			break;

		case 2:
			sound().stop(10602);
			for (auto i = 0; i < 2; ++i) {
				const auto row = pieceRow ? pieceRow + i : 0;
				const auto col = pieceColumn ? pieceColumn + i : 0;
				const auto right = _pieces[row][col] ? 0 : 1;
				for (auto j = 0; j < 6; ++j) {
					_movers[i][j].reset();
					if (j < 5) {
						if (isHorizontal) {
							if (right) {
								_pieces[row][j] = _pieces[row][j + 1];
							} else {
								_pieces[row][5 - j] = _pieces[row][4 - j];
							}
						} else {
							if (right) {
								_pieces[j][col] = _pieces[j + 1][col];
							} else {
								_pieces[5 - j][col] = _pieces[4 - j][col];
							}
						}
					}
				}
				if (isHorizontal) {
					_pieces[row][right ? 5 : 0] = nullptr;
				} else {
					_pieces[right ? 5 : 0][col] = nullptr;
				}
			}

			_script.reset();
			checkFinished();
			user().setIsHandsOn(true);
			break;
		}
	}

	void checkFinished() {
		auto i = -1;
		uint x, y;
		do {
			++i;
			x = i / 6;
			y = i % 6;
		} while (_circlePieces[0] != _pieces[x][y]);

		if (x < 5 && y < 5 &&
			_circlePieces[1] == _pieces[x + 1][y] &&
			_circlePieces[2] == _pieces[x][y + 1] &&
			_circlePieces[3] == _pieces[x + 1][y + 1]) {

			flags().set(kGameFlag133);
			score().doEvent(kScore206);
			room().setNextRoomNo(6121);
		}
	}

	Common::ScopedPtr<GLCycler> _gearCycler;
	Common::FixedArray<GLCel *, 4> _gears;
	Common::FixedArray<Common::FixedArray<GLCel *, 4>, 2> _pushers;
	Common::FixedArray<Common::FixedArray<GLCel *, 6>, 6> _pieces;
	Common::FixedArray<Common::FixedArray<GLCel *, 6>, 2> _movingPieces;
	Common::FixedArray<GLCel *, 4> _circlePieces;
	Common::FixedArray<Common::FixedArray<Common::ScopedPtr<GLMover>, 6>, 2> _movers;

	static constexpr int8 _layout[6][6] = {
		{ 0, 1, 0, 1, 0, 0 },
		{ 1, 1, 1, 1, 1, 0 },
		{ 0, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 1, 1, 0 },
		{ 0, 1, 1, 1, 1, 1 },
		{ 0, 0, 1, 0, 1, 0 }
	};

	static constexpr int8 _initialStates[8][4] = {
		{ 14, 20, 15, 21 }, // completed state
		{  3,  6, 29, 32 },
		{ 18, 15, 29, 14 },
		{ 18, 26,  1, 17 },
		{ 22, 20, 19, 21 },
		{ 14, 15,  9, 21 },
		{ 14, 20, 15, 17 },
		{ 18, 26, 15, 13 }
	};
};

constexpr int8 S2BakeryDoorPuzzleRoom::_layout[6][6];
constexpr int8 S2BakeryDoorPuzzleRoom::_initialStates[8][4];

#undef self
#define self(name) this, &S2BankDoorPuzzle::name

class S2BankDoorPuzzle : public S2SubRoom {
public:
	using S2SubRoom::S2SubRoom;

	virtual void init(const int) override {
		room().drawPic(6222);
		phone().cancelCall();
		emplaceExit(true, 6220, S2Cursor::kBackCel);

		for (auto i = 0; i < _cels.size(); ++i) {
			auto &cel = emplaceTransparentCel(true, 6222, i, 0, GLPoint(318, 183));
			cel.setSelectHandler([this, i](GLEvent &event, GLTarget &target) {
				if (event.getType() == kSciEventMouseRelease) {
					handleTabEvent(static_cast<S2TransparentCel &>(target), i);
				}
			});
			cel.show();
			cel.forceUpdate();
			_cels[i] = &cel;
		}

		auto &reset = emplaceButton(true, true, 6222, 11, 0, roomBottom);
		reset.setMouseUpHandler([&](GLEvent &, GLTarget &) {
			user().setIsHandsOn(false);
			sound().play(10606, false, 100);
			for (auto &cel : _cels) {
				cel->setCel(0, true);
				_numCorrect = _numDown = _targetNo = _currentNo = 0;
			}
			setScript(self(spin));
		});
		reset.show();
		reset.forceUpdate();
		addAsCel(reset);

		_dial1 = &emplaceCel(false, 6222, 9, 0, GLPoint(318, 183));
		_dial1->setCycleSpeed(2);
		_dial1->show();
		_dial10 = &emplaceCel(false, 6222, 10, 0, GLPoint(318, 183));
		_dial10->setCycleSpeed(2);
		_dial10->show();
	}

private:
	void handleTabEvent(S2TransparentCel &cel, const int i) {
		if (cel.getCel()) {
			return;
		}
		user().setIsHandsOn(false);
		if (_numDown == 3) {
			_numDown = 1;
			_targetNo = i;
			cel.setCel(1, true);
			setScript(self(spin));
		} else {
			++_numDown;
			cel.setCel(1, true);

			int initialState;
			if (i) {
				_targetNo = _currentNo + i;
				initialState = 1;
			} else {
				initialState = 4;
			}
			setScript(self(spin), initialState);
		}
		sound().play(10605, false, 100);
	}

	void spin(GLScript &script, const int state) {
		switch (state) {
		case 0: {
			const auto d1Cel = _dial1->getCel();
			const auto d10Cel = _dial10->getCel();
			_currentNo = 0;
			if (d1Cel > 0) {
				_dial1->setCel(d1Cel - 1, true);
			}
			if (d10Cel > 0) {
				_dial10->setCel(d10Cel - 1, true);
			}
			if (d1Cel > 0 || d10Cel > 0) {
				script.setState(-1);
			} else if (_targetNo == 0) {
				script.setState(2);
			}
			script.setTicks(4);
			break;
		}

		case 1: {
			const auto d1Cel = _currentNo % 10;
			const auto d10Cel = _currentNo / 10;
			if (d1Cel == 9) {
				_dial10->setCel(d10Cel * 2 + 1, true);
			}
			_dial1->setCel(d1Cel * 2 + 1, true);
			script.setTicks(4);
			break;
		}

		case 2: {
			++_currentNo;
			const auto d1Cel = _currentNo % 10;
			const auto d10Cel = _currentNo / 10;
			if (d1Cel == 0) {
				_dial10->setCel(d10Cel * 2, true);
			}
			_dial1->setCel(d1Cel * 2, true);
			if (_currentNo < _targetNo) {
				script.setState(0);
			}
			script.setTicks(4);
			break;
		}

		case 3:
			if (_numDown == 3 && _currentNo == (11 + _numCorrect) && ++_numCorrect == 3) {
				flags().set(kGameFlag135);
				sound().play(10607, false, 100);
				room().setNextRoomNo(6221);
			}

			_script.reset();
			user().setIsHandsOn(true);
			break;

		case 4:
			script.setState(2);
			script.setTicks(4);
			break;
		}
	}

	Common::FixedArray<S2TransparentCel *, 9> _cels;
	GLCel *_dial1;
	GLCel *_dial10;

	int _numCorrect = 0, _numDown = 0, _targetNo = 0, _currentNo = 0;
};

#undef self
#define self(name) this, &S2Room6000::name

void S2Room6000::init(const int roomNo) {
	if (room().getPreviousRoomNo() > 6999 && roomNo != 6666) {
		if (flags().get(kGameFlag15) && !flags().get(kGameFlag36)) {
			sound().createAmbient(6);
			room().drawPic(5000);
			flags().set(kGameFlag36);
			room().setNorah(59005, roomNo);
			_ethereal.reset(new GLScript(self(goToEthereal)));
			return;
		} else if (flags().get(kGameFlag16) && !flags().get(kGameFlag37)) {
			sound().createAmbient(6);
			room().drawPic(5000);
			flags().set(kGameFlag37);
			room().setNorah(59006, roomNo);
			_ethereal.reset(new GLScript(self(goToEthereal)));
			return;
		} else if (flags().get(kGameFlag17) && !flags().get(kGameFlag14)) {
			sound().createAmbient(6);
			room().drawPic(5000);
			room().setNorah(59007, roomNo);
			_choose = true;
			_ethereal.reset(new GLScript(self(goToEthereal)));
			return;
		}
	}

	switch (roomNo) {
	case 6000:
	case 6100:
		sound().createAmbient(7);
		enterFrom(13105, 1847, 21302);
		room().drawPan(6100);
		addPanoramaExit(6110, 1421, 195, 1745, 386);
		addPanoramaExit(6101, 950, 259, 987, 313);
		addPanoramaExit(6102, 1032, 269, 1080, 315, S2Cursor::kHighlightCel);
		_fan.reset(new GLScript(self(animateFan)));
		_sign.reset(new GLScript(self(animateSign), 3));
		break;

	case 6101:
		room().drawPic(6101);
		enter(roomNo, 21301, 21302, true);
		emplaceExit(true, 13100, 158, 94, 276, 299);
		break;

	case 6102: {
		room().drawPic(6102);
		exitBorder(6100);

		if (inventory().isUsed(S2Inventory::kInv49)) {
			emplaceHotspot(true, 308, 146, 358, 220).setMouseUpHandler([&](GLEvent &, GLTarget &) {
				if (inventory().isInUse(S2Inventory::kInv49)) {
					inventory().setState(S2Inventory::kInv49, S2InventoryState::Used);
					setScript(self(useGasPump));
				};
			});
		}

		auto &cel = emplaceCel(false, 6102, 2, 0, roomBottom, 400);
		cel.show();
		getPlane().getCast().remove(cel);
		break;
	}

	case 6110:
		if (!flags().get(kGameFlag29) && inventory().hasPrayerStick()) {
			flags().set(kGameFlag29);
			room().drawPic(2);
			movie().play(5110, nullptr, roomTop);
		}

		room().drawPan(6110);

		addPanoramaExit(6120, 1497, 200, 1587, 439);
		addPanoramaExit(6310, 1200, 234, 1362, 354);
		addPanoramaExit(6100, 370, 212, 582, 323);
		addPanoramaExit(6111, 121, 166, 151, 272, S2Cursor::kHighlightCel);
		addPanoramaExit(6315, 1177, 182, 1257, 229, S2Cursor::kHighlightCel);
		_sign.reset(new GLScript(self(animateSign)));
		_fan.reset(new GLScript(self(animateFan), 3));
		break;

	case 6111:
		room().drawPic(6111);
		emplaceExit(true, 6110, S2Cursor::kBackCel);
		score().doEvent(kScore65);
		break;

	case 6120:
		sound().createAmbient(7);
		enterFrom(17150, 1821, 11710);
		room().drawPan(6120);

		addPanoramaExit(6130, 1431, 205, 1643, 304);
		addPanoramaExit(6110, 398, 213, 618, 340);

		if (flags().get(kGameFlag133)) {
			addPanoramaExit(6121, 968, 213, 1046, 338);
		} else {
			addPanoramaExit(6122, 932, 215, 962, 253, S2Cursor::kHighlightCel);
		}

		_flag.reset(new GLScript(self(animateFlag)));
		_fan.reset(new GLScript(self(animateFan), 9));

		break;

	case 6121:
		room().drawPic(6121);
		enter(roomNo, 11709, 11710, false);
		exitBorder(6999);
		emplaceExit(true, 17100, 290, 92, 399, 383);
		break;

	case 6122:
		setSubRoom<S2BakeryDoorPuzzleRoom>(roomNo);
		break;

	case 6130:
		sound().createAmbient(7);
		room().drawPan(6130);
		addPanoramaExit(6140, 1890, 126, 2047, 308);
		addPanoramaExit(6140, 1766, 136, 2048, 400);
		addPanoramaExit(6230, 1005, 190, 1160, 500);
		addPanoramaExit(6120, 383, 212, 606, 415);
		break;

	case 6140:
		sound().createAmbient(8);
		room().drawPan(6140);
		addPanoramaExit(6150, 1416, 205, 1658, 355);
		addPanoramaExit(6130, 715, 203, 1009, 357);
		break;

	case 6150:
		room().drawPan(6150);
		addPanoramaExit(6180, 1368, 210, 1546, 359);
		addPanoramaExit(6140, 486, 195, 691, 383);
		addPanoramaExit(6160, 0, 150, 120, 380);
		addPanoramaExit(6160, 1975, 150, 2047, 380);
		break;

	case 6160:
		room().drawPan(6160);
		addPanoramaExit(6150, 930, 200, 1140, 385);
		addPanoramaExit(6170, 151, 147, 251, 306);
		_birds.reset(new GLScript(self(animateBirds)));
		break;

	case 6170:
		room().drawPan(6170);
		addPanoramaExit(6160, 1123, 201, 1290, 351);
		addPanoramaExit(6380, 1850, 170, 1950, 340);
		break;

	case 6180:
		room().drawPan(6180);
		addPanoramaExit(6190, 1636, 170, 1894, 357);
		addPanoramaExit(6220, 1053, 210, 1270, 352);
		addPanoramaExit(6150, 180, 170, 325, 400);
		break;

	case 6190:
		sound().createAmbient(8);
		enterFrom(15050, 1101, 11530);
		room().drawPan(6190);
		addShadowRoomExits();
		if (!flags().get(kGameFlag138)) {
			setScript(self(showShadow));
		}
		break;

	case 6191:
		room().drawPic(6191);
		enter(roomNo, 11531, 11530, true);
		emplaceExit(true, 15000, 252, 93, 394, 379);
		break;

	case 6192:
		room().drawPic(6192);
		emplaceExit(true, 6190, S2Cursor::kBackCel);
		score().doEvent(kScore64);
		break;

	case 6200:
		sound().createAmbient(8);
		enterFrom(23180, 305, 22302);
		room().drawPan(6200);
		if (flags().get(kGameFlag127)) {
			emplaceSprite(false, 6201, GLPoint(1608, 292));
		}

		addPanoramaExit(6210, 32, 129, 184, 316);
		addPanoramaExit(6190, 845, 244, 972, 413);
		addPanoramaExit(6201, 1545, 216, 1606, 329);
		break;

	case 6201:
		room().drawPic(6201);
		if (flags().get(kGameFlag127)) {
			auto &cel = emplaceCel(false, 6201, 1, 0, roomBottom);
			cel.show();
			getPlane().getCast().remove(cel);
		}

		exitBorder(6200);
		emplaceExit(true, 6204, 302, 143, 361, 203, S2Cursor::kHighlightCel);

		if (inventory().isPlaced(S2Inventory::kInv24)) {
			const auto exitNo = flags().get(kGameFlag127) ? 6203 : 6202;
			emplaceExit(true, exitNo, 427, 289, 575, 383, S2Cursor::kHighlightCel);
		}

		break;

	case 6202:
		room().drawPic(6202);
		emplaceExit(true, 6201, S2Cursor::kBackCel);
		emplaceHotspot(true, 156, 105, 382, 248).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			if (inventory().isInUse(S2Inventory::kInv9)) {
				inventory().setState(S2Inventory::kInv9, S2InventoryState::Used);
				setScript(self(prySiding));
			} else {
				sound().play(10003, false, 120);
			}
		});
		break;

	case 6203: {
		room().drawPic(6203);
		exitBorder(6201);

		auto &cel = emplaceCel(false, 6203, 0, 0, roomBottom);
		cel.show();
		getPlane().getCast().remove(cel);

		if (inventory().isPlaced(S2Inventory::kInv24)) {
			emplaceHotspot(true, 158, 174, 221, 222).setMouseUpHandler([&](GLEvent &, GLTarget &) {
				setScript(self(openKeyBox));
			});
		}

		break;
	}

	case 6204:
		room().drawPic(6201);
		if (flags().get(kGameFlag127)) {
			auto &cel = emplaceCel(false, 6201, 1, 0, roomBottom);
			cel.show();
			getPlane().getCast().remove(cel);
		}

		enter(6201, 22301, 22302, false);
		exitBorder(6999);
		emplaceExit(true, 23100, 270, 47, 393, 381);
		break;

	case 6210:
		room().drawPan(6210);
		addPanoramaExit(6200, 957, 233, 1132, 387);
		addPanoramaExit(6390, 203, 119, 406, 381);
		break;

	case 6220: {
		sound().createAmbient(8);
		enterFrom(21120, 366, 22102);
		room().drawPan(6220);
		addPanoramaExit(6250, 963, 189, 1236, 348);
		addPanoramaExit(6180, 49, 181, 274, 363);

		const auto exitNo = flags().get(kGameFlag135) ? 6221 : 6222;
		addPanoramaExit(exitNo, 1493, 210, 1557, 308, S2Cursor::kHighlightCel);

		break;
	}

	case 6221:
		room().drawPic(6221);
		enter(roomNo, 22101, 22102, false);
		exitBorder(6999, false);
		emplaceExit(true, 21100, 280, 50, 440, 383);
		break;

	case 6222:
		setSubRoom<S2BankDoorPuzzle>(roomNo);
		break;

	case 6230:
		sound().createAmbient(6);

		if (!flags().get(kGameFlag39) && inventory().isPlaced(S2Inventory::kInv33)) {
			room().drawPic(5000);
			flags().set(kGameFlag39);
			room().setNorah(59021, 6230);
			_ethereal.reset(new GLScript(self(goToEthereal)));
		} else {
			room().drawPan(6230);
			enterFrom(6321, -1, 11536);
			addPanoramaExit(6270, 650, 196, 844, 370);
			addPanoramaExit(6130, 145, 134, 394, 368);
			addPanoramaExit(6231, 1644, 199, 1836, 380);
			addPanoramaExit(6240, 1131, 193, 1370, 368);
			_cafeLight.reset(new GLScript(self(animateCafeLight), 0, 6230));
		}

		break;

	case 6231:
		room().drawPic(6231);
		enter(roomNo, 11535, 11536, false);
		exitBorder(6999);
		emplaceExit(true, 6320, 97, 62, 471, 383);
		break;

	case 6240:
		sound().createAmbient(6);
		enterFrom(16120, -1, 11609);
		room().drawPan(6240);
		addPanoramaExit(6250, 1423, 168, 1533, 435);
		addPanoramaExit(6230, 222, 180, 554, 370);
		addPanoramaExit(6241, 825, 230, 925, 310, S2Cursor::kHighlightCel);
		break;

	case 6241:
		room().drawPic(6241);
		enter(roomNo, 11610, 11609, true);
		emplaceExit(true, 16100, 137, 84, 466, 356);
		break;

	case 6250:
		sound().createAmbient(6);
		if (!flags().get(kGameFlag106) && !flags().get(kGameFlag43)) {
			room().drawPic(5000);
			flags().set(kGameFlag43);
			room().setNorah(59027, 6250);
			_ethereal.reset(new GLScript(self(goToEthereal)));
		}

		enterFrom(20110, 1800, 21802);
		room().drawPan(6250);

		addPanoramaExit(6220, 1941, 170, 2047, 351);

		// In SSCI this exit's rectangle was broken, x2 < x1
		addPanoramaExit(6220, 56, 0, 170, 351);

		addPanoramaExit(6290, 828, 144, 1022, 344);
		addPanoramaExit(6240, 516, 195, 716, 324);
		addPanoramaExit(6252, 1707, 232, 1755, 309);
		addPanoramaExit(6260, 1328, 202, 1494, 378);
		addPanoramaExit(6251, 1112, 187, 1238, 315);
		break;

	case 6251:
		room().drawPic(6251);
		enter(roomNo, 21801, 21802, true);
		emplaceExit(true, 20100, 186, 29, 396, 361);
		break;

	case 6252:
		room().drawPic(6252);
		enter(roomNo, 22101, 22102, true);
		emplaceExit(true, 14100, 217, 53, 373, 337);
		break;

	case 6260:
		room().drawPan(6260);
		addPanoramaExit(6250, 400, 200, 535, 375);
		addPanoramaExit(6540, 1312, 146, 1385, 309);
		addPanoramaExit(6540, 1386, 224, 1452, 309);
		addPanoramaExit(6261, 1389, 148, 1493, 224, S2Cursor::kHighlightCel);
		break;

	case 6261:
		room().drawPic(6261);
		emplaceExit(true, 6260, S2Cursor::kBackCel);
		score().doEvent(kScore69);
		break;

	case 6270:
		sound().createAmbient(6);
		enterFrom(18140, 1981, 21802);
		enterFrom(19120, -1, 22102);
		room().drawPan(6270);
		addPanoramaExit(6280, 730, 190, 865, 365);
		addPanoramaExit(6230, 1649, 170, 1840, 470);
		addPanoramaExit(6275, 1329, 224, 1402, 350);
		addPanoramaExit(6279, 133, 162, 298, 315);
		addPanoramaExit(6300, 1000, 155, 1180, 425);
		_pole.reset(new GLScript(self(animatePole)));
		_cafeLight.reset(new GLScript(self(animateCafeLight), 0, 6270));
		break;

	case 6271:
		room().drawPic(6271);
		drawPole(6271);
		enter(roomNo, 22101, 22102, true);
		emplaceExit(true, 19100, 185, 40, 365, 351);
		break;

	case 6272:
		room().drawPic(6272);
		emplaceExit(true, 6279, 64, 0, 104, 383, S2Cursor::kBackCel);
		exitBorder(6279, false, false);
		phone().cancelCall();
		initBarber();
		break;

	case 6273:
		room().drawPic(6273);
		drawPole(6273);
		if (room().getPreviousRoomNo() == 6274) {
			_pole2.reset(new GLScript(self(showPoleNote)));
		}
		emplaceExit(true, 6279, 64, 0, 575, 80, S2Cursor::kBackCel);
		emplaceExit(true, 6279, 64, 81, 144, 250, S2Cursor::kBackCel);
		emplaceExit(true, 6279, 495, 0, 575, 383, S2Cursor::kBackCel);

		emplaceHotspot(true, 346, 290, 430, 344).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
			auto &hotspot = static_cast<S2Hotspot &>(target);
			removeChild(hotspot);
			_pole2.reset(new GLScript(self(showPoleNote), 0, 1));
		});
		break;

	case 6274:
		room().drawPic(6274);
		emplaceExit(true, 6273, S2Cursor::kBackCel);
		sound().play(10604, false, 100);
		score().doEvent(kScore63);
		flags().set(kGameFlag64);
		break;

	case 6275:
		room().drawPic(6275);
		enter(roomNo, 21801, 21802, true);
		emplaceExit(true, 18100, 112, 63, 282, 374);
		break;

	case 6276:
		room().drawPic(6279);
		emplaceExit(true, 6279, S2Cursor::kBackCel);
		score().doEvent(kScore66);
		break;

	case 6279:
		room().drawPic(6271);
		drawPole(6271);
		if (flags().get(kGameFlag134)) {
			emplaceExit(true, 6271, 185, 40, 365, 351, S2Cursor::kHighlightCel);
		} else {
			emplaceExit(true, 6272, 112, 248, 177, 275, S2Cursor::kHighlightCel);
		}

		emplaceExit(true, 6270, 64, 0, 575, 80, S2Cursor::kBackCel);
		emplaceExit(true, 6270, 145, 303, 494, 383, S2Cursor::kBackCel);

		if (flags().get(kGameFlag75)) {
			emplaceExit(true, 6273, 494, 98, 518, 279, S2Cursor::kHighlightCel);
		}

		emplaceExit(true, 6276, 83, 141, 127, 229, S2Cursor::kHighlightCel);
		break;

	case 6280:
		enterFrom(6410, 1641, 0);
		room().drawPan(6280);
		addPanoramaExit(6300, 1310, 170, 1465, 375);
		addPanoramaExit(6270, 1770, 227, 1949, 403);
		addPanoramaExit(6410, 751, 173, 849, 365);
		break;

	case 6290:
		sound().createAmbient(6);
		enterFrom(22110, 1765, 12216);
		room().drawPan(6290);
		addPanoramaExit(6300, 362, 163, 480, 400);
		addPanoramaExit(6250, 1809, 147, 1969, 291);
		addPanoramaExit(6291, 813, 249, 876, 329, S2Cursor::kHighlightCel);
		break;

	case 6291:
		room().drawPic(6291);
		emplaceExit(true, 6999, S2Cursor::kBackCel);
		if (flags().get(kGameFlag136)) {
			enter(roomNo, 12215, 12216, false);
			emplaceExit(true, 22100, 191, 54, 385, 363);
		} else {
			emplaceExit(true, 6292, 320, 142, 374, 188, S2Cursor::kHighlightCel);
		}
		break;

	case 6292:
		room().drawPic(6292);
		phone().cancelCall();
		emplaceExit(true, 6290, 64, 0, 99, 382, S2Cursor::kBackCel);
		emplaceExit(true, 6290, 100, 0, 507, 33, S2Cursor::kBackCel);
		emplaceExit(true, 6290, 508, 0, 575, 383, S2Cursor::kBackCel);
		initWarehouse();
		break;

	case 6300:
		room().drawPan(6300);
		addPanoramaExit(6290, 1328, 185, 1495, 364);
		addPanoramaExit(6280, 278, 236, 547, 401);
		addPanoramaExit(6270, 0, 160, 140, 410);
		_pole.reset(new GLScript(self(animatePole), 3));
		break;

	case 6310:
		sound().createAmbient(7);
		enterFrom(11101, 1712, 11003);
		enterFrom(12110, -1, 11202);
		enterFrom(10110, -1, 11003);
		room().drawPan(6310);
		addPanoramaExit(6110, 181, 156, 377, 373);
		addPanoramaExit(6311, 1136, 243, 1203, 323);
		addPanoramaExit(6312, 1505, 247, 1553, 315);
		addPanoramaExit(6313, 1711, 250, 1756, 315);
		addPanoramaExit(6314, 1555, 273, 1603, 303, S2Cursor::kHighlightCel);
		_fan.reset(new GLScript(self(animateFan), 6));
		break;

	case 6311:
		room().drawPic(6311);
		enter(roomNo, 11111, 11112, true);
		emplaceExit(true, 11100, 256, 110, 411, 383);
		break;

	case 6312:
		room().drawPic(6312);
		enter(roomNo, 11004, 11003, true);
		emplaceExit(true, 10100, 194, 156, 345, 383);
		break;

	case 6313:
		room().drawPic(6313);
		enter(roomNo, 11201, 11202, true);
		emplaceExit(true, 12100, 242, 141, 394, 380);
		break;

	case 6314:
		room().drawPic(6314);
		emplaceExit(true, 6310, S2Cursor::kBackCel);
		break;

	case 6315:
		room().drawPic(6315);
		emplaceExit(true, 6110, S2Cursor::kBackCel);
		setScript(self(showMotelSign));
		break;

	case 6320:
		sound().createAmbient(9);
		flags().set(kGameFlag89);
		enterFrom(6231, -1, 11536);
		room().drawPan(6320);
		addPanoramaExit(6321, 650, 208, 825, 344);
		addPanoramaExit(6350, 1317, 279, 1492, 484);
		addPanoramaExit(6330, 1899, 259, 2047, 469);
		addPanoramaExit(6322, 1672, 290, 1734, 434, S2Cursor::kHighlightCel);
		break;

	case 6321:
		room().drawPic(6321);
		enter(roomNo, 11535, 11536, true);
		emplaceExit(true, 6230, 195, 85, 481, 353);
		break;

	case 6322:
		room().drawPic(6322);
		emplaceExit(true, 6320, S2Cursor::kBackCel);
		break;

	case 6330:
		if (flags().get(kGameFlag28) && !flags().get(kGameFlag23)) {
			flags().set(kGameFlag23);
			room().drawPic(2);
			movie().play(5050, nullptr, roomTop);
			room().getPanorama().setPanX(1814);
		}
		room().drawPan(6330);
		addPanoramaExit(6320, 787, 227, 946, 416);
		addPanoramaExit(6360, 214, 219, 451, 382);
		addPanoramaExit(6340, 1367, 235, 1555, 505);
		addPanoramaExit(6331, 1786, 301, 1860, 377, S2Cursor::kHighlightCel);
		break;

	case 6331:
		room().drawPic(6331);
		emplaceExit(true, 6330, S2Cursor::kBackCel);
		break;

	case 6340:
		room().drawPan(6340);
		addPanoramaExit(6330, 3, 270, 391, 493);
		addPanoramaExit(6350, 876, 269, 1183, 486);
		addPanoramaExit(6341, 2023, 303, 2047, 321, S2Cursor::kHighlightCel);
		addPanoramaExit(6341, 0, 303, 18, 321, S2Cursor::kHighlightCel);
		break;

	case 6341:
		room().drawPic(6341);
		emplaceExit(true, 6340, S2Cursor::kBackCel);
		score().doEvent(kScore164);
		break;

	case 6350:
		if (room().getPreviousRoomNo() == 6340 && !flags().get(kGameFlag24)) {
			flags().set(kGameFlag24);
			room().drawPic(2);
			movie().play(5060, nullptr, roomTop);
			room().getPanorama().setPanX(62);
		}
		room().drawPan(6350);
		addPanoramaExit(6320, 137, 403, 219, 511);
		addPanoramaExit(6320, 222, 289, 376, 511);
		addPanoramaExit(6320, 377, 403, 440, 511);
		addPanoramaExit(6340, 1516, 271, 1812, 491);
		addPanoramaExit(6354, 843, 206, 1029, 498);

		if (inventory().isUsed(S2Inventory::kInv23)) {
			emplaceSprite(false, 6351, GLPoint(781, 275));
		}

		break;

	case 6351:
		room().drawPic(6351);
		emplaceExit(true, 6355, 64, 0, 575, 80, S2Cursor::kBackCel);
		emplaceExit(true, 6355, 64, 81, 144, 250, S2Cursor::kBackCel);
		phone().cancelCall();
		initToes();
		break;

	case 6353: {
		room().drawPic(6353);
		emplaceExit(true, roomNo, 64, 0, 575, 80, S2Cursor::kBackCel);
		emplaceExit(true, roomNo, 64, 340, 575, 383, S2Cursor::kBackCel);

		GLCel *cel;
		if (inventory().isPlaced(S2Inventory::kInv32)) {
			cel = &emplaceCel(false, 6353, 0, 0, roomBottom);
			emplaceHotspot(true, 344, 134, 385, 301).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
				if (!inventory().setState(S2Inventory::kInv32, S2InventoryState::Taken)) {
					return;
				}

				auto &hotspot = static_cast<S2Hotspot &>(target);
				score().doEvent(kScore163);
				removeChild(hotspot);
				sound().play(12902, false, 100);
				inventory().addItem(S2Inventory::kInv32);
				cel->setLoop(1, true);
			});
		} else {
			cel = &emplaceCel(true, 6353, 1, 0, roomBottom);
		}
		cel->show();
		getPlane().getCast().remove(*cel);

		emplaceHotspot(true, 95, 109, 297, 292).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
			removeChild(static_cast<S2Hotspot &>(target));
			flags().set(kGameFlag65);
			sound().play(12105, false, 80);
			cel->setCel(1, true);
		});

		break;
	}

	case 6354:
		room().drawPic(6350);
		emplaceExit(true, 6999, 64, 0, 575, 80, S2Cursor::kBackCel);
		emplaceExit(true, 6999, 64, 340, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 6355, 259, 210, 339, 270, S2Cursor::kHighlightCel);

		if (inventory().isUsed(S2Inventory::kInv23)) {
			emplaceCel(false, 6350, 2, 0, roomBottom).show();
		}

		if (room().getPreviousRoomNo() == 6353) {
			setScript(self(openStatue));
		} else if (inventory().isUsed(S2Inventory::kInv23) && flags().get(kGameFlag219)) {
			setScript(self(openStatue), 0, 1);
		}
		break;

	case 6355:
		room().drawPic(6355);
		emplaceExit(true, 6354, S2Cursor::kBackCel);
		emplaceExit(true, 6351, 219, 105, 399, 195, S2Cursor::kHighlightCel);

		if (inventory().isUsed(S2Inventory::kInv23)) {
			emplaceCel(false, 6355, 0, 0, roomBottom).show();
		} else {
			emplaceHotspot(true, 308, 205, 366, 293).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
				if (!inventory().isInUse(S2Inventory::kInv23)) {
					return;
				}

				inventory().setState(S2Inventory::kInv23, S2InventoryState::Used);
				sound().play(11803, false, 100);
				auto &cel = emplaceCel(false, 6355, 0, 0, roomBottom);
				cel.show();
				getPlane().getCast().remove(cel);

				if (flags().get(kGameFlag219)) {
					room().setNextRoomNo(6354);
				}

				removeChild(static_cast<S2Hotspot &>(target));
			});
		}

		break;

	case 6357:
		room().drawPic(6357);
		emplaceExit(true, 6370, S2Cursor::kBackCel);
		break;

	case 6360:
		enterFrom(6330, 1355, 0);
		enterFrom(6370, 554, 0);
		room().drawPan(6360);
		if (flags().get(kGameFlag218)) {
			addPanoramaExit(6370, 1406, 242, 1459, 319);
			// SSCI checked to see if there was a sprite already and did not add
			// the sprite, but then deleted the sprite unconditionally when the
			// room was disposed, so the check is omitted
			emplaceSprite(false, 6361, GLPoint(1390, 224));
		}
		addPanoramaExit(6330, 748, 309, 994, 504);
		addPanoramaExit(6372, 1471, 177, 1561, 373);
		break;

	case 6370:
		room().drawPan(6370);
		addPanoramaExit(6360, 907, 171, 1155, 500);
		if (inventory().isPlaced(S2Inventory::kInv33)) {
			emplaceSprite(false, 6372, GLPoint(652, 433));
			addPanoramaExit(6373, 645, 426, 705, 465, S2Cursor::kHighlightCel);
		}
		addPanoramaExit(6357, 107, 241, 158, 299, S2Cursor::kHighlightCel);
		break;

	case 6371:
		room().drawPic(6371);
		emplaceExit(true, 6375, 64, 0, 99, 180, S2Cursor::kBackCel);
		emplaceExit(true, 6375, 544, 0, 575, 180, S2Cursor::kBackCel);
		phone().cancelCall();
		initMausoleum();
		break;

	case 6372:
		room().drawPic(6372);
		if (flags().get(kGameFlag218)) {
			auto &cel = emplaceCel(false, 6374, 0, 0, roomTop);
			cel.show();
			getPlane().getCast().remove(cel);
		}
		exitBorder(6360, false);
		emplaceExit(true, 6375, 255, 0, 392, 383);
		break;

	case 6373:
		room().drawPan(6370);
		if (inventory().isTaken(S2Inventory::kInv33)) {
			flags().set(kGameFlag39);
			inventory().addItem(S2Inventory::kInv33);
			// TODO: A sprite was deleted here, figure out where it was created
			// originally and record it
			sound().play(12506, false, 100);
			room().newRoom(6370);
		} else {
			emplaceSprite(false, 6371, GLPoint(587, 388));
			addPanoramaExit(6360, 907, 171, 1155, 500);
		}
		break;

	case 6375:
		enterFrom(6372, 1833, 0);
		room().drawPan(6375);
		addPanoramaExit(6360, 926, 202, 1109, 507);
		addPanoramaExit(6371, 0, 251, 60, 295, S2Cursor::kHighlightCel);
		addPanoramaExit(6371, 1971, 251, 2047, 295, S2Cursor::kHighlightCel);
		addPanoramaExit(6376, 0, 185, 48, 245, S2Cursor::kHighlightCel);
		addPanoramaExit(6376, 1971, 185, 2047, 245, S2Cursor::kHighlightCel);
		break;

	case 6376:
		room().drawPic(6375);
		emplaceExit(true, 6375, S2Cursor::kBackCel);
		score().doEvent(kScore165);
		break;

	case 6380:
		enterFrom(6423, 680, 11536);
		room().drawPan(6380);

		addPanoramaExit(6381, 1708, 152, 1860, 363);
		addPanoramaExit(6170, 857, 211, 1036, 328);
		addPanoramaExit(6382, 1742, 386, 1769, 406, S2Cursor::kHighlightCel);
		break;

	case 6381:
		room().drawPic(6381);
		emplaceExit(true, 6999, S2Cursor::kBackCel);

		// TODO: Should this hotspot only be created when the key is not already
		// used?
		emplaceHotspot(true, 234, 162, 284, 211).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			if (inventory().isInUse(S2Inventory::kInv4) || inventory().isUsed(S2Inventory::kInv4)) {
				inventory().setState(S2Inventory::kInv4, S2InventoryState::Used);
				sound().play(10615, false, 120);
				enter(6381, 11535, 11536, false);
				emplaceExit(true, 6420, 160, 57, 363, 299);
			} else {
				sound().play(10004, false, 120);
			}
		});

		if (inventory().isUsed(S2Inventory::kInv4)) {
			enter(6381, 11535, 11536, false);
			emplaceExit(true, 6420, 160, 57, 363, 299);
		}

		break;

	case 6382: {
		room().drawPic(6381);

		auto loopNo = inventory().isPlaced(S2Inventory::kInv4) ? 1 : 2;
		auto *cel = &emplaceCel(false, 6382, loopNo, 0, roomBottom);
		cel->show();
		getPlane().getCast().remove(*cel);

		cel = &emplaceCel(false, 6382, 0, 0, roomBottom);
		cel->show();
		getPlane().getCast().remove(*cel);

		cel = &emplaceCel(false, 6382, 3, 0, roomBottom);
		cel->show();
		getPlane().getCast().remove(*cel);

		emplaceExit(true, 6380, S2Cursor::kBackCel);
		emplaceHotspot(true, 151, 121, 339, 215).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			setScript(self(openRock), 0, 1);
		});

		break;
	}

	case 6390:
		enterFrom(6400, 1010, 0);
		room().drawPan(6390);
		addPanoramaExit(6210, 1102, 408, 1324, 511);
		addPanoramaExit(6400, 286, 231, 402, 336);
		break;

	case 6400:
		sound().createAmbient(8);
		flags().set(kGameFlag85);
		enterFrom(6390, 1330, 0);
		enterFrom(47801, -1, 12702);
		room().drawPan(6400);
		addPanoramaExit(6390, 428, 215, 526, 335);
		addPanoramaExit(6401, 1124, 331, 1396, 433, S2Cursor::kHighlightCel);

		if (flags().get(kGameFlag132)) {
			addPanoramaExit(47820, 1424, 173, 1630, 361);
			_motor.reset(new GLScript(self(animateMotor)));
		}
		break;

	case 6401: {
		room().drawPic(6401);
		exitBorder(6400);
		int16 celNo;
		if (!flags().get(kGameFlag131)) {
			emplaceHotspot(true, 274, 143, 342, 218).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
				if (!inventory().isInUse(S2Inventory::kInv50)) {
					return;
				}

				inventory().setState(S2Inventory::kInv50, S2InventoryState::Used);
				setScript(self(fillGas));
				removeChild(static_cast<S2Hotspot &>(target));
			});
			celNo = 0;
		} else {
			celNo = 5;
		}

		auto &cel = emplaceCel(false, 6401, 2, celNo, roomBottom);
		cel.show();
		getPlane().getCast().remove(cel);

		if (flags().get(kGameFlag132)) {
			setScript(self(startMotor), 6);
		} else {
			emplaceHotspot(true, 225, 275, 275, 316).setMouseUpHandler([&](GLEvent &, GLTarget &) {
				const int state = inventory().isUsed(S2Inventory::kInv50) ? 0 : 4;
				setScript(self(startMotor), state);
			});
		}

		break;
	}

	case 6410: {
		sound().createAmbient(6);
		enterFrom(24101, 1620, 22402);
		room().drawPan(6410);
		addPanoramaExit(6280, 1548, 377, 1786, 510);
		const int exitNo = inventory().isUsed(S2Inventory::kInv21) ? 6413 : 6411;
		addPanoramaExit(exitNo, 594, 188, 823, 510);
		if (flags().get(kGameFlag0)) {
			emplaceSprite(false, 6411, GLPoint(463, 210));
			addPanoramaExit(6412, 474, 220, 540, 288, S2Cursor::kHighlightCel);
		}
		break;
	}

	case 6411:
		room().drawPic(6411);
		emplaceHotspot(true, 216, 209, 263, 290).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			if (inventory().isInUse(S2Inventory::kInv21) ||
				inventory().isUsed(S2Inventory::kInv21)) {
				inventory().setState(S2Inventory::kInv21, S2InventoryState::Used);
				sound().play(10612, false, 120);
				room().setNextRoomNo(6413);
			} else {
				sound().play(10001, false, 120);
			}
		});
		emplaceExit(true, 6410, S2Cursor::kBackCel);
		break;

	case 6412:
		room().drawPic(6412);
		emplaceExit(true, 6410, S2Cursor::kBackCel);
		score().doEvent(kScore67);
		break;

	case 6413:
		room().drawPic(6411);
		enter(6411, 22401, 22402, true);
		emplaceExit(true, 24100, 233, 99, 444, 380);
		break;

	case 6420: {
		sound().createAmbient(8);
		enterFrom(25710, 927, 22502);
		enterFrom(6381, 1820, 11536);
		room().drawPan(6420);
		addPanoramaExit(6423, 1035, 234, 1195, 319);

		const int exitNo = flags().get(kGameFlag137) ? 6421 : 6422;
		addPanoramaExit(exitNo, 90, 212, 170, 339);

		break;
	}

	case 6421:
		room().drawPic(6421);
		enter(roomNo, 22501, 22502, true);
		emplaceExit(true, 25700, 230, 86, 415, 348);
		break;

	case 6422:
		room().drawPic(6422);
		phone().cancelCall();
		initBook();

		emplaceExit(true, 6420, 64, 0, 575, 33, S2Cursor::kBackCel);
		emplaceExit(true, 6420, 64, 383, 575, 343, S2Cursor::kBackCel);
		break;

	case 6423:
		_enterSoundNo = 11535;
		_exitSoundNo = 11536;
		room().drawPic(6423);
		emplaceCel(false, 6423, 0, 0, roomBottom).show();
		setScript(self(openGate));
		emplaceExit(true, 6999, S2Cursor::kBackCel);
		emplaceExit(true, 6380, 217, 48, 363, 353);
		break;

	case 6540: {
		sound().createAmbient(6);
		const int previousRoomNo = room().getPreviousRoomNo();
		if (previousRoomNo != 6260 && previousRoomNo != 28300 && previousRoomNo != 6667) {
			room().getPanorama().setPanX(1177);
		}
		room().drawPan(6540);
		addPanoramaExit(6260, 288, 171, 517, 392);
		addPanoramaExit(28300, 1306, 32, 1510, 372);
		break;
	}

	case 6666:
		room().drawPic(5000);
		_norah.reset(new GLScript(self(showNorah)));
		break;

	case 6667:
		room().drawPic(500);
		_norah.reset(new GLScript(self(chooseEnemy)));
		break;

	case 6999: {
		const int previousRoomNo = room().getPreviousRoomNo();
		if (previousRoomNo == 6423) {
			setScript(self(openGate), 3);
		} else if (previousRoomNo == 6354 && _cel) {
			setScript(self(openStatue));
		} else if (!_cel) {
			switch (previousRoomNo) {
			case 6121: room().newRoom(6120); break;
			case 6291: room().newRoom(6290); break;
			case 6354: room().newRoom(6350); break;
			case 6381: room().newRoom(6380); break;
			default: error("Stuck in room 6999");
			}
		} else {
			setScript(self(cancelAction));
		}
		break;
	}
	}
}

void S2Room6000::dispose(const int roomNo) {
	_cycler.reset();
	_cycler2.reset();
	if (room().getNextRoomNo() != 6999) {
		_cel.reset();
	}
	_ethereal.reset();
	_fan.reset();
	_sign.reset();
	_flag.reset();
	_birds.reset();
	_cafeLight.reset();
	_pole2.reset();
	_pole.reset();
	_motor.reset();
	_shadow.reset();
	_panoramaCycler1.reset();
	_panoramaCycler.reset();
	_norah.reset();

	S2Room::dispose(roomNo);

	switch (roomNo) {
	case 6400:
		sound().stop(12707);
		break;

	case 6540:
		if (room().getNextRoomNo() == 6260 && !flags().get(kGameFlag20) && inventory().hasPrayerStick()) {
			flags().set(kGameFlag20);
			room().drawPic(2);
			movie().play(5020, nullptr, roomTop);
		}
		break;
	}
}

void S2Room6000::enter(const int roomNo, const uint16 enterSound, const uint16 exitSound, const bool addExit) {
	_enterSoundNo = enterSound;
	_exitSoundNo = exitSound;
	setScript(self(enterScript), 0, roomNo);
	if (addExit) {
		emplaceExit(true, 6999, S2Cursor::kBackCel);
	}
}

void S2Room6000::initBank() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::initBarber() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::initWarehouse() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::initToes() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::initMausoleum() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::initBook() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::enterScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		script.setSeconds(1);
		break;

	case 1:
		_cel.reset(new GLCel(getPlane(), script.getData(), 0, 0, roomBottom));
		_cel->show();
		_cycler.reset(new GLEndCycler());
		_cycler->add(*_cel);
		_cycler->start(script);
		sound().play(_enterSoundNo, false, 100);
		break;

	case 2:
		getPlane().getCast().remove(*_cel);
		_cycler.reset();
		user().setIsHandsOn(true);
		break;
	}
}

void S2Room6000::cancelAction(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		_cycler.reset(new GLEndBackCycler());
		_cycler->add(*_cel);
		_cycler->start(script);
		sound().play(_exitSoundNo, false, 100);
		break;

	case 1:
		_cel.reset();
		script.setSeconds(1);
		break;

	case 2: {
		_script.reset();
		_cycler.reset();
		// SSCI used a big switch here; we do the same thing algorithmically
		room().setNextRoomNo(room().getPreviousRoomNo() / 10 * 10);
		user().setIsHandsOn(true);
		break;
	}
	}
}

void S2Room6000::goToEthereal(GLScript &script, const int state) {
	switch (state) {
	case 0:
		script.setCycles(1);
		break;

	case 1:
		_ethereal.reset();
		room().setNextRoomNo(_choose ? 6667 : 6666);
		_choose = false;
		break;
	}
}

void S2Room6000::animateFan(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_panoramaSprite0 = &emplaceSprite(true, 6101, GLPoint(805, 218), 0, 4);
		_panoramaCycler.reset(new S2PanoramaCycler());
		script.setCycles(1);
		break;

	case 1:
		_panoramaCycler->add(*_panoramaSprite0);
		_panoramaCycler->start(script);
		break;

	case 2:
		script.setState(0);
		script.setCycles(1);
		break;

	case 3:
		_panoramaSprite1 = &emplaceSprite(true, 6111, GLPoint(621, 236), 0, 3);
		_panoramaSprite2 = &emplaceSprite(true, 6112, GLPoint(737, 232), 0, 3);
		_panoramaCycler1.reset(new S2PanoramaCycler());
		script.setCycles(1);
		break;

	case 4:
		_panoramaCycler1->add(*_panoramaSprite1);
		_panoramaCycler1->add(*_panoramaSprite2);
		_panoramaCycler1->start(script);
		break;

	case 5:
		script.setState(3);
		script.setCycles(1);
		break;

	case 6:
		_panoramaSprite0 = &emplaceSprite(true, 6311, GLPoint(488, 238), 0, 3);
		_panoramaCycler.reset(new S2PanoramaCycler());
		script.setCycles(1);
		break;

	case 7:
		_panoramaCycler->add(*_panoramaSprite0);
		_panoramaCycler->start(script);
		break;

	case 8:
		script.setState(6);
		script.setCycles(1);
		break;

	case 9:
		_panoramaSprite1 = &emplaceSprite(true, 6122, GLPoint(608, 247), 0, 3);
		_panoramaCycler1.reset(new S2PanoramaCycler());
		script.setCycles(1);
		break;

	case 10:
		_panoramaCycler1->add(*_panoramaSprite1);
		_panoramaCycler1->start(script);;
		break;

	case 11:
		script.setState(9);
		script.setCycles(1);
		break;
	}
}

void S2Room6000::animateSign(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_panoramaSprite0 = &emplaceSprite(true, 6113, GLPoint(1179, 181), 0, 4);
		_panoramaCycler.reset(new S2PanoramaCycler());
		script.setCycles(1);
		break;

	case 1:
		_panoramaSprite0->setCycleSpeed(_game.getRandomNumber(15, 25));
		_panoramaCycler->add(*_panoramaSprite0);
		_panoramaCycler->start(script);
		break;

	case 2:
		script.setState(0);
		script.setCycles(1);
		break;

	case 3:
		_panoramaSprite1 = &emplaceSprite(true, 6102, GLPoint(1374, 222), 0, 4);
		_panoramaCycler1.reset(new S2PanoramaCycler());
		script.setCycles(1);
		break;

	case 4:
		_panoramaSprite1->setCycleSpeed(_game.getRandomNumber(15, 25));
		_panoramaCycler1->add(*_panoramaSprite1);
		_panoramaCycler1->start(script);
		break;

	case 5:
		script.setState(3);
		script.setCycles(1);
		break;
	}
}

void S2Room6000::animateFlag(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_panoramaSprite0 = &emplaceSprite(true, 6121, GLPoint(1252, 10), 0, 5);
		_panoramaCycler.reset(new S2PanoramaEndForwardBackwardCycler());
		script.setCycles(1);
		break;

	case 1:
		_panoramaSprite0->setCycleSpeed(_game.getRandomNumber(5, 15));
		_panoramaCycler->add(*_panoramaSprite0);
		_panoramaCycler->start(script);
		break;

	case 2:
		script.setState(0);
		script.setCycles(1);
		break;
	}
}

void S2Room6000::animateBirds(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_panoramaSprite0 = &emplaceSprite(true, 5990, GLPoint(500, 5), 37, 38, true);
		_panoramaCycler.reset(new S2PanoramaStartResetCycler());
		script.setCycles(1);
		break;

	case 1:
		_panoramaCycler->add(*_panoramaSprite0);
		_panoramaCycler->start(script);
		break;

	case 2:
		script.setState(0);
		script.setCycles(1);
		break;
	}
}

void S2Room6000::animateCafeLight(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_lightState = 0;
		if (script.getData() == 6270) {
			_panoramaSprite1 = &emplaceSprite(true, 6278, GLPoint(1535, 178));
		} else {
			_panoramaSprite1 = &emplaceSprite(true, 6238, GLPoint(793, 213));
		}
		script.setCycles(1);
		break;

	case 1:
		_panoramaSprite1->show();
		script.setTicks(_lightState + (_lightState == 1 ? 2 : 6));
		break;

	case 2:
		_panoramaSprite1->hide();
		script.setTicks(_lightState + (_lightState == 1 ? 9 : 6));
		break;

	case 3:
		if (++_lightState == 3) {
			script.setState(3);
		} else {
			script.setState(0);
		}
		script.setCycles(1);
		break;

	case 4:
		_panoramaSprite1->hide();
		script.setSeconds(4);
		break;

	case 5:
		_lightState = 0;
		_panoramaSprite1->show();
		script.setState(0);
		script.setSeconds(6);
		break;
	}
}

void S2Room6000::animatePole(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_panoramaSprite0 = &emplaceSprite(true, 6271, GLPoint(262, 230), 0, 11);
		_panoramaCycler.reset(new S2PanoramaCycler());
		script.setCycles(1);
		break;

	case 1:
		_panoramaSprite0->setCycleSpeed(8);
		_panoramaCycler->add(*_panoramaSprite0);
		_panoramaCycler->start(script);
		break;

	case 2:
		script.setState(0);
		script.setCycles(1);
		break;

	case 3:
		_panoramaSprite0 = &emplaceSprite(true, 6301, GLPoint(105, 252), 0, 10);
		_panoramaCycler.reset(new S2PanoramaCycler());
		script.setCycles(1);
		break;

	case 4:
		_panoramaSprite0->setCycleSpeed(8);
		_panoramaCycler->add(*_panoramaSprite0);
		_panoramaCycler->start(script);
		break;

	case 5:
		script.setState(3);
		script.setCycles(1);
		break;
	}
}

void S2Room6000::animateMotor(GLScript &, const int) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::animateShadow(GLScript &script, const int state) {
	switch (state) {
	case 0:
		room().getPanorama().removeAllExits();
		_panoramaSprite0 = &emplaceSprite(true, 6195, GLPoint(1056, 67), 0, 10, true);
		_panoramaCycler.reset(new S2PanoramaEndForwardCycler());
		_panoramaSprite0->setCycleSpeed(10);
		_panoramaCycler->add(*_panoramaSprite0);
		_panoramaCycler->start(script);
		break;

	case 1:
		_panoramaCycler.reset();
		removeChild(*_panoramaSprite0);
		addShadowRoomExits();
		_shadow.reset();
		break;
	}
}

// In SSCI, this was a script, for no reason (it had only one state)
void S2Room6000::drawPole(const int roomNo) {
	user().setIsHandsOn(false);
	const auto loopNo = (roomNo == 6271 ? 1 : 0);
	auto &cel = emplaceCel(false, roomNo, loopNo, 0, roomBottom);
	cel.show();
	getPlane().getCast().remove(cel);
	_cycler2.reset(new GLCycler());
	_cycler2->add(cel);
	user().setIsHandsOn(true);
	_cycler2->start();
}

void S2Room6000::showPoleNote(GLScript &, const int) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::showShadow(GLScript &script, const int state) {
	switch (state) {
	case 0:
		if (room().getPanorama().panX() > 605 && room().getPanorama().panX() < 1019) {
			flags().set(kGameFlag138);
			_shadow.reset(new GLScript(self(animateShadow)));
			script.setState(1);
			script.setCycles(1);
		} else {
			script.setSeconds(1);
		}
		break;

	case 1:
		script.setState(-1);
		script.setCycles(1);
		break;

	case 2:
		_script.reset();
		break;
	}
}

void S2Room6000::addShadowRoomExits() {
	addPanoramaExit(6200, 1744, 201, 1977, 337);
	addPanoramaExit(6180, 650, 186, 887, 348);
	addPanoramaExit(6191, 259, 218, 307, 299);
	addPanoramaExit(6192, 1103, 214, 1165, 309, S2Cursor::kHighlightCel);
}

void S2Room6000::useGasPump(GLScript &, const int) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::prySiding(GLScript &, const int) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::openKeyBox(GLScript &, const int) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::showMotelSign(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		script.setSeconds(1);
		break;

	case 1: {
		const int16 loopNo = _game.getRandomNumber(0, 1);
		_cel.reset(new GLCel(getPlane(), 6315, loopNo, 0, roomBottom));
		_cel->show();
		script.setSeconds(1);
		break;
	}

	case 2:
		_cel.reset();
		_script.reset();
		user().setIsHandsOn(true);
		break;
	}
}

void S2Room6000::openStatue(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		int16 celNo;
		GLCycler *cycler;
		uint16 soundNo;
		if (script.getData()) {
			celNo = 0;
			cycler = new GLEndCycler();
			soundNo = 12605;
		} else {
			celNo = 5;
			cycler = new GLEndBackCycler();
			soundNo = 12606;
		}
		_cel.reset(new GLCel(getPlane(), 6350, 0, celNo, roomBottom));
		_cel->show();
		_cycler.reset(cycler);
		sound().play(soundNo, false, 80);
		_cycler->add(*_cel);
		_cycler->start(script);
		break;

	case 1:
		getPlane().getCast().remove(*_cel);
		if (script.getData()) {
			emplaceHotspot(true, 284, 287, 374, 336).setMouseUpHandler([&](GLEvent &, GLTarget &target) {
				removeChild(static_cast<S2Hotspot &>(target));
				room().newRoom(6353);
			});
			_script.reset();
			_cycler.reset();
			user().setIsHandsOn(true);
		} else {
			_cel.reset();
			// SSCI had an extra unnecessary cue state and immediate cue call
			// here
			script.setSeconds(1);
		}
		break;

	case 2:
		room().setNextRoomNo(6350);
		_script.reset();
		_cycler.reset();
		user().setIsHandsOn(true);
		break;
	}
}

void S2Room6000::openRock(GLScript &, const int) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::fillGas(GLScript &, const int) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::startMotor(GLScript &, const int) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::openGate(GLScript &, const int) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Room6000::showNorah(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		_cel.reset(new GLCel(getPlane(), 5001, 0, 0, { 250, 322 }));
		_cel->show();
		_cycler.reset(new GLEndCycler());
		_cycler->add(*_cel);
		_cycler->start(script);
		interface().putText(room().getNorahSoundNo());
		// SSCI used a hard-coded duration table to get the length of the sound;
		// we get it from the sound itself
		_norahDuration = sound().play(room().getNorahSoundNo(), false, 100);
		break;

	case 1:
		_cel.reset(new GLCel(getPlane(), _game.getRandomNumber(5003, 5006), 0, 0, { 250, 322 }));
		_cel->setCycleSpeed(10);
		_cel->show();
		_cycler.reset(new GLEndForwardBackwardCycler());
		_cycler->add(*_cel);
		_cycler->start(script);
		break;

	case 2: {
		const auto position = sound().getPosition(room().getNorahSoundNo());
		if (_norahDuration - 275 > position || position < 0) {
			script.setState(0);
		}

		script.setCycles(1);
		break;
	}

	case 3:
		_cel.reset(new GLCel(getPlane(), 5002, 0, 0, { 250, 322 }));
		_cel->show();
		_cycler.reset(new GLEndCycler());
		_cycler->add(*_cel);
		_cycler->start(script);
		break;

	case 4:
		user().setIsHandsOn(true);
		_cycler.reset();
		_cel.reset();
		room().setNextRoomNo(room().getNorahNextRoomNo());
		_script.reset();
		break;
	}
}

void S2Room6000::chooseEnemy(GLScript &, const int) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

} // End of namespace Sci
