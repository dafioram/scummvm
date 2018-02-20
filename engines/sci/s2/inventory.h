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

#ifndef SCI_S2_INVENTORY_H
#define SCI_S2_INVENTORY_H

#include "common/scummsys.h"
#include "sci/s2/system/types.h"

namespace Sci {

enum class S2Inventory : uint8 {
	None,
	FortyDollars,
	kInv2,
	kInv3,
	kInv4,
	kInv5,
	kInv6,
	kInv7,
	kInv8,
	kInv9,
	kInv10,
	kInv11,
	kInv12,
	kInv13,
	kInv14,
	kInv15,
	kInv16,
	kInv17,
	kInv18,
	kInv19,
	kInv20,
	kInv21,
	kInv22,
	kInv23,
	kInv24,
	kInv25,
	kInv26,
	kInv27,
	kInv28,
	kInv29,
	kInv30,
	kInv31,
	kInv32,
	kInv33,
	kInv34,
	kInv35,
	kInv36,
	kInv37,
	kInv38,
	kInv39,
	kInv40,
	kInv41,
	Change,
	kInv43,
	kInv44,
	kInv45,
	kInv46,
	kInv47,
	kInv48,
	kInv49,
	kInv50,
	kInv51,
	kNumInventory
};

enum class S2InventoryState : uint8 {
	Normal = 0,
	Placed = 1,
	Taken  = 2,
	InUse  = 4,
	Used   = 8
};

enum class S2PrayerStick : uint8 {
	None = 0,
	kNumPrayerSticks
};

struct S2InventoryItem {
	S2InventoryState state;
	// SSCI held three cel types and two states, but the first and third cels
	// were always the same, and the second state was used to reset the first
	// state because the inventory was constructed in a static mutable table, so
	// we can just use two cels and once state instead
	GLCelRes smallCel;
	GLCelRes bigCel;

	S2InventoryItem() {}
	constexpr S2InventoryItem(S2InventoryState state_, const GLCelRes &smallCel_, const GLCelRes &bigCel_) :
		state(state_),
		smallCel(smallCel_),
		bigCel(bigCel_) {}
};

} // End of namespace Sci

#endif
