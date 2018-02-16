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
#include "sci/event.h"
#include "sci/s2/button.h"
#include "sci/s2/game.h"
#include "sci/s2/kernel.h"
#include "sci/s2/rooms/global.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/types.h"

namespace Sci {

void S2GlobalRoom::init(const int roomNo) {
	_plane = _game.getRoomManager().getGlobalPlane();
	_game.getInterface().putText(0);
	_kernel.eventManager.flushEvents();

	switch (roomNo) {
	case 4000:
		initMainMenu();
		break;
	default:
		error("Unknown global room %d", roomNo);
	}
}

void S2GlobalRoom::dispose(const int roomNo) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

bool S2GlobalRoom::handleEvent(GLEvent &event) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
	event.claim();
	return true;
}

void S2GlobalRoom::initMainMenu() {
	auto &button = addButton(*_plane, 4000, 0, 0, GLPoint(0, 479), 202);
	button.setHighlightedFace(4000, 0, 2);
	button.setDepressedFace(4000, 0, 2);
	button.enable();
	button.setAutoHighlight(true);
	button.setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
		_game.getSoundManager().play(10913, false, 100);
		warning("TODO: New game handler");
	});
}

} // End of namespace Sci
