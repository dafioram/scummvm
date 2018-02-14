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

#ifndef SCI_S2_SYSTEM_GLPLANE_H
#define SCI_S2_SYSTEM_GLPLANE_H

#include "common/ptr.h"
#include "sci/engine/vm_types.h"
#include "sci/graphics/plane32.h"
#include "sci/s2/system/glcast.h"
#include "sci/s2/system/types.h"

namespace Common { struct Point; }

namespace Sci {

class AbsGLPlane {
public:
	static void init(GfxFrameout *graphicsManager) {
		_graphicsManager = graphicsManager;
	}

	AbsGLPlane(const PlaneType type, const Common::Rect &rect, const int16 priority, const GLPoint &vanishingPoint, const uint8 color, const int pictureNo = 0, const bool mirror = false);
	virtual ~AbsGLPlane();

	reg_t getId() const { return _plane->_object; }
	GLCast &getCast() { return _cast; }

	void toGlobal(Common::Point &point) const;
	void toLocal(Common::Point &point) const;

private:
	static GfxFrameout *_graphicsManager;

	// TODO: AbsGLPlane destructor should be calling to GfxFrameout to delete
	// the plane
	Common::ScopedPtr<Plane> _plane;
	GLCast _cast;
	bool _isDirty;
};

class GLPicturePlane : public AbsGLPlane {};

class GLTransparentPlane : public AbsGLPlane {
public:
	// This constructor signature is reduced vs SSCI, which had a bunch of
	// unused stuff
	GLTransparentPlane(const Common::Rect &rect, const int16 priority);
};

} // End of namespace Sci

#endif
