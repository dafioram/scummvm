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

#ifndef SCI_S2_SYSTEM_GLVR_PLANE_H
#define SCI_S2_SYSTEM_GLVR_PLANE_H

#include "sci/s2/bitmap.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/glscreen_item.h"

namespace Sci {

class GLVRPlane : public GLOpaquePlane {
public:
	GLVRPlane(const Common::Rect &rect);

	void updateScreen();

private:
	S2Bitmap _bitmap;
	GLScreenItem _screenItem;
};

} // End of namespace Sci

#endif
