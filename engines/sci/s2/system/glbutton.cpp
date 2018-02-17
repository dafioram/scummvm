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

#include "sci/s2/system/glbutton.h"
#include "sci/s2/system/glevent.h"

namespace Sci {
GLButton::GLButton(AbsGLPlane &plane, const uint16 viewNo, const int16 loopNo, const int16 celNo, const GLPoint &position, const int16 priority) :
	GLCel(plane, viewNo, loopNo, celNo, position, priority),
	_isEnabled(false),
	_isHighlighted(false),
	_isDepressed(false),
	_isHog(false),
	_mouseUpHandler(nullptr) {
	setEnabledFace(viewNo, loopNo);
	setDisabledFace(viewNo, loopNo);
	setDepressedFace(viewNo, loopNo);
	setHighlightedFace(viewNo, loopNo);
}

bool GLButton::handleEvent(GLEvent &event) {
	if (event.getType() & kSciEventMouse) {
		if (getSelectHandler()) {
			getSelectHandler()(event, *this);
		} else {
			generalSelect(event);
		}
	}

	return event.isClaimed();
}

void GLButton::enable(const bool shouldUpdate) {
	_isEnabled = true;
	changeCel(_enabledCel, shouldUpdate);
}

void GLButton::disable(const bool shouldUpdate) {
	_isEnabled = false;
	changeCel(_disabledCel, shouldUpdate);
}

void GLButton::press(const bool shouldUpdate) {
	_isDepressed = true;
	changeCel(_depressedCel, shouldUpdate);
}

void GLButton::release(const bool shouldUpdate) {
	_isDepressed = false;
	changeCel(_enabledCel, shouldUpdate);
}

void GLButton::highlight(const bool shouldUpdate) {
	_isHighlighted = true;
	changeCel(_highlightedCel, shouldUpdate);
}

void GLButton::dim(const bool shouldUpdate) {
	_isHighlighted = false;
	changeCel(_enabledCel, shouldUpdate);
}

void GLButton::generalSelect(GLEvent &event) {
	if (_isEnabled && checkIsOnMe(event.getMousePosition())) {
		if (event.getType() == kSciEventMousePress) {
			press();
		} else if (event.getType() == kSciEventMouseRelease && _isDepressed) {
			release();
			if (_mouseUpHandler) {
				_mouseUpHandler(event, *this);
			}
		}
		event.claim();
	} else if (event.getType() == kSciEventMouseRelease && _isDepressed) {
		release();
		event.claim();
	}
}

void GLButton::setEnabledFace(const uint16 viewNo, const int16 loopNo, const int16 celNo) {
	_enabledCel = GLCelRes::makeView(viewNo, loopNo, celNo);
}

void GLButton::setDepressedFace(const uint16 viewNo, const int16 loopNo, const int16 celNo) {
	_depressedCel = GLCelRes::makeView(viewNo, loopNo, celNo);
}

void GLButton::setDisabledFace(const uint16 viewNo, const int16 loopNo, const int16 celNo) {
	_disabledCel = GLCelRes::makeView(viewNo, loopNo, celNo);
}

void GLButton::setHighlightedFace(const uint16 viewNo, const int16 loopNo, const int16 celNo) {
	_highlightedCel = GLCelRes::makeView(viewNo, loopNo, celNo);
}

void GLButton::changeCel(const GLCelRes &celInfo, const bool shouldUpdate) {
	if (hasVLC()) {
		load(celInfo, shouldUpdate);
	} else if (shouldUpdate) {
		forceUpdate();
	}
}

} // End of namespace Sci
