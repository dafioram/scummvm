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

#include "common/textconsole.h"
#include "image/bmp.h"
#include "sci/s2/panorama_image.h"
#include "sci/s2/panorama_sprite.h"
#include "sci/resource/manager.h"

namespace Sci {

ResourceManager *S2PanoramaImage::_resourceManager = nullptr;

S2PanoramaImage::S2PanoramaImage(const Common::Rect &rect) :
	GLObject(),
	_width(rect.width()),
	_height(rect.height()),
	_pixels(nullptr),
	_isSprite(false) {}

S2PanoramaImage::S2PanoramaImage(const uint16 resourceNo) :
	GLObject(),
	_isSprite(false),
	_isDrawn(false) {
	loadImage(resourceNo);
}

void S2PanoramaImage::loadImage(const uint16 panoramaNo) {
	const Resource *resource = _resourceManager->findResource(ResourceId(kResourceTypePano, panoramaNo), false);
	// SSCI would just return early if the resource was not found
	assert(resource);

	Image::BitmapDecoder decoder;
	Common::MemoryReadStream stream(resource->toStream());
	const bool success = decoder.loadStream(stream);
	assert(success);

	const Graphics::Surface &surface(*decoder.getSurface());

	_width = surface.h;
	_height = surface.w;

	_ownedPixels.resize(surface.w * surface.h);
	_pixels = _ownedPixels.data();
	for (int y = surface.h - 1; y >= 0; --y) {
		byte *target = _pixels + y + surface.h * (surface.w - 1);
		const byte *source = static_cast<const byte *>(surface.getBasePtr(0, y));
		for (int x = 0; x < surface.w; ++x) {
			*target = *source++;
			target -= surface.h;
		}
	}
}

void S2PanoramaImage::calculateDimensions(S2PanoramaSprite &source, int16 &targetX, int16 &targetY, int &skipX, int &paddingX) const {
	targetX = source.getPosition().y;
	targetY = 2048 - source.getPosition().x - source.getHeight();
	if (targetY < 0) {
		targetY += 2048;
	}
	skipX = 0;
	paddingX = 0;
	if (targetX < 0) {
		skipX = -targetX;
	}
	if (targetX + source.getCelWidth() > 512) {
		paddingX = targetX + source.getCelWidth() - 512;
	}
}

void S2PanoramaImage::draw(S2PanoramaSprite &source) {
	int16 targetX, targetY;
	int skipX, paddingX;
	calculateDimensions(source, targetX, targetY, skipX, paddingX);

	source.setIsDrawn(true);

	auto &savedPixels = source.getSavedPixels();
	savedPixels.resize(source.getCelWidth() * source.getCelHeight());

	byte *savePixel = savedPixels.data();

	byte *targetPixel = _pixels + _width * targetY + targetX + skipX;
	const byte *sourcePixel = source.getPixels() + skipX + source.getCel() * source.getCelWidth();

	const byte transparentColor = *sourcePixel;

	for (int y = 0; y < source.getCelHeight(); ++y) {
		for (int x = 0; x < source.getCelWidth() - skipX - paddingX; ++x) {
			*savePixel++ = *targetPixel;
			if (source.getHasTransparency() && *sourcePixel == transparentColor) {
				++targetPixel;
				++sourcePixel;
			} else {
				*targetPixel++ = *sourcePixel++;
			}
		}

		sourcePixel += source.getWidth() - source.getCelWidth() + skipX + paddingX;

		if (targetY + y == getHeight()) {
			targetPixel = _pixels + targetX + skipX;
		} else {
			targetPixel += _width - source.getCelWidth() + skipX + paddingX;
		}
	}
}

void S2PanoramaImage::erase(S2PanoramaSprite &source) {
	if (!source.getIsDrawn()) {
		return;
	}

	int16 targetX, targetY;
	int skipX, paddingX;
	calculateDimensions(source, targetX, targetY, skipX, paddingX);

	auto &savedPixels = source.getSavedPixels();

	byte *targetPixel = _pixels + _width * targetY + targetX + skipX;
	const byte *sourcePixel = savedPixels.data();

	for (int y = 0; y < source.getCelHeight(); ++y) {
		for (int x = 0; x < source.getCelWidth() - skipX - paddingX; ++x) {
			*targetPixel++ = *sourcePixel++;
		}

		if (targetY + y == getHeight()) {
			targetPixel = _pixels + targetX + skipX;
		} else {
			targetPixel += _width - source.getCelWidth() + skipX + paddingX;
		}
	}

	// SSCI did not reset the flag
	source.setIsDrawn(false);
}

} // End of namespace Sci
