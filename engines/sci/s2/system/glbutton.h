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

#ifndef SCI_S2_SYSTEM_GLBUTTON_H
#define SCI_S2_SYSTEM_GLBUTTON_H

#include "sci/s2/system/glcel.h"
#include "sci/s2/system/types.h"

namespace Sci {

class AbsGLPlane;
class GLEvent;
class GLTarget;

class GLButton : public GLCel {
public:
	GLButton(AbsGLPlane &plane, const uint16 viewNo, const int16 loopNo, const int16 celNo, const GLPoint &position, const int16 priority);

	bool getIsEnabled() const { return _isEnabled; }
	bool getIsHighlighted() const { return _isHighlighted; }
	bool getIsDepressed() const { return _isDepressed; }

	void setEnabledFace(const uint16 viewNo, const int16 loopNo, const int16 celNo = 0);
	void setDisabledFace(const uint16 viewNo, const int16 loopNo, const int16 celNo = 2);
	void setDepressedFace(const uint16 viewNo, const int16 loopNo, const int16 celNo = 1);
	void setHighlightedFace(const uint16 viewNo, const int16 loopNo, const int16 celNo = 3);

	void setMouseUpHandler(const EventHandler &handler) { _mouseUpHandler = handler; }

	void enable(const bool shouldUpdate = true);
	void disable(const bool shouldUpdate = true);

protected:
	void press(const bool shouldUpdate = true);
	void release(const bool shouldUpdate = true);
	void highlight(const bool shouldUpdate = true);
	void dim(const bool shouldUpdate = true);

private:
	void changeCel(const GLCelRes &celInfo, const bool shouldUpdate);

	bool _isEnabled;
	bool _isHighlighted;
	bool _isDepressed;
	bool _isHog;

	GLCelRes _enabledCel;
	GLCelRes _disabledCel;
	GLCelRes _depressedCel;
	GLCelRes _highlightedCel;

	EventHandler _mouseUpHandler;
};

} // End of namespace Sci

#endif
