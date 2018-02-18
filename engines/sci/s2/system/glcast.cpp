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

#include "common/textconsole.h"
#include "sci/s2/system/glcast.h"
#include "sci/s2/system/glcel.h"
#include "sci/s2/system/glevent.h"
#include "sci/s2/system/globject.h"
#include "sci/s2/system/glscreen_item.h"
#include "sci/s2/system/gltarget.h"

namespace Sci {

void GLCast::add(GLObject &object) {
	if (object.getIsScreenItem()) {
		_screenItems.push_back(&static_cast<GLScreenItem &>(object));
	}
	if (object.getNeedsDoIt()) {
		_doIts.push_back(&object);
	}
	if (object.getNeedsEvent()) {
		_eventHandlers.push_back(&static_cast<GLTarget &>(object));
	}
}

void GLCast::remove(GLCel &object) {
	if (static_cast<GLScreenItem &>(object).getIsScreenItem()) {
		_screenItems.remove(&object);
	}
	_doIts.remove(&static_cast<GLTarget &>(object));
	if (static_cast<GLTarget &>(object).getNeedsEvent()) {
		_eventHandlers.remove(&object);
	}
}

void GLCast::remove(GLObject &object) {
	// SSCI did not check whether things were screen items or targets before
	// passing pointers in; we do this to avoid UB. (It also checked to see if
	// the list contained the element before trying to remove the element, which
	// is not necessary.)
	if (object.getIsScreenItem()) {
		_screenItems.remove(&static_cast<GLScreenItem &>(object));
	}
	_doIts.remove(&object);
	if (object.getNeedsEvent()) {
		_eventHandlers.remove(&static_cast<GLTarget &>(object));
	}
}

void GLCast::doIt() {
	for (auto &object : _doIts) {
		object->doIt();
	}
}

bool GLCast::handleEvent(GLEvent &event) {
	for (auto &target : _eventHandlers) {
		target->handleEvent(event);
		if (event.isClaimed()) {
			break;
		}
	}

	return event.isClaimed();
}

void GLCast::addEventHandler(GLTarget &target) {
	if (!_eventHandlers.contains(&target)) {
		target.setNeedsEvent(true);
		_eventHandlers.push_back(&target);
	}
}

void GLCast::removeEventHandler(GLTarget &target) {
	// Check for existence moved down from call sites
	if (_eventHandlers.contains(&target)) {
		target.setNeedsEvent(false);
		_eventHandlers.remove(&target);
	}
}

} // End of namespace Sci
