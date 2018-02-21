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

#ifndef SCI_S2_EXIT_H
#define SCI_S2_EXIT_H

#include "common/array.h"
#include "sci/s2/cursor.h"
#include "sci/s2/system/glpoly.h"
#include "sci/s2/system/types.h"

namespace Sci {

class S2Exit : public GLPoly {
public:
	S2Exit(AbsGLPlane &plane, const int targetRoomNo, const S2Cursor::Cel cursorCel = S2Cursor::kForwardCel);
	S2Exit(AbsGLPlane &plane, const int targetRoomNo, const Common::Rect &rect, const S2Cursor::Cel cursorCel = S2Cursor::kForwardCel);
	S2Exit(AbsGLPlane &plane, const int targetRoomNo, const PointsList &poly, const S2Cursor::Cel cursorCel = S2Cursor::kForwardCel);

	int getTargetRoomNo() const { return _targetRoomNo; }
	void setTargetRoomNo(const int roomNo) { _targetRoomNo = roomNo; }

	bool getIsEnabled() const { return _isEnabled; }
	void disable() { _isEnabled = false; }
	void enable() { _isEnabled = true; }

	S2Cursor::Cel getCursorCel() const { return _cursorCel; }

private:
	static GLPoint _defaultPoly[10];

	PointsList rectToPoints(const Common::Rect &rect) const {
		// In order to match the poly dimensions for rectangles in SSCI, the
		// rectangle coordinates need to be converted from BR-exclusive to
		// BR-inclusive
		const int16 right = rect.right - 1;
		const int16 bottom = rect.bottom - 1;

		const GLPoint points[] = {
			{ rect.left, rect.top }, { right, rect.top },
			{ right, bottom }, { rect.left, bottom }
		};
		return PointsList(points, ARRAYSIZE(points));
	}

	bool _isEnabled;
	int _targetRoomNo;
	S2Cursor::Cel _cursorCel;
};

} // End of namespace Sci

#endif
