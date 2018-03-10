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

#include "sci/s2/rooms/28000.h"

namespace Sci {

#define self(name) this, &S2TangramPuzzle::name

class S2TangramPuzzle : public S2SubRoom {
	enum { kNumPuzzles = 12, kMaxPieces = 10, kGridSize = 12 };
	using Grid = Common::FixedArray<Common::FixedArray<int8, kGridSize>, kGridSize>;
	struct Puzzle {
		S2PrayerStick stick;
		int exitNo;
		GLPoint origin;
		int numPieces;
		int8 pieces[kMaxPieces];
		GLPoint positions[kMaxPieces];
		int8 defaults[kGridSize][kGridSize];
	};

	class TangramCel : public S2TransparentCel {
		enum { kNumShapes = 32, kSideSize = 6 };
		using Shape = int8[kSideSize][kSideSize];

	public:
		TangramCel(AbsGLPlane &plane, const int16 pieceNo, const GLPoint &origin, S2TangramPuzzle &parent) :
			S2TransparentCel(plane, 48201, pieceNo, 0, GLPoint(0, 0)),
			_origin(origin),
			_parent(parent),
			_size(_sizes[pieceNo]) {
			// SSCI called needsDoIt but this is already set for GLCels

			// In SSCI, these included another GLPoint, but its value was always
			// 0,0 so this is omitted
			_column = (2 - _size * 20 / 2 - origin.x) / 20;
			_row = (2 - _size * 20 / 2 - origin.y) / 20;

			for (auto y = 0; y < kSideSize; ++y) {
				auto value = _shapes[pieceNo][y];
				for (auto x = kSideSize - 1; x >= 0; --x) {
					if (y > _size) {
						_shape[y][x] = 0;
					} else {
						_shape[y][x] = value % 8;
						value /= 8;
					}
				}
			}
		}

		void setGridPosition(const int row, const int column, const bool shouldUpdate) {
			_row = row;
			_column = column;
			setPositionFromGrid(row, column, shouldUpdate);
		}

		void setPositionFromGrid(const int row, const int column, const bool shouldUpdate) {
			GLPoint newPosition;
			newPosition.x = column * 20 + (_size * 20 / 2) - 2;
			newPosition.y = row * 20 + (_size * 20 / 2) - 2;
			newPosition += _origin;
			setPosition(newPosition, shouldUpdate);
		}

		void rotateRight() {
			rotateShape<true>();
		}

		void rotateLeft() {
			rotateShape<false>();
		}

		void drop() {
			drop(_row, _column);
		}

		virtual void doIt() override {
			if (!_isActive) {
				return;
			}

			const auto mousePosition = _parent.user().getMousePosition();
			if (_isMoved) {
				int newColumn, newRow;
				calculateRowColumn(mousePosition, newRow, newColumn);
				setPositionFromGrid(newRow, newColumn, true);
			} else {
				_isMoved = (ABS(mousePosition.x - _mouseGlobal.x) > 20 ||
							ABS(mousePosition.y - _mouseGlobal.y) > 20);
			}
		}

		virtual bool handleEvent(GLEvent &event) override {
			// SSCI did not inherit from S2TransparentCel, instead it had a
			// duplicate of the transparent isOnMe check
			if (!checkIsOnMe(event.getMousePosition()) ||
				!(event.getType() & kSciEventMouse)) {
				return event.isClaimed();
			}

			if (event.getType() == kSciEventMousePress) {
				_isActive = true;
				event.claim();
				_mouseGlobal = event.getMousePosition();
				_mouseLocal = _mouseGlobal - getPosition();
				setPriority(400);
				_parent.cursor().hide();
				_isMoved = false;
			} else if (_isActive) {
				setPriority(5);
				_parent.cursor().show();
				event.claim();
				_isActive = false;
				// SSCI mutated the passed event's mouse position directly
				int newColumn, newRow;
				calculateRowColumn(event.getMousePosition(), newRow, newColumn);
				if (_isMoved) {
					lift(_row, _column);
					if (checkCanDrop(newRow, newColumn)) {
						_row = newRow;
						_column = newColumn;
						drop();
						// SSCI used a callback, but all the callbacks do the
						// same thing, so we just call back directly instead
						_parent.checkFinished();
					} else {
						drop();
					}

					_isMoved = false;
					setPositionFromGrid(_row, _column, true);
				} else {
					setPositionFromGrid(_row, _column, false);
					lift(_row, _column);
					rotateRight();
					if (checkCanDrop(_row, _column)) {
						drop(_row, _column);
						forceUpdate();
					} else {
						_parent.cancelRotate(*this);
					}
				}
			}

			return event.isClaimed();
		}

	private:
		void calculateRowColumn(GLPoint position, int &row, int &column) {
			const auto newOrigin = position - _mouseLocal;
			column = (2 + (newOrigin.x - _size * 20 / 2) - _origin.x) / 20;
			row = (2 + (newOrigin.y - _size * 20 / 2) - _origin.y) / 20;
		}

		void lift(const int row, const int column) {
			if (!intersectsGrid(row, column)) {
				return;
			}
			for (auto y = 0; y < _size; ++y) {
				for (auto x = 0; x < _size; ++x) {
					const auto gridX = column + x;
					const auto gridY = row + y;

					if (isInGrid(gridY, gridX)) {
						auto &target = _parent._grid[gridY][gridX];
						target -= _shape[y][x];
					}
				}
			}
		}

		bool checkCanDrop(const int row, const int column) {
			if (!intersectsGrid(row, column)) {
				return true;
			}

			bool hit = false;
			for (auto y = 0; y < _size; ++y) {
				for (auto x = 0; x < _size; ++x) {
					const auto gridX = column + x;
					const auto gridY = row + y;

					if (isInGrid(gridY, gridX)) {
						if (_shape[y][x]) {
							hit = true;

							const auto value = _parent._grid[gridY][gridX];
							if (value != 0 && value + _shape[y][x] != 5) {
								return false;
							}
						}
					} else if (_shape[y][x] && hit) {
						return false;
					}
				}
			}

			return true;
		}

		void drop(const int row, const int column) {
			_parent.sound().play(12126, false, 100);
			if (!intersectsGrid(row, column)) {
				return;
			}
			for (auto y = 0; y < _size; ++y) {
				for (auto x = 0; x < _size; ++x) {
					const auto gridX = column + x;
					const auto gridY = row + y;

					if (isInGrid(gridY, gridX)) {
						auto &target = _parent._grid[gridY][gridX];
						target += _shape[y][x];
					}
				}
			}
		}

		bool intersectsGrid(const int row, const int column) {
			return (isInGrid(row, column) ||
					isInGrid(row + _size, column) ||
					isInGrid(row + _size, column + _size) ||
					isInGrid(row, column + _size));
		}

		bool isInGrid(const int row, const int column) {
			return row >= 0 && row < 12 && column >= 0 && column < 12;
		}

		template <bool RIGHT>
		void rotateShape() {
			_celNo = (_celNo + (RIGHT ? 1 : 3)) % 4;
			Shape temp;
			for (auto y = 0; y < _size; ++y) {
				for (auto x = 0; x < _size; ++x) {
					auto &target = temp[y][x];
					if (RIGHT) {
						target = _shape[_size - 1 - x][y];
					} else {
						target = _shape[x][_size - 1 - y];
					}
					if (target && target < 5) {
						target = ((RIGHT ? 2 : 3) * target) % 5;
					}
				}
			}
			for (auto y = 0; y < _size; ++y) {
				Common::copy(temp[y], temp[y] + _size, _shape[y]);
			}
			setCel(_celNo);
		}

		static constexpr uint32 _shapes[kNumShapes][kSideSize] = {
			{ 0004000,
			  0045000,
			  0051000,
			  0010000,
			  0000000,
			  0000000 },

			{ 0000000,
			  0043000,
			  0455300,
			  0000000,
			  0000000,
			  0000000 },

			{ 0000000,
			  0004300,
			  0045530,
			  0455553,
			  0000000,
			  0000000 },

			{ 0000000,
			  0555500,
			  0555500,
			  0000000,
			  0000000,
			  0000000 },

			{ 0043000,
			  0455300,
			  0255530,
			  0025510,
			  0002100,
			  0000000 },

			{ 0030000,
			  0053000,
			  0055000,
			  0055000,
			  0000000,
			  0000000 },

			{ 0003000,
			  0005300,
			  0005530,
			  0005553,
			  0000000,
			  0000000 },

			{ 0030000,
			  0053000,
			  0055300,
			  0055530,
			  0025510,
			  0002100 },

			{ 0000430,
			  0004553,
			  0045551,
			  0455510,
			  0255100,
			  0021000 },

			{ 0055000,
			  0055000,
			  0051000,
			  0010000,
			  0000000,
			  0000000 },

			{ 0000000,
			  0000000,
			  0025530,
			  0002553,
			  0000000,
			  0000000 },

			{ 0003000,
			  0005300,
			  0005500,
			  0002500,
			  0000200,
			  0000000 },

			{ 0003000,
			  0005300,
			  0005530,
			  0002553,
			  0000000,
			  0000000 },

			{ 0004300,
			  0045530,
			  0455553,
			  0255551,
			  0025510,
			  0002100 },

			{ 0555000,
			  0555000,
			  0555000,
			  0000000,
			  0000000,
			  0000000 },

			{ 0030000,
			  0053000,
			  0055300,
			  0000000,
			  0000000,
			  0000000 },

			{ 0000000,
			  0004555,
			  0045555,
			  0455555,
			  0000000,
			  0000000 },

			{ 0043000,
			  0455300,
			  0255100,
			  0021000,
			  0000000,
			  0000000 },

			{ 0000000,
			  0000000,
			  0045551,
			  0455510,
			  0000000,
			  0000000 },

			{ 0000000,
			  0000000,
			  0255530,
			  0025553,
			  0000000,
			  0000000 },

			{ 0550000,
			  0550000,
			  0000000,
			  0000000,
			  0000000,
			  0000000 },

			{ 0430000,
			  0253000,
			  0025000,
			  0002000,
			  0000000,
			  0000000 },

			{ 0430000,
			  0253000,
			  0025300,
			  0000000,
			  0000000,
			  0000000 },

			{ 0030000,
			  0053000,
			  0025300,
			  0002530,
			  0000250,
			  0000020 },

			{ 0043000,
			  0455300,
			  0255530,
			  0025553,
			  0000000,
			  0000000 },

			{ 0000000,
			  0000451,
			  0004510,
			  0005100,
			  0001000,
			  0000000 },

			{ 0000000,
			  0000000,
			  0045530,
			  0455553,
			  0000000,
			  0000000 },

			{ 0555500,
			  0555500,
			  0555500,
			  0555500,
			  0000000,
			  0000000 },

			{ 0000000,
			  0253000,
			  0025300,
			  0002530,
			  0000253,
			  0000000 },

			{ 0055000,
			  0055000,
			  0055550,
			  0055550,
			  0055550,
			  0055550 },

			{ 0030000,
			  0053000,
			  0055300,
			  0055530,
			  0055550,
			  0055550 },

			{ 0055000,
			  0055000,
			  0055550,
			  0055550,
			  0000000,
			  0000000 }
		};

		static constexpr int8 _sizes[kNumShapes] = {
			4, 4, 6, 4, 6, 4, 6, 6,
			6, 4, 6, 6, 6, 6, 4, 4,
			6, 4, 6, 6, 2, 4, 4, 6,
			6, 6, 6, 4, 6, 6, 6, 6
		};

		Shape _shape;
		bool _isActive = false;
		bool _isMoved = false;
		int16 _celNo = 0;
		int _row, _column;
		int _size;
		GLPoint _origin;
		GLPoint _mouseGlobal;
		GLPoint _mouseLocal;
		S2TangramPuzzle &_parent;
	};

public:
	using S2SubRoom::S2SubRoom;
	virtual void init(const int roomNo) override {
		room().drawPic(roomNo);
		_index = roomNo - 48201;

		auto &puzzle = _puzzles[_index];
		for (auto y = 0; y < kGridSize; ++y) {
			Common::copy(puzzle.defaults[y], puzzle.defaults[y] + kGridSize, _grid[y].data());
		}

		for (auto i = 0; i < puzzle.numPieces; ++i) {
			_pieces[i].reset(new TangramCel(getPlane(), puzzle.pieces[i], puzzle.origin, *this));
			_pieces[i]->setGridPosition(puzzle.positions[i].y, puzzle.positions[i].x, true);
			auto max = _game.getRandomNumber(0, 3);
			for (auto j = 0; j < max; ++j) {
				_pieces[i]->rotateRight();
			}
			_pieces[i]->show();
			_pieces[i]->forceUpdate();
		}

		emplaceExit(true, puzzle.exitNo, puzzle.origin.x, 0, puzzle.origin.x + 240, puzzle.origin.y, S2Cursor::kBackCel);
	}

	void cancelRotate(TangramCel &cel) {
		user().setIsHandsOn(false);
		setScript(self(cancelRotateScript), 0, 0, &cel);
	}

private:
	void cancelRotateScript(GLScript &script, const int state) {
		auto &cel = *static_cast<TangramCel *>(script.getDataPointer());
		switch (state) {
		case 0:
			cel.forceUpdate();
			script.setTicks(30);
			break;
		case 1:
			sound().play(12126, false, 100);
			cel.rotateLeft();
			cel.drop();
			cel.forceUpdate();
			_script.reset();
			user().setIsHandsOn(true);
		}
	}

	void checkFinished() {
		for (auto y = 0; y < kGridSize; ++y) {
			for (auto x = 0; x < kGridSize; ++x) {
				if (_grid[y][x] != 5) {
					return;
				}
			}
		}

		flags().set(GameFlag(205 + _index));
		score().doEvent(S2Score(230 + _index));
		user().setIsHandsOn(false);
		setScript(self(finishedScript));
	}

	void finishedScript(GLScript &script, const int state) {
		switch (state) {
		case 0:
			script.setTicks(90);
			break;

		case 1:
			room().setNextRoomNo(_puzzles[_index].exitNo);
			user().setIsHandsOn(true);
			_script.reset();
			break;
		}
	}

	int _index;
	Grid _grid;
	Common::FixedArray<Common::ScopedPtr<TangramCel>, kMaxPieces> _pieces;

	static constexpr Puzzle _puzzles[kNumPuzzles] = {
		{
			S2PrayerStick::Claw,
			48000,
			{ 201, 76 },
			10,
			{ 0, 1, 2, 3,
			  4, 5, 6, 6,
			  7, 2 },
			{ { 12, -3 }, { -6,  9 }, { 12, 0 }, { -6, 6 },
			  { 12,  3 }, { -6,  3 }, { 12, 6 }, { -6, 0 },
			  { 12,  9 }, { -6, -3 } },
			{ { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 5, 2, 5, 5, 5, 1, 2, 5, 5, 5, 1, 5 },
			  { 5, 0, 2, 5, 1, 0, 0, 2, 5, 1, 0, 5 },
			  { 5, 0, 0, 5, 0, 0, 0, 0, 5, 0, 0, 5 },
			  { 5, 0, 0, 5, 0, 0, 0, 0, 5, 0, 0, 5 },
			  { 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5 },
			  { 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5 },
			  { 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5 },
			  { 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5 },
			  { 5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5 },
			  { 5, 5, 3, 0, 0, 0, 0, 0, 0, 4, 5, 5 },
			  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 } }
		}, {
			S2PrayerStick::Hourglass,
			28400,
			{ 204, 76 },
			9,
			{ 24, 1, 1, 8,
			  12, 10, 11, 12,
			  2 },
			{ { 12, -3 }, { -5, -3 }, { -5, 0 }, { 12, 1 },
			  { -6,  3 }, { 12,  5 }, { -6, 6 }, { 12, 9 },
			  { -6,  9 } },
			{ { 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4 },
			  { 5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5 },
			  { 5, 5, 3, 0, 0, 0, 0, 0, 0, 4, 5, 5 },
			  { 5, 5, 5, 3, 0, 0, 0, 0, 4, 5, 5, 5 },
			  { 5, 5, 5, 5, 3, 0, 0, 4, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 3, 4, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 1, 2, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 1, 0, 0, 2, 5, 5, 5, 5 },
			  { 5, 5, 5, 1, 0, 0, 0, 0, 2, 5, 5, 5 },
			  { 5, 5, 1, 0, 0, 0, 0, 0, 0, 2, 5, 5 },
			  { 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 5 },
			  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 } }
		}, {
			S2PrayerStick::Stairs,
			28700,
			{ 200, 76 },
			10,
			{ 14,  2,  2, 15,
			  12, 10, 15, 13,
			   2, 16 },
			{ { 12, -3 }, { -6,  9 }, { 12, 0 }, { -6, 6 },
			  { 12,  3 }, { -6,  3 }, { 12, 6 }, { -6, 0 },
			  { 12,  9 }, { -6, -3 } },
			{ { 0, 0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 0, 0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 0, 0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 0, 0, 0, 0, 0, 0, 5, 5, 5, 5, 5, 5 },
			  { 0, 0, 0, 0, 0, 0, 5, 5, 5, 5, 5, 5 },
			  { 0, 0, 0, 0, 0, 0, 5, 5, 5, 5, 5, 5 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 5 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 5 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 5 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
		}, {
			S2PrayerStick::Spider,
			28700,
			{ 201, 81 },
			8,
			{ 19, 3, 18, 19,
			  27, 26, 0, 24 },
			{ { 12, -3 }, { 12, 1 }, { -6, 9 }, { 12,  5 },
			  { -6,  5 }, { 12, 9 }, { -6, 1 }, { -6, -3 } },
			{ { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 1, 0, 0, 0, 0, 2, 5, 5, 5 },
			  { 5, 5, 1, 0, 0, 0, 0, 0, 0, 2, 5, 5 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 5, 5, 3, 0, 0, 0, 0, 0, 0, 4, 5, 5 },
			  { 5, 5, 5, 3, 0, 0, 0, 0, 4, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 } }
		}, {
			S2PrayerStick::Rain,
			48000,
			{ 200, 54 },
			9,
			{ 3, 5, 6, 6,
			  17, 7, 5, 24,
			  1 },
			{ { 12, -3 }, { -6, 6 }, { 12, 1 }, { -6, 9 },
			  { 12,  5 }, { -6, 3 }, { 12, 9 }, { -6, 0 },
			  { -6, -3 } },
			{ { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
			  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 } }
		}, {
			S2PrayerStick::Cow,
			28400,
			{ 201, 118 },
			7,
			{ 25, 26, 6, 24,
			  22, 2, 24 },
			{ { -6, -3 }, { -6, 3 }, { -6, 9 }, { 12, -3 },
			  {  4, -4 }, { 12, 3 }, { 12, 9 } },
			{ { 5, 5, 5, 1, 0, 0, 0, 0, 2, 5, 5, 5 },
			  { 5, 5, 1, 0, 0, 0, 0, 0, 0, 2, 5, 5 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 5, 5, 3, 0, 0, 0, 0, 0, 0, 4, 5, 5 },
			  { 5, 5, 5, 3, 0, 0, 0, 0, 4, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 3, 0, 0, 4, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 3, 4, 5, 5, 5, 5, 5 } }
		}, {
			S2PrayerStick::Bird,
			48000,
			{ 201, 78 },
			7,
			{ 21, 22, 5, 20,
			  6, 23, 1 },
			{ { -6, -3 }, { -6, 3 }, { -6, 9 }, { 12, -3 },
			  { 12,  1 }, { 12, 5 }, { 12, 9} },
			{ { 5, 5, 5, 5, 5, 1, 2, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 1, 0, 0, 2, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 4, 3, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 4, 5, 5, 3, 5, 5, 5, 5 } }
		}, {
			S2PrayerStick::Arrowhead,
			28700,
			{ 206, 76 },
			8,
			{ 7, 6, 6, 17,
			  1, 24, 18, 6 },
			{ { -6, -3 }, { -6, 1 }, { -6, 5 }, { -6, 9 },
			  { 12, -3 }, { 12, 1 }, { 12, 5 }, { 12, 9 } },
			{ { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4 },
			  { 5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5 },
			  { 5, 5, 3, 0, 0, 0, 0, 0, 0, 4, 5, 5 },
			  { 5, 5, 5, 3, 0, 0, 0, 0, 4, 5, 5, 5 },
			  { 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4 },
			  { 5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5 },
			  { 5, 5, 3, 0, 0, 0, 0, 0, 0, 4, 5, 5 },
			  { 5, 5, 5, 3, 0, 0, 0, 0, 4, 5, 5, 5 },
			  { 5, 5, 5, 5, 3, 0, 0, 4, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 3, 4, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 } }
		}, {
			S2PrayerStick::Butterfly,
			28700,
			{ 201, 78 },
			8,
			{ 22, 28, 2, 1,
			  5, 9, 19, 28 },
			{ { -6, -3 }, { -6, 1 }, { -6, 5 }, { -6, 9 },
			  { 12, -3 }, { 12, 1 }, { 12, 5 }, { 12, 9} },
			{ { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4 },
			  { 5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5 },
			  { 5, 5, 3, 0, 0, 0, 0, 0, 0, 4, 5, 5 },
			  { 5, 5, 1, 0, 0, 0, 0, 0, 0, 2, 5, 5 },
			  { 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 5 },
			  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
			  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 } }
		}, {
			S2PrayerStick::Star,
			28400,
			{ 186, 66 },
			8,
			{ 1, 3, 26, 28,
			  7, 9, 25, 24 },
			{ { -6, -3 }, { -6, 1 }, { -6, 5 }, { -6, 9 },
			  { 12, -3 }, { 12, 1 }, { 12, 5 }, { 12, 9} },
			{ { 5, 5, 5, 5, 5, 1, 2, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 1, 0, 0, 2, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 5 },
			  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
			  { 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4 },
			  { 5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 3, 0, 0, 4, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 3, 4, 5, 5, 5, 5, 5 } }
		}, {
			S2PrayerStick::Shield,
			28400,
			{ 202, 78 },
			8,
			{ 6, 7, 1, 27,
			  6, 17, 1, 7 },
			{ { -6, -3 }, { -6, 1 }, { -6, 5 }, { -6, 9 },
			  { 12, -3 }, { 12, 1 }, { 12, 5 }, { 12, 9 } },
			{ { 5, 5, 5, 5, 5, 1, 2, 5, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 1, 0, 0, 2, 5, 5, 5, 5 },
			  { 5, 5, 5, 1, 0, 0, 0, 0, 2, 5, 5, 5 },
			  { 5, 5, 1, 0, 0, 0, 0, 0, 0, 2, 5, 5 },
			  { 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 5 },
			  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
			  { 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4 },
			  { 5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5 },
			  { 5, 5, 3, 0, 0, 0, 0, 0, 0, 4, 5, 5 },
			  { 5, 5, 5, 3, 0, 0, 0, 0, 4, 5, 5, 5 },
			  { 5, 5, 5, 5, 3, 0, 0, 4, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 5, 3, 4, 5, 5, 5, 5, 5 } }
		}, {
			S2PrayerStick::Jack,
			48000,
			{ 201, 95 },
			9,
			{ 5, 29, 31, 5,
			  3, 31, 30, 28,
			  6 },
			{ { -6, -3 }, { -6,  0 }, { -6, 3 }, { -6, 6 },
			  { -6,  9 }, { 12, -3 }, { 12, 0 }, { 12, 3 },
			  { 12,  6 } },
			{ { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
			  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 },
			  { 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5 } }
		}
	};
};

constexpr S2TangramPuzzle::Puzzle S2TangramPuzzle::_puzzles[S2TangramPuzzle::kNumPuzzles];
constexpr int8 S2TangramPuzzle::TangramCel::_sizes[S2TangramPuzzle::TangramCel::kNumShapes];
constexpr uint32 S2TangramPuzzle::TangramCel::_shapes[S2TangramPuzzle::TangramCel::kNumShapes][S2TangramPuzzle::TangramCel::kSideSize];

#undef self
#define self(name) this, &S2Room28000::name

void S2Room28000::init(const int roomNo) {
	switch (roomNo) {
	case 28300:
		sound().createAmbient(28);
		flags().set(kGameFlag101);
		sound().fade(32802, 0, 15, 12, true);
		sound().play(32801, true, 0);
		sound().fade(32801, 80, 10, 12);
		if (flags().get(kGameFlag28) && !flags().get(kGameFlag30)) {
			flags().set(kGameFlag30);
			flags().set(kGameFlag67);
			room().drawPic(2);
			movie().play(5120);
		}

		if (_game.getRandomNumber(1, 100) < 30) {
			sound().play(12842);
		}

		room().drawPan(28100);
		addPanoramaExit(6540, 438, 2, 574, 511);
		addPanoramaExit(28400, 1434, 51, 1548, 511);
		if (!flags().get(kGameFlag6)) {
			auto &sprite = emplaceSprite(true, 28301, GLPoint(130, 352), 0, 1, true);
			sprite.setMouseDownHandler(makeRestoreLife(kGameFlag6));
		}
		break;

	case 28400:
		room().drawPan(28400);
		addPanoramaExit(28300, 257, 121, 471, 511);
		addPanoramaExit(28500, 1761, 169, 1922, 511);
		sound().fade(32801, 0, 15, 12, true);
		sound().fade(32803, 0, 15, 12, true);
		sound().play(32802, true, 0);
		sound().fade(32802, 80, 10, 12);
		setUpFight();
		break;

	case 28500:
		room().drawPan(28500);
		if (!flags().get(kGameFlag10)) {
			auto &sprite = emplaceSprite(true, 28501, GLPoint(1100, 17), 0, 1, true);
			sprite.setMouseDownHandler(makeRestoreLife(kGameFlag10));
		}
		if (!flags().get(kGameFlag11)) {
			auto &sprite = emplaceSprite(true, 28502, GLPoint(4, 371), 0, 1, true);
			sprite.setMouseDownHandler(makeRestoreLife(kGameFlag11));
		}
		addPanoramaExit(28700, 1459, 110, 1688, 511);
		addPanoramaExit(28400, 585, 125, 878, 511);
		sound().fade(32802, 0, 15, 12, true);
		sound().play(32803, true, 0);
		sound().fade(32803, 80, 10, 12);
		break;

	case 28600:
		enterFrom(48200, 262, 0);
		room().drawPan(28600);
		if (!flags().get(kGameFlag7)) {
			auto &sprite = emplaceSprite(true, 28601, GLPoint(1257, 11), 0, 1, true);
			sprite.setMouseDownHandler(makeRestoreLife(kGameFlag7));
		}
		addPanoramaExit(48000, 560, 125, 645, 511);
		addPanoramaExit(48200, 1520, 100, 1680, 511);
		sound().fade(32802, 0, 15, 12, true);
		sound().play(32803, true, 0);
		sound().fade(32803, 80, 10, 12);
		break;

	case 28700:
		room().drawPan(28700);
		addPanoramaExit(28500, 608, 146, 817, 511);
		addPanoramaExit(28900, 1630, 105, 1839, 511);
		sound().fade(32803, 0, 15, 12, true);
		sound().play(32802, true, 0);
		sound().fade(32802, 80, 10, 12);
		setUpFight();
		break;

	case 28900:
		room().drawPan(28900);
		if (!flags().get(kGameFlag8)) {
			auto &sprite = emplaceSprite(true, 28901, GLPoint(10, 7), 0, 1, true);
			sprite.setMouseDownHandler(makeRestoreLife(kGameFlag8));
		}
		if (!flags().get(kGameFlag9)) {
			auto &sprite = emplaceSprite(true, 28902, GLPoint(1336, 4), 0, 1, true);
			sprite.setMouseDownHandler(makeRestoreLife(kGameFlag9));
		}
		addPanoramaExit(28700, 448, 132, 667, 511);
		addPanoramaExit(48000, 1445, 131, 1689, 511);
		sound().fade(32802, 0, 15, 12, true);
		sound().play(32803, true, 0);
		sound().fade(32803, 80, 10, 12);
		break;

	case 48000:
		room().drawPan(48000);
		addPanoramaExit(28900, 291, 170, 516, 511);
		addPanoramaExit(28600, 1825, 183, 1890, 511);
		sound().fade(32803, 0, 15, 12, true);
		sound().fade(32801, 0, 15, 12, true);
		sound().play(32802, true, 0);
		sound().fade(32802, 80, 10, 12);
		setUpFight();
		break;

	case 48200:
		sound().createAmbient(28);
		enterFrom(28600, 700, 0);
		room().drawPan(48200);
		addPanoramaExit(50000, 754, 180, 896, 511);
		addPanoramaExit(28600, 1957, 160, 2047, 511);
		addPanoramaExit(28600, 0, 160, 129, 511);
		sound().fade(32802, 0, 15, 12, true);
		sound().play(32801, true, 0);
		sound().fade(32801, 80, 10, 12);
		break;

	case 48201:
	case 48202:
	case 48203:
	case 48204:
	case 48205:
	case 48206:
	case 48207:
	case 48208:
	case 48209:
	case 48210:
	case 48211:
	case 48212:
		setSubRoom<S2TangramPuzzle>(roomNo);
		break;

	case 48999:
		phone().cancelCall();
		setScript(self(cancelScript));
		break;
	default:
		error("Unknown room %d", roomNo);
	}
}

void S2Room28000::dispose(const int roomNo) {
	if (room().getNextRoomNo() < 28300 || room().getNextRoomNo() > 48999) {
		sound().fade(32801, 0, 15, 12, true);
		sound().fade(32802, 0, 15, 12, true);
		sound().fade(32803, 0, 15, 12, true);
	}

	switch (roomNo) {
	case 28300:
	case 48200:
		sound().fade(32801, 0, 15, 12, true);
		break;

	case 28400:
	case 28700:
	case 48000:
		_fightScripts.clear();
		_fightCyclers.clear();
		for (auto &sprite : _fightSprites) {
			room().getPanorama().removeSprite(sprite);
		}
		_fightSprites.clear();
		break;
	}

	S2Room::dispose(roomNo);
}

void S2Room28000::cancelScript(GLScript &script, const int state) {
	Common::Point position;
	int roomNo;

	switch (inventory().getPrayerStickId()) {
	case S2PrayerStick::Arrowhead:
		position = { 1075, 86 };
		roomNo = 48208;
		break;

	case S2PrayerStick::Claw:
		position = { 1082, 172 };
		roomNo = 48201;
		break;

	case S2PrayerStick::Bird:
		position = { 11, 135 };
		roomNo = 48207;
		break;

	case S2PrayerStick::Hourglass:
		position = { 1294, 116 };
		roomNo = 48202;
		break;

	case S2PrayerStick::Jack:
		position = { 1433, 29 };
		roomNo = 48212;
		break;

	case S2PrayerStick::Cow:
		position = { 506, 144 };
		roomNo = 48206;
		break;

	case S2PrayerStick::Star:
		position = { 67, 60 };
		roomNo = 48210;
		break;

	case S2PrayerStick::Spider:
		position = { 446, 85 };
		roomNo = 48204;
		break;

	case S2PrayerStick::Rain:
		position = { 1669, 107 };
		roomNo = 48205;
		break;

	case S2PrayerStick::Butterfly:
		position = { 1453, 97 };
		roomNo = 48209;
		break;

	case S2PrayerStick::Stairs:
		position = { 300, 93 };
		roomNo = 48203;
		break;

	case S2PrayerStick::Shield:
		position = { 1595, 56 };
		roomNo = 48211;
		break;

	default:
		break;
	}

	switch (state) {
	case 0: {
		user().setIsHandsOn(false);
		auto &sprite = emplaceSprite(true, 28000, position, 0, 3, true);
		_panoramaCycler.reset(new S2PanoramaEndForwardCycler());
		_panoramaCycler->add(sprite);
		_panoramaCycler->start(script);
		sound().play(12803);
		break;
	}

	case 1:
		_panoramaCycler.reset();
		room().newRoom(roomNo);
		user().setIsHandsOn(true);
		break;
	}
}

void S2Room28000::setUpFight() {
	const auto roomNo = room().getCurrentRoomNo();
	using PS = S2PrayerStick;
	if (inventory().getPrayerStickId() != PS::None) {
		flags().set(kGameFlag217);
		Common::Rect exit;
		switch (inventory().getPrayerStickId()) {
		case PS::Claw:
			if (roomNo == 48000) {
				exit = { 1108, 189, 1164, 239 };
			}
			break;

		case PS::Cow:
			if (roomNo == 28400) {
				exit = { 535, 150, 589, 227 };
			}
			break;

		case PS::Spider:
			if (roomNo == 28700) {
				exit = { 475, 117, 533, 168 };
			}
			break;

		case PS::Shield:
			if (roomNo == 28400) {
				exit = { 1635, 96, 1662, 128 };
			}
			break;

		case PS::Stairs:
			if (roomNo == 28700) {
				exit = { 332, 128, 371, 163 };
			}
			break;

		case PS::Butterfly:
			if (roomNo == 28700) {
				exit = { 1478, 117, 1535, 188 };
			}
			break;

		case PS::Jack:
			if (roomNo == 48000) {
				exit = { 1463, 59, 1509, 99 };
			}
			break;

		case PS::Hourglass:
			if (roomNo == 28400) {
				exit = { 1331, 149, 1357, 177 };
			}
			break;

		case PS::Bird:
			if (roomNo == 48000) {
				exit = { 30, 146, 101, 217 };
			}
			break;

		case PS::Rain:
			if (roomNo == 48000) {
				exit = { 1687, 130, 1759, 187 };
			}
			break;

		case PS::Star:
			if (roomNo == 28400) {
				exit = { 98, 84, 144, 134 };
			}
			break;

		case PS::Arrowhead:
			if (roomNo == 28700) {
				exit = { 1113, 105, 1145, 150 };
			}
			break;

		default:
			break;
		}

		if (!exit.isEmpty()) {
			addPanoramaExit(48999, exit.left, exit.top, exit.right, exit.bottom, S2Cursor::kHighlightCel);
		}
	}

	const auto max = _game.getRandomNumber(1, 4);
	switch (roomNo) {
	case 28400: {
		auto spriteNo = _game.getRandomNumber(1, 8);
		uint16 resourceNo = 28400 + spriteNo;
		Common::Point position;
		int numCels;
		for (auto i = 0; i < max; ++i) {
			switch (spriteNo) {
			case 1:
				position = { 66, 54 };
				numCels = 5;
				break;

			case 2:
				position = { 724, 198 };
				numCels = 5;
				break;

			case 3:
				position = { 506, 112 };
				numCels = 6;
				break;

			case 4:
				position = { 1600, 59 };
				numCels = 6;
				break;

			case 5:
				position = { 1268, 102 };
				numCels = 5;
				break;

			case 6:
				position = { 1840, 179 };
				numCels = 5;
				break;

			case 7:
				position = { 0, 222 };
				numCels = 5;
				break;

			case 8:
				position = { 1136, 169 };
				numCels = 7;
				break;
			}

			_fightSprites.emplace_back(resourceNo, position, 0, numCels, true, true);

			if (++spriteNo == 9) {
				spriteNo = 1;
			}
		}
		break;
	}

	case 28700: {
		auto spriteNo = _game.getRandomNumber(1, 8);
		uint16 resourceNo = 28700 + spriteNo;
		Common::Point position;
		int numCels;
		for (auto i = 0; i < max; ++i) {
			switch (spriteNo) {
			case 1:
				position = { 307, 95 };
				numCels = 5;
				break;

			case 2:
				position = { 1184, 109 };
				numCels = 5;
				break;

			case 3:
				position = { 1439, 72 };
				numCels = 6;
				break;

			case 4:
				position = { 474, 178 };
				numCels = 6;
				break;

			case 5:
				position = { 1063, 52 };
				numCels = 5;
				break;

			case 6:
				position = { 93, 212 };
				numCels = 5;
				break;

			case 7:
				position = { 818, 100 };
				numCels = 6;

			case 8:
				position = { 1475, 189 };
				numCels = 5;
			}

			_fightSprites.emplace_back(resourceNo, position, 0, numCels, true, true);

			if (++spriteNo == 9) {
				spriteNo = 1;
			}
		}
		break;
	}

	case 48000: {
		auto spriteNo = _game.getRandomNumber(1, 5);
		uint16 resourceNo = 48011 + spriteNo;
		Common::Point position;
		int numCels;
		for (auto i = 0; i < max; ++i) {
			switch (spriteNo) {
			case 1:
				position = { 894, 44 };
				numCels = 5;
				break;

			case 2:
				position = { 319, 108 };
				numCels = 5;
				break;

			case 3:
				position = { 1080, 150 };
				numCels = 6;
				break;

			case 4:
				position = { 108, 11 };
				numCels = 5;
				break;

			case 5:
				position = { 1908, 263 };
				numCels = 5;
				break;
			}

			_fightSprites.emplace_back(resourceNo, position, 0, numCels, true, true);

			if (++spriteNo == 6) {
				spriteNo = 1;
			}
		}
		break;
	}
	}

	_fightCyclers.resize(max);

	for (auto i = 0; i < max; ++i) {
		_fightScripts.emplace_back(self(fightScript), 0, i);
	}
}

void S2Room28000::fightScript(GLScript &script, const int state) {
	auto &cycler = _fightCyclers[script.getData()];
	auto &sprite = _fightSprites[script.getData()];

	switch (state) {
	case 0:
		cycler.reset(new S2PanoramaEndResetCycler());
		room().getPanorama().addSprite(sprite, true);
		sprite.setCycleSpeed(0);
		script.setCycles(1);
		break;

	case 1:
		script.setSeconds(_game.getRandomNumber(5, 15));
		break;

	case 2:
		cycler->add(sprite);
		cycler->start(script);
		sound().play(12801, false, Audio32::kMaxVolume, false, nullptr, make_reg(0, script.getData()));
		break;

	case 3: {
		if (room().getCurrentGlobalRoomNo()) {
			script.setState(2);
			script.setCycles(1);
			break;
		}

		sound().play(12802, false, Audio32::kMaxVolume, false, nullptr, make_reg(0, script.getData()));

		int damage;
		if (!inventory().hasPrayerStick()) {
			damage = flags().get(kGameFlag16) ? -7 : -4;
		} else {
			damage = flags().get(kGameFlag16) ? -5 : -2;
		}
		interface().changeLife(damage);
		score().doEvent(kScore6);
		script.setState(0);
		script.setCycles(1);
		break;
	}
	}
}

void S2Room28000::restoreLife(S2PanoramaSprite &target, const GameFlag flag) {
	if (interface().getLife() == 100) {
		return;
	}

	sound().play(12804);
	flags().set(flag);
	removeChild(target);
}

} // End of namespace Sci
