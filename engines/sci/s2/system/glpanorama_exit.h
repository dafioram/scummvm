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

#ifndef SCI_S2_GLPANORAMA_EXIT_H
#define SCI_S2_GLPANORAMA_EXIT_H

#include "common/rect.h"

namespace Sci {

class GLPanoramaExit : public Common::Rect {
public:
	GLPanoramaExit(const int roomNo, const int16 left, const int16 top, const int16 right, const int16 bottom, const int16 cursorCel = 3) :
		Common::Rect(left, top, right, bottom),
		_roomNo(roomNo),
		_cursorCel(cursorCel) {}

	int getRoomNo() const { return _roomNo; }
	int16 getCursorCel() const { return _cursorCel; }

private:
	int _roomNo;
	int16 _cursorCel;
};

} // End of namespace Sci

#endif
