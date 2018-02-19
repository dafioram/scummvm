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

#include "sci/s2/game.h"
#include "sci/s2/rooms/10000.h"
#include "sci/s2/system/glpanorama.h"

namespace Sci {

void S2Room10000::init(const int roomNo) {
	switch (roomNo) {
	case 10400:
		_game.getRoomManager().drawPan(10400);
		_game.getSoundManager().createAmbient(10);

		_game.getFlags().set(kGameFlag91);
		_game.getFlags().set(kGameFlag71);

		if (_game.getRoomManager().getPreviousRoomNo() == 1020) {
			_game.getRoomManager().getPanorama().setPanX(717);
			_game.getInterface().resetButtons();
			_game.save();
		}

		// TODO: Add panorama exits
		break;
	}
}

void S2Room10000::dispose(const int roomNo) {

}

bool S2Room10000::handleEvent(GLEvent &event) {
	return false;
}

} // End of namespace Sci
