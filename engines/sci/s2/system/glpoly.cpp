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

#include "common/textconsole.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/glpoly.h"

namespace Sci {

GLPoly::GLPoly(AbsGLPlane &plane) :
	GLTarget(plane) {}

GLPoly::GLPoly(AbsGLPlane &plane, const PointsList &points) :
	GLTarget(plane),
	_points(points) {}

GLPoly::~GLPoly() {
	_plane->getCast().removeEventHandler(*this);
}

void GLPoly::init() {
	_plane->getCast().addEventHandler(*this);
}


static inline int crossProduct(const Common::Point &p1, const Common::Point &p2) {
	const int v = p1.x * p2.y - p1.y * p2.x;
	return v == 0 ? 0 : v < 0 ? -1 : 1;
}

bool GLPoly::checkIsOnMe(const GLPoint &p) const {
	// TODO: This was in the kernel in SSCI, but our pathfinding code is not
	// currently designed in a manner which allows it to be reused like this, so
	// this is a reimplementation of the SSCI algorithm which is just a cross
	// counter
	if (_points.size() < 3) {
		warning("GLPoly::checkIsOnMe with less than 3 points");
		return false;
	}

	Common::Point v1(_points[0]);
	Common::Point v2(_points[1]);
	Common::Point v3(_points[2]);
	Common::Point v4(_points.size() > 3 ? _points[3] : _points[0]);

	int numCrosses = 0;
	for (int i = 4; ; ++i) {
		if (((v1.y <= p.y && p.y <= v2.y) || (v2.y <= p.y && p.y <= v1.y)) &&
			((v1.x <= p.x && p.x <= v2.x) || (v2.x <= p.x && p.x <= v1.x))) {
			if (crossProduct(p - v1, p - v2) == 0) {
				return true;
			}
		}

		if ((v1.y < p.y && p.y < v2.y) || (v2.y < p.y && p.y < v1.y)) {
			Common::Point t(v2 - v1);
			if (t.y < 0) {
				t = Common::Point(-t.x, -t.y);
			}
			if (crossProduct(t, p - v1) > 0) {
				++numCrosses;
			}
		} else if (v2.y == p.y && p.x < v2.x) {
			if (v3.y != v2.y) {
				if ((v2.y - v1.y) * (v3.y - v2.y) > 0) {
					++numCrosses;
				}
			} else {
				if ((v2.y - v1.y) * (v4.y - v3.y) > 0) {
					++numCrosses;
				}
			}
		}

		v1 = v2;
		v2 = v3;
		v3 = v4;

		if (v3 == _points[2]) {
			break;
		}

		v4 = _points[i % _points.size()];
	}

	return (numCrosses & 1);
}

} // End of namespace Sci
