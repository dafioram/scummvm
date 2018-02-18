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
#include "sci/s2/bitmap.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/glscreen_item.h"

namespace Sci {

GfxFrameout *GLScreenItem::_graphicsManager = nullptr;

GLScreenItem::GLScreenItem(AbsGLPlane &plane, const GLCelRes &celInfo, const GLPoint &position, const int16 priority, const ScaleInfo &scaleInfo) :
	GLObject(),
	_celInfo(celInfo),
	_position(position),
	_plane(&plane),
	_screenItem(new ScreenItem(plane.getId(), celInfo, position, scaleInfo)),
	_isDirty(false),
	_isVisible(false) {
	if (priority != -9999) {
		_screenItem->setPriority(priority);
	}
	setNeedsDoIt(true);
	setIsScreenItem(true);
	plane.getCast().add(*this);
}

GLScreenItem::GLScreenItem(AbsGLPlane &plane, const uint16 viewNo, const int16 loopNo, const int16 celNo, const GLPoint &position, const int16 priority, const ScaleInfo &scaleInfo) :
	GLScreenItem(plane, GLCelRes(viewNo, loopNo, celNo), position, priority, scaleInfo) {}

GLScreenItem::GLScreenItem(AbsGLPlane &plane, S2Bitmap &bitmap, const GLPoint &position, const int16 priority, const ScaleInfo &scaleInfo) :
	GLScreenItem(plane, GLCelRes(bitmap.getHandle()), position, priority, scaleInfo) {}

GLScreenItem::~GLScreenItem() {
	if (_screenItem && _isVisible) {
		_graphicsManager->deleteScreenItem(*_screenItem.release());
	}
	_plane->getCast().remove(*this);
}

void GLScreenItem::setCelRes(const GLCelRes &celInfo, const bool shouldUpdate) {
	load(celInfo, shouldUpdate);
}

void GLScreenItem::setLoop(const int16 loopNo, const bool shouldUpdate) {
	assert(_celInfo.type == kCelTypeView);
	_celInfo.loopNo = loopNo;
	load(_celInfo, shouldUpdate);
}

void GLScreenItem::setCel(const int16 celNo, const bool shouldUpdate) {
	assert(_celInfo.type == kCelTypeView);
	_celInfo.celNo = celNo;
	load(_celInfo, shouldUpdate);
}

int16 GLScreenItem::getLastCel() const {
	return CelObjView::getNumCels(_celInfo.resourceId, _celInfo.loopNo) - 1;
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

	ScreenItem *clonedItem = _screenItem->clone();
	_graphicsManager->deleteScreenItem(*_screenItem.release());
	_screenItem.reset(clonedItem);
	_isVisible = false;
}

void GLScreenItem::load(const GLCelRes &celInfo, const bool shouldUpdate) {
	_celInfo = celInfo;
	// SSCI did not check for inequality, and also did not reset the celObj
	if (_screenItem->_celInfo != celInfo) {
		_screenItem->_celInfo = celInfo;
		_screenItem->_celObj.reset();
	}
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
	return _graphicsManager->getNowSeenRect(_plane->getId(), _screenItem->_object, result);
}

} // End of namespace Sci
