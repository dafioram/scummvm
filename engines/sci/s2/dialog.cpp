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

#include "sci/event.h"
#include "sci/graphics/frameout.h"
#include "sci/s2/control.h"
#include "sci/s2/dialog.h"
#include "sci/s2/system/glevent.h"
#include "sci/s2/system/gluser.h"

namespace Sci {

GLUser *S2Dialog::_user = nullptr;
GfxFrameout *S2Dialog::_graphicsManager = nullptr;

S2Dialog::S2Dialog(const Common::Rect &rect, const uint8 color) :
	GLColoredPlane(rect, color),
	GLObject(),
	_result(Result::None),
	_isModeless(false) {
	setPriority(255);
}

S2Dialog::~S2Dialog() {
	if (_isModeless) {
		hide();
		_user->getOrphans().remove(this);
	}
	// TODO: Get ownership semantics into the container
	for (auto &control : _controls) {
		delete control;
	}
}

auto S2Dialog::createS2Dialog() -> Result {
	_user->getOrphans().push_front(this);
	show();
	_graphicsManager->kernelFrameOut(true);
	while (_result == Result::None) {
		GLEvent event;
		event.refresh();
		event.localize(*this);
		if (event.getType() == kSciEventQuit) {
			event.claim();
			break;
		} else if (event.getType() & kSciEventMouse) {
			for (auto &control : _controls) {
				control->handleEvent(event);
				if (event.isClaimed()) {
					dialogEvent(event, control);
					_graphicsManager->frameOut(true);
					break;
				}
			}
		// SSCI did not allow keyboard interaction with the dialogue; we add
		// this for convenience
		} else if (event.getType() == kSciEventKeyDown) {
			dialogEvent(event, nullptr);
		}
		if (!event.isClaimed() && handleEvent(event)) {
			_graphicsManager->frameOut(true);
		}
		event.globalize();
		doIt();
		_graphicsManager->kernelFrameOut(true);
	}
	hide();
	_user->getOrphans().remove(this);
	return _result;
}

void S2Dialog::addControl(S2Control &control) {
	_controls.push_back(&control);
	_user->getOrphans().remove(&control);
}

void S2Dialog::show() {
	for (auto &control : _controls) {
		control->show();
		_user->getOrphans().remove(control);
	}
	if (_isModeless) {
		_user->getOrphans().push_front(this);
		setPriority(5, true);
	} else {
		setPriority(255, true);
	}
}

void S2Dialog::hide() {
	setPriority(-1, true);
	for (auto &control : _controls) {
		control->hide();
	}
}

bool S2Dialog::handleEvent(GLEvent &event) {
	if (_isModeless) {
		if ((event.getType() & kSciEventMouse) && checkIsOnMe(event.getMousePosition())) {
			event.localize(*this);
			if (!event.isClaimed()) {
				for (auto &control : _controls) {
					control->handleEvent(event);
					if (event.isClaimed()) {
						dialogEvent(event, control);
						break;
					}
				}
			}
			event.claim();
			event.globalize();
		}
	}
	return event.isClaimed();
}

} // End of namespace Sci
