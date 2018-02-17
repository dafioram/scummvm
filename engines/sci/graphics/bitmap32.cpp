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
#include "sci/graphics/celobj32.h"
#include "sci/engine/segment.h"

namespace Sci {

GfxBitmap32::GfxBitmap32(BitmapProvider *provider, GfxText32 *text) :
	_provider(provider),
	_gfxText32(text) {}

SciBitmap *GfxBitmap32::create(reg_t *addr, const int16 width, const int16 height, const uint8 skipColor, const uint8 backColor, const int16 originX, const int16 originY, const int16 xResolution, const int16 yResolution, const uint32 paletteSize, const bool useRemap, const bool gc) {
	SciBitmap *bitmap = _provider->allocateBitmap(addr, width, height, skipColor, 0, 0, xResolution, yResolution, 0, useRemap, gc);
	assert(bitmap);
	memset(bitmap->getPixels(), backColor, width * height);
	return bitmap;
}

void GfxBitmap32::destroy(reg_t bitmapId) {
	_provider->freeBitmap(bitmapId);
}

void GfxBitmap32::drawView(const reg_t bitmapId, const uint16 viewNo, const int16 loopNo, const int16 celNo, const int16 x, const int16 y, const int16 alignX, const int16 alignY) {
	SciBitmap &bitmap = *_provider->lookupBitmap(bitmapId);
	CelObjView view(viewNo, loopNo, celNo);

	Common::Point position(
		x == -1 ? bitmap.getOrigin().x : x,
		y == -1 ? bitmap.getOrigin().y : y
	);

	position.x -= alignX == -1 ? view._origin.x : alignX;
	position.y -= alignY == -1 ? view._origin.y : alignY;

	Common::Rect drawRect(
		position.x,
		position.y,
		position.x + view._width,
		position.y + view._height
	);
	drawRect.clip(Common::Rect(bitmap.getWidth(), bitmap.getHeight()));
	view.draw(bitmap.getBuffer(), drawRect, position, view._mirrorX);
}

void GfxBitmap32::drawText(const reg_t bitmapId, const Common::String &text, Common::Rect textRect, const uint8 foreColor, const uint8 backColor, const uint8 skipColor, const GuiResourceId fontId, TextAlign alignment, const int16 borderColor, const bool dimmed) {

	SciBitmap &bitmap = *_provider->lookupBitmap(bitmapId);
	textRect.clip(Common::Rect(bitmap.getWidth(), bitmap.getHeight()));
	const reg_t textBitmapObject = _gfxText32->createFontBitmap(textRect.width(), textRect.height(), Common::Rect(textRect.width(), textRect.height()), text, foreColor, backColor, skipColor, fontId, alignment, borderColor, dimmed, false, false);
	CelObjMem textCel(textBitmapObject);
	textCel.draw(bitmap.getBuffer(), textRect, Common::Point(textRect.left, textRect.top), false);
	_provider->freeBitmap(textBitmapObject);
}

} // End of namespace Sci
