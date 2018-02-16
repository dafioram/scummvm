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
#include "sci/s2/system/glpoly.h"
#include "sci/s2/system/types.h"

namespace Sci {

class S2Exit : public GLPoly {
public:
	S2Exit(AbsGLPlane &plane, const int targetRoomNo, const int16 cursorCel = 3);
	S2Exit(AbsGLPlane &plane, const int targetRoomNo, const Common::Rect &rect, const int16 cursorCel = 3);
	S2Exit(AbsGLPlane &plane, const int targetRoomNo, const PointsList &poly, const int16 cursorCel = 3);

	int getTargetRoomNo() const { return _targetRoomNo; }
	void setTargetRoomNo(const int roomNo) { _targetRoomNo = roomNo; }

	bool getIsEnabled() const { return _isEnabled; }
	void disable() { _isEnabled = false; }
	void enable() { _isEnabled = true; }

	int16 getCursorCel() const { return _cursorCel; }

	virtual bool checkIsOnMe(const GLPoint &point) const override;

private:
	static GLPoint _defaultPoly[10];

	PointsList rectToPoints(const Common::Rect &rect) const {
		const GLPoint points[] = {
			{ rect.left, rect.top }, { rect.right, rect.top },
			{ rect.right, rect.bottom }, { rect.left, rect.bottom }
		};
		return PointsList(points, ARRAYSIZE(points));
	}

	bool _isEnabled;
	int _targetRoomNo;
	int16 _cursorCel;
};

} // End of namespace Sci

#endif
