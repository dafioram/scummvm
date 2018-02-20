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

#include "sci/graphics/bitmap32.h"
#include "sci/s2/bitmap.h"

namespace Sci {

GfxBitmap32 *S2Bitmap::_bitmapManager = nullptr;

S2Bitmap::S2Bitmap(const int16 width, const int16 height, const uint8 skipColor, const uint8 backColor, const bool remap) {
	_bitmapManager->create(&_handle, width, height, skipColor, backColor, 0, 0, 640, 480, 0, remap, false);
}

S2Bitmap::~S2Bitmap() {
	_bitmapManager->destroy(_handle);
}

void S2Bitmap::drawView(const uint16 viewNo, const int16 loopNo, const int16 celNo, const int16 x, const int16 y) {
	_bitmapManager->drawView(_handle, viewNo, loopNo, celNo, x, y, -1, -1);
}

void S2Bitmap::drawText(const Common::String &text, const Common::Rect &textRect, const uint8 foreColor, const uint8 backColor, const uint8 skipColor, const uint16 fontId, TextAlign alignment, const int16 borderColor, const bool dimmed) {
	_bitmapManager->drawText(_handle, text, textRect, foreColor, backColor, skipColor, fontId, alignment, borderColor, dimmed);
}

void S2Bitmap::fill(const Common::Rect &rect, const uint8 color) {
	_bitmapManager->fillRect(_handle, rect, color);
}

byte *S2Bitmap::getPixels() {
	return _bitmapManager->getRawBitmap(_handle)->getPixels();
}

} // End of namespace Sci
