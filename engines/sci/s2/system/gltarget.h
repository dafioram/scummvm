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

#ifndef SCI_S2_SYSTEM_GLTARGET_H
#define SCI_S2_SYSTEM_GLTARGET_H

#undef abort
#undef FILE
#undef time
#undef mktime
#undef localtime
#undef gmtime
#undef getdate
#undef asctime
#undef difftime
#undef ctime
#undef clock
#include <functional>
#include "sci/s2/system/globject.h"
#include "sci/s2/system/types.h"

namespace Sci {

class AbsGLPlane;
class GLUser;

class GLTarget : public GLObject {
public:
	using EventHandler = std::function<void(GLEvent &, GLTarget &)>;

	template <typename T, typename U>
	static EventHandler makeHandler(T object, U fn) {
		using namespace std::placeholders;
		return std::bind(fn, object, _1, _2);
	}

	GLTarget();
	GLTarget(AbsGLPlane &plane);

	static void init(GLUser *user) { _user = user; }

	void setSelectHandler(const EventHandler &handler) { _selectHandler = handler; }

	virtual bool handleEvent(GLEvent &event) override;
	virtual bool checkIsOnMe(const GLPoint &point) const { return false; }

protected:
	const EventHandler &getSelectHandler() const { return _selectHandler; }

	static GLUser *_user;

	AbsGLPlane *_plane;
	EventHandler _selectHandler;
};

} // End of namespace Sci

#endif
