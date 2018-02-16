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
	AbsGLPlane(const AbsGLPlane &) = delete;
	virtual ~AbsGLPlane();

	reg_t getId() const { return _plane->_object; }

	int16 getPriority() const { return _plane->_priority; }
	void setPriority(const int16 priority, const bool shouldUpdate = false);

	Common::Rect getRect() const {
		Common::Rect inclusiveRect(_plane->_gameRect);
		--inclusiveRect.right;
		--inclusiveRect.bottom;
		return inclusiveRect;
	}

	GLCast &getCast() { return _cast; }

	Common::Point toGlobal(const Common::Point &point) const;
	Common::Point toLocal(const Common::Point &point) const;

	bool checkIsOnMe(const Common::Point &point) const {
		return _plane->_gameRect.contains(point);
	}

	void repaint();

protected:
	static GfxFrameout *_graphicsManager;

	void update();

	// This plane object is owned by GfxFrameout
	Plane *_plane;
	GLCast _cast;
	bool _isDirty;
};

class GLPicturePlane : public AbsGLPlane {
public:
	GLPicturePlane(const Common::Rect &rect, const uint16 resourceNo, const int16 priority = -9999, const bool mirrored = false, const GLPoint &vanishingPoint = GLPoint(0, 0));

	uint16 getPicNo() const { return _plane->getPictureId(); }
	void setPic(const uint16 picNo, const bool shouldUpdate = false);
	void deletePic(const uint16 picNo);

	void addPicAt(const uint16 resourceNo, const int16 x, const int16 y, const bool mirrorX = false, const bool deleteDuplicate = true);
};

class GLTransparentPlane : public AbsGLPlane {
public:
	// This constructor signature is reduced vs SSCI, which had a bunch of
	// unused stuff
	GLTransparentPlane(const Common::Rect &rect, const int16 priority);
};

} // End of namespace Sci

#endif
