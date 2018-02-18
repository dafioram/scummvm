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

#ifndef SCI_S2_SYSTEM_GLCAST_H
#define SCI_S2_SYSTEM_GLCAST_H

#include "sci/s2/system/glset.h"

namespace Sci {

class GLCel;
class GLEvent;
class GLObject;
class GLTarget;
class GLScreenItem;

class GLCast {
public:
	void add(GLObject &object);
	// TODO: GLCel is added here to avoid UB-triggering behaviour when trying to
	// remove a GLCel from the cast, but virtual inheritance is probably more
	// appropriate
	void remove(GLCel &object);
	void remove(GLObject &object);
	void doIt();
	bool handleEvent(GLEvent &event);
	void addEventHandler(GLTarget &target);
	void removeEventHandler(GLTarget &target);

private:
	GLSetAsArray<GLObject> _doIts;
	GLSetAsArray<GLTarget> _eventHandlers;
	GLSetAsArray<GLScreenItem> _screenItems;
};

} // End of namespace Sci

#endif
