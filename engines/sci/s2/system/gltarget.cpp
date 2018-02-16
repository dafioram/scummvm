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

#include "sci/s2/system/glevent.h"
#include "sci/s2/system/gltarget.h"

namespace Sci {

GLUser *GLTarget::_user = nullptr;

GLTarget::GLTarget() :
	GLObject(),
	_plane(nullptr),
	_mouseHandler(nullptr) {}

GLTarget::GLTarget(AbsGLPlane &plane) :
	GLObject(),
	_plane(&plane),
	_mouseHandler(nullptr) {}

bool GLTarget::handleEvent(GLEvent &event) {
	if (event.getType() == kSciEventMouse && checkIsOnMe(event.getMousePosition())) {
		if (_mouseHandler) {
			_mouseHandler(event, *this);
		}
		event.claim();
	}
	return event.isClaimed();
}

} // End of namespace Sci
