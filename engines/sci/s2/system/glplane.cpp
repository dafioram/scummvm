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

#include "common/rect.h"
#include "common/textconsole.h"
#include "sci/graphics/frameout.h"
#include "sci/s2/system/glplane.h"

namespace Sci {

GfxFrameout *AbsGLPlane::_graphicsManager = nullptr;

AbsGLPlane::AbsGLPlane(const PlaneType type, const Common::Rect &rect, int16 priority, const GLPoint &vanishingPoint, const uint8 color, const int pictureNo, const bool mirrored) :
	_isDirty(false) {

	// This is AbsGLPlane::Init

	PlanePictureCodes pictureType;
	switch (type) {
	case kPlaneTypeColored:
		pictureType = kPlanePicColored;
		break;
	case kPlaneTypeTransparent:
		pictureType = kPlanePicTransparent;
		break;
	case kPlaneTypePicture:
		pictureType = kPlanePicTransparentPicture;
		break;
	case kPlaneTypeOpaque:
		pictureType = kPlanePicOpaque;
		break;
	default:
		error("Invalid plane type %d", type);
	}

	if (priority == -9999) {
		// TODO: This is weird since there is never any SCI priority?
		priority = _graphicsManager->getPlanes().getTopSciPlanePriority() + 1;
	}

	Common::Rect exclusiveRect(rect);
	++exclusiveRect.left;
	++exclusiveRect.bottom;

	_plane.reset(new Plane(pictureType, color, exclusiveRect, priority, vanishingPoint, pictureNo, mirrored));

	_cast = GLCast();
	_graphicsManager->addPlane(_plane.get());
	_plane->changePic();
}

AbsGLPlane::~AbsGLPlane() {
	// TODO: How could there be no plane, since it is created in the
	// constructor? Can it actually just be made a member of AbsGLPlane?
	if (_plane) {
		_graphicsManager->deletePlane(*_plane);
	}
}

GLTransparentPlane::GLTransparentPlane(const Common::Rect &rect, const int16 priority) :
	AbsGLPlane(kPlaneTypeTransparent, rect, priority, GLPoint(0, 0), 0) {}

void AbsGLPlane::toGlobal(Common::Point &point) const {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void AbsGLPlane::toLocal(Common::Point &point) const {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

} // End of namespace Sci
