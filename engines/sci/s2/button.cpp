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

#include "sci/s2/button.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/gluser.h"
#include "sci/s2/system/types.h"

namespace Sci {

S2Button::S2Button(AbsGLPlane &plane, const uint16 viewNo, const int16 loopNo, const int16 celNo, const GLPoint &position, const int16 priority) :
	GLButton(plane, viewNo, loopNo, celNo, position, priority),
	_autoHighlight(false) {}

void S2Button::setAutoHighlight(const bool set) {
	if (!set && _autoHighlight && getIsHighlighted()) {
		dim();
	}
	_autoHighlight = set;
}

void S2Button::doIt() {
	if (!_autoHighlight || !getIsEnabled() || getIsDepressed() || !getIsVisible()) {
		return;
	}

	const auto mousePosition(getPlane().toLocal(_user->getMousePosition()));
	const bool isOnMe = checkIsOnMe(mousePosition);
	if (!getIsHighlighted() && isOnMe) {
		highlight();
	} else if (getIsHighlighted() && !isOnMe) {
		dim();
	}
}

} // End of namespace Sci
