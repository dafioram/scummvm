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

#include "sci/s2/system/glevent.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/glplane_manager.h"

namespace Sci {

void GLPlaneManager::doIt() {
	for (auto i = 0; i < _planes.size(); ++i) {
		_planes[i]->getCast().doIt();
	}
}

void GLPlaneManager::add(AbsGLPlane &plane) {
	const auto lowerPlane = Common::find_if(_planes.begin(), _planes.end(), [&](const AbsGLPlane *const candidate) {
		return candidate->getPriority() < plane.getPriority();
	});
	if (lowerPlane == _planes.end()) {
		_planes.insert_at(0, &plane);
	} else {
		_planes.insert(lowerPlane, &plane);
	}
}

void GLPlaneManager::remove(AbsGLPlane &plane) {
	_planes.remove(&plane);
}

bool GLPlaneManager::contains(AbsGLPlane &plane) const {
	return _planes.contains(&plane);
}

bool GLPlaneManager::handleEvent(GLEvent &event) {
	// TODO: SSCI did not account for mutations of the list in the case that a
	// element removed itself, so some cast members may get skipped
	for (auto i = 0; i < _planes.size(); ++i) {
		auto &&plane(_planes[i]);
		if (plane->checkIsOnMe(event.getMousePosition())) {
			event.localize(*plane);
			if (plane->getCast().handleEvent(event)) {
				break;
			}
			event.globalize();
		}
	}

	return event.isClaimed();
}

} // End of namespace Sci