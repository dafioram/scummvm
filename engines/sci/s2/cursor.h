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

#ifndef SCI_S2_CURSOR_H
#define SCI_S2_CURSOR_H

#include "sci/s2/system/glcursor.h"
#include "sci/s2/system/types.h"

namespace Sci {

class GfxCursor32;

class S2Cursor : public GLCursor {
public:
	enum Cel {
		kNormalCel    = 0,
		kHighlightCel = 1,
		kWaitCel      = 2,
		kForwardCel   = 3,
		kBackCel      = 4
	};

	S2Cursor(GfxCursor32 &kernelCursor);

	bool hasInventory() const { return _inventoryState & kInventory; }
	bool hasPrayerStick() const { return _inventoryState & kPrayerStick; }

	// SSCI did not accept a GLCelRes, it accepted an S2Inventory and just used
	// the global inventory table directly, but we do not have one of those (it
	// is part of S2InventoryManager) and the only reason for sending the
	// inventory item was to get the cel anyway
	void getItem(const GLCelRes &celInfo);
	void dropItem();
	void getPrayerStick(const GLCelRes &celInfo);
	void dropPrayerStick();

private:
	enum InventoryState {
		kNormal      = 0,
		kInventory   = 1,
		kPrayerStick = 2
	};

	GLCelRes _noInventoryCel;
	GLCelRes _prayerStickNormalCel;
	GLCelRes _inventoryCel;
	int _inventoryState;
};

} // End of namespace Sci

#endif
