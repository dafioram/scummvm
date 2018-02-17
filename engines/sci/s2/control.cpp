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

#include "sci/s2/control.h"
#include "sci/s2/system/glcel.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/gluser.h"
#include "sci/s2/text_button.h"

namespace Sci {

GLUser *S2Control::_user = nullptr;

S2Control::S2Control() :
	_isVisible(false),
	_priority(0) {}

S2Control::~S2Control() {
	_user->getOrphans().remove(this);
	// TODO: Ownership semantics in container
	for (auto &button : _buttons) {
		delete button;
	}
}

void S2Control::show() {
	for (auto &cel : _cels) {
		if (!cel->getIsVisible()) {
			cel->show();
			cel->getPlane().getCast().removeEventHandler(*cel);
		}
	}
	for (auto &button : _buttons) {
		if (!button->getIsVisible()) {
			button->show();
			button->getPlane().getCast().removeEventHandler(*button);
		}
	}
	_isVisible = true;
	_user->getOrphans().push_back(this);
}

void S2Control::hide() {
	for (auto &cel : _cels) {
		cel->hide();
	}
	for (auto &button : _buttons) {
		button->hide();
	}
	_isVisible = false;
	_user->getOrphans().remove(this);
}

bool S2Control::handleEvent(GLEvent &event) {
	bool eventWasLocalized = false;
	if (!_cels.empty()) {
		if (event.getPlane() != &_cels.front()->getPlane()) {
			eventWasLocalized = true;
			event.localize(_cels.front()->getPlane());
		}
		if (!event.isClaimed()) {
			for (auto &cel : _cels) {
				cel->GLTarget::handleEvent(event);
				if (event.isClaimed()) {
					controlEvent(event, *cel);
					break;
				}
			}
		}
		if (eventWasLocalized) {
			event.globalize();
		}
	}

	eventWasLocalized = false;
	if (!_buttons.empty()) {
		if (event.getPlane() != &_buttons.front()->getPlane()) {
			eventWasLocalized = true;
			event.localize(_buttons.front()->getPlane());
		}
		if (!event.isClaimed()) {
			for (auto &button : _buttons) {
				button->GLTarget::handleEvent(event);
				if (event.isClaimed()) {
					controlEvent(event, *button);
					break;
				}
			}
		}
		if (eventWasLocalized) {
			event.globalize();
		}
	}

	return event.isClaimed();
}

void S2Control::addCel(GLCel &cel) {
	setRect(cel.getRect());
	_cels.emplace_back(&cel);
	cel.getPlane().getCast().removeEventHandler(cel);
	if (_priority < cel.getPriority()) {
		_priority = cel.getPriority();
	}
}

void S2Control::addButton(S2TextButton &button) {
	setRect(button.getRect());
	_buttons.push_back(&button);
	button.getPlane().getCast().removeEventHandler(button);
	if (_priority < button.getPriority()) {
		_priority = button.getPriority();
	}
}

void S2Control::setRect(const Common::Rect &rect) {
	if (_cels.empty() && _buttons.empty()) {
		_boundingBox = rect;
	} else {
		_boundingBox.extend(rect);
	}
}

} // End of namespace Sci
