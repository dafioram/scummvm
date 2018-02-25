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

#ifndef SCI_S2_BITMAP_H
#define SCI_S2_BITMAP_H

#include "common/scummsys.h"
#include "sci/engine/vm_types.h"
#include "sci/graphics/text32.h"

namespace Common {
struct Rect;
class String;
}

namespace Sci {

class GfxBitmap32;
class SciBitmap;

class S2Bitmap {
public:
	static void init(GfxBitmap32 *bitmapManager) { _bitmapManager = bitmapManager; }

	S2Bitmap(const int16 width, const int16 height, const uint8 skipColor, const uint8 backColor, const bool remap = false);
	S2Bitmap(const S2Bitmap &) = delete;
	S2Bitmap &operator=(const S2Bitmap &) = delete;
	S2Bitmap(S2Bitmap &&);
	S2Bitmap &operator=(S2Bitmap &&);
	~S2Bitmap();

	reg_t getHandle() const { return _handle; }

	void drawView(const uint16 viewNo, const int16 loopNo, const int16 celNo, const int16 x, const int16 y);

	void drawText(const Common::String &text, const Common::Rect &textRect, const uint8 foreColor, const uint8 backColor, const uint8 skipColor, const uint16 fontId, TextAlign alignment = kTextAlignDefault, const int16 borderColor = -1, const bool dimmed = false);

	void fill(const Common::Rect &rect, const uint8 color);

	byte *getPixels();

private:
	static GfxBitmap32 *_bitmapManager;

	reg_t _handle;
};

} // End of namespace Sci

#endif
