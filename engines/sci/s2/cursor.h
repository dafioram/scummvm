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

private:
	enum InventoryState {
		kNormal,
		kInventory,
		kPrayerStick
	};

	GLCelRes _normalCel;
	GLCelRes _prayerStickNormalCel;
	GLCelRes _inventoryCel;
	int _inventoryState;
};

} // End of namespace Sci

#endif
