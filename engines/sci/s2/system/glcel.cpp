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
#include "sci/s2/system/glcel.h"
#include "sci/s2/system/glplane.h"

namespace Sci {

GLCel::GLCel(AbsGLPlane &plane, const uint16 viewNo, const int16 loopNo, const int16 celNo, const GLPoint &position, const int16 priority, const ScaleInfo &scaleInfo) :
	GLScreenItem(plane, viewNo, loopNo, celNo, position, priority, scaleInfo),
	GLFeature(plane),
	_cycleSpeed(6),
	_moveSpeed(6),
	_stepSize(3, 3) {
	setIsScreenItem(true);
	setNeedsDoIt(true);
	setNeedsEvent(true);
	// SSCI called setBounds here but the underlying ScreenItem will never be
	// part of the graphics system yet so this call would always fail
	GLFeature::init();
}
GLCel::GLCel(AbsGLPlane &plane, const GLCelRes &celInfo, const GLPoint &position, const int16 priority, const ScaleInfo &scaleInfo) :
	GLCel(plane, celInfo.resourceId, celInfo.loopNo, celInfo.celNo, position, priority, scaleInfo) {}

void GLCel::show() {
	getPlane().getCast().addEventHandler(*this);
	GLScreenItem::show();
}

void GLCel::hide() {
	getPlane().getCast().removeEventHandler(*this);
	GLScreenItem::hide();
}

void GLCel::update() {
	GLScreenItem::update();
	setBounds();
}

void GLCel::setBounds() {
	Common::Rect bounds;
	if (getNowSeenRect(bounds)) {
		setRect(bounds);
	}
}

} // End of namespace Sci
