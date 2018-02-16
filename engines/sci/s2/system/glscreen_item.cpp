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

#include "sci/graphics/frameout.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/glscreen_item.h"

namespace Sci {

GfxFrameout *GLScreenItem::_graphicsManager = nullptr;

GLScreenItem::GLScreenItem(AbsGLPlane &plane, const uint16 viewNo, const int16 loopNo, const int16 celNo, const GLPoint &position, const int16 priority, const ScaleInfo &scaleInfo) :
	GLObject(),
	_viewNo(viewNo),
	_loopNo(loopNo),
	_celNo(celNo),
	_bitmap(nullptr),
	_position(position),
	_plane(&plane),
	_screenItem(new ScreenItem(plane.getId(), CelInfo32::makeView(viewNo, loopNo, celNo), position, scaleInfo)),
	_isDirty(false),
	_isVisible(false) {
	setNeedsDoIt(true);
	setIsScreenItem(true);
	plane.getCast().add(*this);
}

GLScreenItem::~GLScreenItem() {
	if (_screenItem && _isVisible) {
		_graphicsManager->deleteScreenItem(*_screenItem.release());
	}
	_plane->getCast().remove(*this);
}

void GLScreenItem::show() {
	if (_isVisible) {
		return;
	}

	if (_isDirty) {
		update();
	} else {
		_graphicsManager->addScreenItem(*_screenItem);
		_isVisible = true;
	}
}

void GLScreenItem::hide() {
	if (!_isVisible) {
		return;
	}

	warning("TODO: %s", __PRETTY_FUNCTION__);
	_graphicsManager->deleteScreenItem(*_screenItem.release());
	_isVisible = false;
}

void GLScreenItem::load(const GLCelRes &celInfo, const bool shouldUpdate) {
	_viewNo = celInfo.resourceId;
	_loopNo = celInfo.loopNo;
	_celNo = celInfo.celNo;
	_celInfo = celInfo;
	_screenItem->_celInfo = celInfo;
	_isDirty = true;
	if (shouldUpdate) {
		update();
	}
}

void GLScreenItem::update() {
	if (!_isDirty) {
		return;
	}

	if (!_isVisible) {
		_graphicsManager->addScreenItem(*_screenItem);
		_isVisible = true;
	} else {
		_graphicsManager->updateScreenItem(*_screenItem);
	}
	_isDirty = false;
}

void GLScreenItem::forceUpdate() {
	_isDirty = true;
	update();
}

bool GLScreenItem::getNowSeenRect(Common::Rect &result) const {
	const bool success = _graphicsManager->getNowSeenRect(_plane->getId(), _screenItem->_object, result);
	if (success) {
		--result.right;
		--result.bottom;
	}
	return success;
}

} // End of namespace Sci
