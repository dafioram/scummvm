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

#ifndef SCI_S2_SYSTEM_GLEVENT_H
#define SCI_S2_SYSTEM_GLEVENT_H

#include "common/rect.h"
#include "sci/event.h"

namespace Sci {

class AbsGLPlane;
class GLPoint;

class GLEvent {
public:
	typedef SciEventType EventType;

	SciEventType type;
	uint16 message;
	SciKeyModifiers modifiers;
	bool claimed;
	Common::Point point;

	void refresh() {
		refresh(kSciEventAny);
	}

	void refresh(const SciEventType mask) {
		const SciEvent event = _eventManager->getSciEvent(mask);
		type = event.type;
		message = event.character;
		modifiers = event.modifiers;
		claimed = false;
		_plane = nullptr;
		point = event.mousePos;
	}

	void localize(const AbsGLPlane &plane);
	void globalize();

private:
	static EventManager *_eventManager;

	const AbsGLPlane *_plane;
};

} // End of namespace Sci

#endif
