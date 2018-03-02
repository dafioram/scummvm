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

#include "sci/s2/exit.h"
#include "sci/s2/cursor.h"
#include "sci/s2/room_manager.h"
#include "sci/s2/system/glevent.h"

namespace Sci {

S2Cursor *S2Exit::_cursor = nullptr;
S2RoomManager *S2Exit::_roomManager = nullptr;
constexpr GLPoint S2Exit::_defaultPoly[];

S2Exit::S2Exit(AbsGLPlane &plane, const int targetRoomNo, const S2Cursor::Cel cursorCel) :
	S2Exit(plane, targetRoomNo, PointsList(_defaultPoly, ARRAYSIZE(_defaultPoly)), cursorCel) {}

S2Exit::S2Exit(AbsGLPlane &plane, const int targetRoomNo, const Common::Rect &rect, const S2Cursor::Cel cursorCel) :
	S2Exit(plane, targetRoomNo, rectToPoints(rect), cursorCel) {}

S2Exit::S2Exit(AbsGLPlane &plane, const int targetRoomNo, const int16 x1, const int16 y1, const int16 x2, const int16 y2, const S2Cursor::Cel cursorCel) :
	S2Exit(plane, targetRoomNo, rectToPoints({ x1, y1, int16(x2 + 1), int16(y2 + 1) }), cursorCel) {}

S2Exit::S2Exit(AbsGLPlane &plane, const int targetRoomNo, const PointsList &poly, const S2Cursor::Cel cursorCel) :
	GLPoly(plane, poly),
	_isEnabled(true),
	_targetRoomNo(targetRoomNo),
	_cursorCel(cursorCel) {
	GLPoly::init();
}

bool S2Exit::handleEvent(GLEvent &event) {
	if (event.getType() == kSciEventMouseRelease && _isEnabled && !_cursor->hasInventory()) {
		event.localize(*_plane);
		if (checkIsOnMe(event.getMousePosition())) {
			_cursor->endHighlight();
			_roomManager->setNextRoomNo(_targetRoomNo);
			event.claim();
		}
	}
	event.globalize();
	return event.isClaimed();
}

} // End of namespace Sci
