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

	_plane = new Plane(pictureType, color, rect, priority, vanishingPoint, pictureNo, mirrored);

	_graphicsManager->addPlane(_plane);
	_plane->changePic();
}

AbsGLPlane::~AbsGLPlane() {
	_graphicsManager->deletePlane(*_plane);
}

void AbsGLPlane::setPriority(const int16 priority, const bool shouldUpdate) {
	_plane->_priority = priority;
	_isDirty = true;
	if (shouldUpdate) {
		update();
	}
}

void AbsGLPlane::setRect(const Common::Rect &rect, const bool shouldUpdate) {
	_plane->setGameRect(rect);
	_isDirty = true;
	if (shouldUpdate) {
		update();
	}
}

Common::Point AbsGLPlane::toGlobal(const Common::Point &point) const {
	return Common::Point(point.x + _plane->_gameRect.left, point.y + _plane->_gameRect.top);
}

Common::Point AbsGLPlane::toLocal(const Common::Point &point) const {
	return Common::Point(point.x - _plane->_gameRect.left, point.y - _plane->_gameRect.top);
}

void AbsGLPlane::repaint() {
	_graphicsManager->directFrameOut(_plane->_screenRect);
}

void AbsGLPlane::update() {
	_graphicsManager->updatePlane(*_plane);
	_isDirty = false;
}

GLPicturePlane::GLPicturePlane(const Common::Rect &rect, const uint16 resourceNo, const int16 priority, const bool mirrored, const GLPoint &vanishingPoint) :
	AbsGLPlane(kPlaneTypePicture, rect, priority, vanishingPoint, 0, resourceNo, mirrored) {}

void GLPicturePlane::setPic(const uint16 picNo, const bool shouldUpdate) {
	_plane->setPic(picNo);
	_isDirty = true;

	if (shouldUpdate) {
		update();
	}
}

void GLPicturePlane::deletePic(const uint16 picNo) {
	_plane->deletePic(picNo);
}

void GLPicturePlane::addPicAt(const uint16 resourceNo, const int16 x, const int16 y, const bool mirrorX, const bool deleteDuplicate) {
	_plane->addPic(resourceNo, Common::Point(x, y), mirrorX, deleteDuplicate);
}

GLTransparentPlane::GLTransparentPlane(const Common::Rect &rect, const int16 priority) :
	AbsGLPlane(kPlaneTypeTransparent, rect, priority, GLPoint(0, 0), 0) {}

GLColoredPlane::GLColoredPlane(const Common::Rect &rect, const uint8 color, const int16 priority) :
	AbsGLPlane(kPlaneTypeColored, rect, priority, GLPoint(0, 0), color) {}

GLOpaquePlane::GLOpaquePlane(const Common::Rect &rect, const uint8 color, const int16 priority) :
	AbsGLPlane(kPlaneTypeOpaque, rect, priority, GLPoint(0, 0), color) {}

} // End of namespace Sci
