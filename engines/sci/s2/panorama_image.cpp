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
#include "sci/resource/manager.h"

namespace Sci {

ResourceManager *S2PanoramaImage::_resourceManager = nullptr;

S2PanoramaImage::S2PanoramaImage(const Common::Rect &rect) :
	GLObject(),
	_width(rect.width()),
	_height(rect.height()),
	_pixels(nullptr),
	_isSprite(false) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
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

} // End of namespace Sci
