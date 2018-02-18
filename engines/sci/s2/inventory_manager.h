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

#ifndef SCI_S2_INVENTORY_MANAGER_H
#define SCI_S2_INVENTORY_MANAGER_H

#include "common/textconsole.h"
#include "sci/s2/inventory.h"

namespace Sci {

class S2InventoryManager {
public:
	enum class State {
		Normal = 0,
		Placed = 1,
		Taken  = 2,
		InUse  = 4,
		Used   = 8
	};

	enum class PrayerStick {
		None = 0
	};

	PrayerStick getPrayerStickId() const { warning("TODO: %s", __PRETTY_FUNCTION__); return PrayerStick::None; }

	void selectItem(const int slotNo);
	void unselectItem(const bool returnToInventory);

	void addItem(const Inventory item);

	State getState(const Inventory item) const { warning("TODO: %s", __PRETTY_FUNCTION__); return State::Normal; }
};

} // End of namespace Sci

#endif
