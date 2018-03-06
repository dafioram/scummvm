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

#include "sci/s2/cursor.h"

namespace Sci {

static const GLCelRes defaultCel = GLCelRes(99, 0, S2Cursor::kNormalCel);

S2Cursor::S2Cursor(GfxCursor32 &kernelCursor) :
	GLCursor(kernelCursor, defaultCel),
	_noInventoryCel(defaultCel),
	_prayerStickNormalCel(defaultCel),
	_inventoryCel(defaultCel),
	_inventoryState(0) {
	setHighlightedCelRes(GLCelRes(99, 0, kHighlightCel));
	setHandsOffCelRes(GLCelRes(99, 0, kWaitCel));
	setPosition(Common::Point(320, 192));
}

void S2Cursor::getItem(const GLCelRes &celInfo) {
	if (!hasInventory()) {
		endHighlight();
		_inventoryCel = celInfo;
		if (!hasPrayerStick()) {
			_noInventoryCel = getNormalCelRes();
		}
		_inventoryState |= kInventory;
		setNormalCelRes(_inventoryCel);
	}
}

void S2Cursor::dropItem() {
	if (hasInventory()) {
		_inventoryState &= ~kInventory;
		if (hasPrayerStick()) {
			setNormalCelRes(_prayerStickNormalCel);
		} else {
			setNormalCelRes(_noInventoryCel);
		}
	}
}

void S2Cursor::getPrayerStick(const GLCelRes &celInfo) {
	if (!hasPrayerStick()) {
		endHighlight();
		_prayerStickNormalCel = celInfo;
		if (!hasInventory()) {
			_noInventoryCel = getNormalCelRes();
		}
		_inventoryState |= kPrayerStick;
		setNormalCelRes(celInfo);
	}
}

void S2Cursor::dropPrayerStick() {
	if (hasPrayerStick()) {
		_inventoryState &= ~kPrayerStick;
		if (hasInventory()) {
			setNormalCelRes(_inventoryCel);
		} else {
			setNormalCelRes(_noInventoryCel);
		}
	}
}

} // End of namespace Sci
