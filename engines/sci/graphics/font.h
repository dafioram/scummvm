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

#ifndef SCI_GRAPHICS_FONT_H
#define SCI_GRAPHICS_FONT_H

#include "sci/graphics/helpers.h"
#include "sci/util.h"

namespace Sci {

#ifdef ENABLE_SCI32
enum {
	kSci32SystemFont = -1
};
#endif

class GfxFont {
public:
	virtual ~GfxFont() {}
	GuiResourceId getResourceId() const { return _fontId; }
	virtual uint8 getHeight() const = 0;
	virtual bool isDoubleByte(uint16 chr) const = 0;
	virtual uint8 getCharWidth(uint16 chr) const = 0;
	virtual void draw(uint16 chr, int16 top, int16 left, byte color, bool greyedOutput, GfxScreen *screen) const = 0;
#ifdef ENABLE_SCI32
	virtual void draw(uint16 chr, int16 top, int16 left, byte color, bool greyedOutput, byte *buffer, int16 width, int16 height) const = 0;
#endif

protected:
	GuiResourceId _fontId;
};

class GfxFontFromResource final : public GfxFont {
public:
	GfxFontFromResource(ResourceManager *resMan, const GuiResourceId fontId);
	GfxFontFromResource(const GfxFontFromResource &other);
	GfxFontFromResource &operator=(const GfxFontFromResource &other);
	virtual ~GfxFontFromResource();

	virtual uint8 getHeight() const override;
	virtual uint8 getCharWidth(uint16 chr) const override;
	virtual bool isDoubleByte(uint16 chr) const override { return false; }
	virtual void draw(uint16 chr, int16 top, int16 left, byte color, bool greyedOutput, GfxScreen *screen) const override;
#ifdef ENABLE_SCI32
	virtual void draw(uint16 chr, int16 top, int16 left, byte color, bool greyedOutput, byte *buffer, int16 width, int16 height) const override;
#endif

private:
	uint8 getCharHeight(uint16 chr) const;
	SciSpan<const byte> getCharData(uint16 chr) const;

	template <typename Renderer>
	void render(int charWidth, int charHeight, uint16 chr, int16 top, int16 left, byte color, bool greyedOutput, Renderer renderer) const;

	ResourceManager *_resMan;
	const Resource *_resource;
	SciSpan<const byte> _data;
	SciSpan<const uint16> _offsets;
	uint16 _numChars;
	uint8 _fontHeight;
};

} // End of namespace Sci

#endif
