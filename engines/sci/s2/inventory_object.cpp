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

#include "sci/s2/inventory_object.h"
#include "sci/s2/system/glcel.h"
#include "sci/s2/system/types.h"
#include "sci/s2/game.h"

namespace Sci {

S2Game *S2InventoryObject::_game = nullptr;

S2InventoryObject::S2InventoryObject(AbsGLPlane &plane, const uint16 viewNo, const int16 loopNo, const int16 celNo, Inventory id, const GLPoint &position, const int16 priority, const int slotNo) :
	GLCel(plane, viewNo, loopNo, celNo, position, priority),
	_id(id),
	_isEnabled(true),
	_slotNo(slotNo) {}

void S2InventoryObject::enable() {
	_isEnabled = true;
	setBounds();
}

void S2InventoryObject::disable() {
	_isEnabled = false;
}

bool S2InventoryObject::handleEvent(GLEvent &event) {
	if (_game->getFlags().get(kGameFlag44)) {
		return false;
	}
	if (_game->getRoomManager().getCurrentRoomNo() == 6667) {
		return false;
	}
	const auto globalRoomNo = _game->getRoomManager().getCurrentGlobalRoomNo();
	if (globalRoomNo && (globalRoomNo < 4300 || globalRoomNo >= 4400)) {
		return false;
	}

	event.localize(getPlane());
	if (event.getType() == kSciEventMouseRelease && _isEnabled && checkIsOnMe(event.getMousePosition())) {

		if (_slotNo >= 0) {
			if (_id != Inventory::None) {
				_game->getInventoryManager().selectItem(_slotNo);
			} else if (_game->getCursor().hasInventory()) {
				_game->getInventoryManager().unselectItem(true);
			}
		} else {
			_game->getInventoryManager().addItem(_id);
			disable();
			hide();
		}
		event.isClaimed();
	}
	event.globalize();

	return event.isClaimed();
}

} // End of namespace Sci
