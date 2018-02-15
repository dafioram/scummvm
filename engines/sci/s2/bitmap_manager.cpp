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

#include "sci/engine/segment.h"
#include "sci/s2/bitmap_manager.h"

namespace Sci {

SciBitmap *BitmapManager::allocateBitmap(reg_t *addr, const int16 width, const int16 height, const uint8 skipColor, const int16 originX, const int16 originY, const int16 xResolution, const int16 yResolution, const uint32 paletteSize, const bool remap, const bool gc) {
	uint offset = _table.allocEntry();
	*addr = make_reg(0, offset + 1);
	SciBitmap &bitmap = _table.at(offset);
	bitmap.create(width, height, skipColor, originX, originY, xResolution, yResolution, paletteSize, remap, gc);
	return &bitmap;
}

SciBitmap *BitmapManager::lookupBitmap(const reg_t addr) {
	const uint32 index = addr.getOffset() - 1;
	if (!_table.isValidEntry(index)) {
		return nullptr;
	}
	return &_table.at(index);
}

void BitmapManager::freeBitmap(const reg_t addr) {
	_table.freeEntry(addr.getOffset() - 1);
}

} // End of namespace Sci
