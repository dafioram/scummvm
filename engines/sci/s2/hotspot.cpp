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

#include "sci/s2/hotspot.h"
#include "sci/s2/system/glevent.h"
#include "sci/s2/system/glplane.h"

namespace Sci {

S2Hotspot::S2Hotspot(AbsGLPlane &plane, const int16 x1, const int16 y1, const int16 x2, const int16 y2) :
	GLPoly(plane),
	_isEnabled(false),
	_mouseUpHandler(nullptr) {
	GLPoint points[] = { { x1, y1 }, { x2, y1 }, { x2, y2 }, { x1, y2 } };
	setNeedsEvent(true);
	setPoints({ points, ARRAYSIZE(points) });
	enable();
}

S2Hotspot::~S2Hotspot() {
	// SSCI required the caller to do this manually
	if (_isEnabled) {
		_plane->getCast().remove(*this);
	}
}

bool S2Hotspot::handleEvent(GLEvent &event) {
	if (_isEnabled && event.getType() == kSciEventMouseRelease) {
		event.localize(*_plane);
		if (checkIsOnMe(event.getMousePosition())) {
			if (_mouseUpHandler) {
				_mouseUpHandler(event, *this);
			}
			event.claim();
		} else {
			event.globalize();
		}
	}
	return event.isClaimed();
}

void S2Hotspot::enable() {
	if (!_isEnabled) {
		_plane->getCast().add(*this);
		_isEnabled = true;
	}
}

void S2Hotspot::disable() {
	if (_isEnabled) {
		_plane->getCast().remove(*this);
		_isEnabled = false;
	}
}

} // End of namespace Sci
