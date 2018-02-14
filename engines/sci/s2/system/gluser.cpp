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

#include "sci/s2/system/glgame.h"
#include "sci/s2/system/gluser.h"

namespace Sci {

GLUser::GLUser(GLGame &game) :
	_game(game),
	_state(0) {
	_hogs.reserve(10);
	_primaDonnas.reserve(10);
	_orphans.reserve(10);
}

void GLUser::doIt() {
	_event.refresh();

	_mousePosition = _event.getMousePosition();

	if ((getHandlesNulls() || _event.getType() != kSciEventNone) &&
		(getIsHandsOn() || handleHandsOff(_event))) {

		handleEvent(_event);
	}
}

bool GLUser::handleEvent(GLEvent &event) {
	if (_hogs.empty()) {
		_primaDonnas.handleEvent(event) ||
		_game.getPlanes().handleEvent(event) ||
		_orphans.handleEvent(event);
	} else {
		processHogs(event);
	}

	return event.isClaimed();
}

bool GLUser::handleHandsOff(GLEvent &event) {
	event.claim();
	return false;
}

bool GLUser::processHogs(GLEvent &event) {
	if (getHogsAreModal()) {
		while (!_hogs.handleEvent(event)) {
			event.refresh();
		}
	} else {
		_hogs.handleEvent(event);
	}

	return event.isClaimed();
}

} // End of namespace Sci
