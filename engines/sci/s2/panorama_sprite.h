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

#ifndef SCI_S2_PANORAMA_SPRITE_H
#define SCI_S2_PANORAMA_SPRITE_H

#undef abort
#undef FILE
#undef time
#undef mktime
#undef localtime
#undef gmtime
#undef getdate
#undef asctime
#undef difftime
#undef ctime
#undef clock
#include <functional>
#include "sci/s2/panorama_image.h"

namespace Sci {

class S2PanoramaSprite : public S2PanoramaImage {
public:
	using EventHandler = std::function<void()>;

	const EventHandler &getMouseDownHandler() const { return _mouseDownHandler; }
	void setMouseDownHandler(const EventHandler &handler) { _mouseDownHandler = handler; }

	const Common::Rect &getRect() { return _bounds; }

private:
	Common::Rect _bounds;
	EventHandler _mouseDownHandler;
};

} // End of namespace Sci

#endif
