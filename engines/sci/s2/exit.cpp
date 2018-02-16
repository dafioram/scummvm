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

namespace Sci {

GLPoint S2Exit::_defaultPoly[] = {
	{  64,  0 }, { 575,   0 }, { 575, 383 }, {  64, 383 }, {  64,  0 },
	{ 144, 80 }, { 144, 303 }, { 494, 303 }, { 494,  80 }, { 144, 80 }
};

S2Exit::S2Exit(AbsGLPlane &plane, const int targetRoomNo, const int16 cursorCel) :
	S2Exit(plane, targetRoomNo, PointsList(_defaultPoly, ARRAYSIZE(_defaultPoly)), cursorCel) {}

S2Exit::S2Exit(AbsGLPlane &plane, const int targetRoomNo, const Common::Rect &rect, const int16 cursorCel) :
	S2Exit(plane, targetRoomNo, rectToPoints(rect), cursorCel) {}

S2Exit::S2Exit(AbsGLPlane &plane, const int targetRoomNo, const PointsList &poly, const int16 cursorCel) :
	GLPoly(plane, PointsList(_defaultPoly, ARRAYSIZE(_defaultPoly))),
	_isEnabled(true),
	_targetRoomNo(targetRoomNo),
	_cursorCel(cursorCel) {
	GLPoly::init();
}

bool S2Exit::checkIsOnMe(const GLPoint &point) const {
	warning("TODO: %s", __PRETTY_FUNCTION__);
	return false;
}

} // End of namespace Sci
