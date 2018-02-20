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

#ifndef SCI_GRAPHICS_BITMAP32_H
#define SCI_GRAPHICS_BITMAP32_H

#include "common/scummsys.h"
#include "common/str.h"
#include "sci/graphics/text32.h"

namespace Sci {

class BitmapProvider;
struct reg_t;
class SciBitmap;

class GfxBitmap32 {
public:
	GfxBitmap32(BitmapProvider *provider, GfxText32 *text32);

	SciBitmap *create(reg_t *addr, const int16 width, const int16 height, const uint8 skipColor, const uint8 backColor, const int16 originX, const int16 originY, const int16 xResolution, const int16 yResolution, const uint32 paletteSize, const bool useRemap, const bool gc);

	void destroy(reg_t bitmapId);

	void drawView(const reg_t bitmapId, const uint16 viewNo, const int16 loopNo, const int16 celNo, const int16 x, const int16 y, const int16 alignX, const int16 alignY);

	void drawText(const reg_t bitmapId, const Common::String &text, Common::Rect textRect, const uint8 foreColor, const uint8 backColor, const uint8 skipColor, const GuiResourceId fontId, TextAlign alignment, const int16 borderColor, const bool dimmed);

	void fillRect(const reg_t bitmapId, const Common::Rect &rect, const uint8 color);

	SciBitmap *getRawBitmap(const reg_t bitmapId);

private:
	BitmapProvider *_provider;
	GfxText32 *_gfxText32;
};

} // End of namespace Sci

#endif
