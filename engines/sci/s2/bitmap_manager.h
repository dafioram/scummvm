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

#ifndef SCI_S2_BITMAP_MANAGER_H
#define SCI_S2_BITMAP_MANAGER_H

#include "sci/engine/segment.h"
#include "sci/engine/vm_types.h"

namespace Sci {

class SciBitmap;

class BitmapManager : public BitmapProvider {
public:
	virtual SciBitmap *allocateBitmap(reg_t *addr, const int16 width, const int16 height, const uint8 skipColor, const int16 originX, const int16 originY, const int16 xResolution, const int16 yResolution, const uint32 paletteSize, const bool remap, const bool gc) override;
	virtual SciBitmap *lookupBitmap(const reg_t addr) override;
	virtual void freeBitmap(const reg_t addr) override;

private:
	BitmapTable _table;
};

} // End of namespace Sci

#endif
