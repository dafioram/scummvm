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

#ifndef SCI_S2_HOTSPOT_H
#define SCI_S2_HOTSPOT_H

#include "sci/s2/system/glpoly.h"

namespace Sci {

class GLEvent;

class S2Hotspot : public GLPoly {
public:
	S2Hotspot(AbsGLPlane &plane, const int16 x1, const int16 y1, const int16 x2, const int16 y2);

	bool getIsEnabled() const { return _isEnabled; }

	void enable();
	void disable();

	bool handleEvent(GLEvent &event);

	const EventHandler &getMouseUpHandler() const { return _mouseUpHandler; }
	void setMouseUpHandler(const EventHandler &handler) { _mouseUpHandler = handler; }

private:
	bool _isEnabled;
	EventHandler _mouseUpHandler;

};

} // End of namespace Sci

#endif
