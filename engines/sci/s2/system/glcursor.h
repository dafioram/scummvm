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

#ifndef SCI_S2_SYSTEM_GLCURSOR_H
#define SCI_S2_SYSTEM_GLCURSOR_H

#include "sci/s2/system/globject.h"
#include "sci/s2/system/types.h"

namespace Common { struct Point; }

namespace Sci {

class GfxCursor32;

class GLCursor : public GLObject {
public:
	GLCursor(GfxCursor32 &kernelCursor, const GLCelRes &celInfo);

	const GLCelRes &getHighlightedCelRes() { return _highlightedCel; }
	void setHighlightedCelRes(const GLCelRes &celInfo);
	void setHandsOffCelRes(const GLCelRes &celInfo);
	Common::Point getPosition() const { return _position; }
	void setPosition(const Common::Point &position);
	void show();
	void hide();

	const bool isHighlighted() const { return _state & kHighlightedState; }
	const bool isHandsOff() const { return _state & kHandsOffState; }

	void goHandsOn();
	void goHandsOff();

	void beginHighlight();
	void endHighlight();

protected:
	GfxCursor32 &_kernelCursor;

private:
	void updateKernel(const GLCelRes &celInfo) const;

	enum State {
		kNormalState      = 0,
		kHiddenState      = 1,
		kWaitState        = 2,
		kHandsOffState    = 4,
		kRestrictedState  = 8,
		kHighlightedState = 16
	};

	GLCelRes _normalCel;
	GLCelRes _waitCel;
	GLCelRes _handsOffCel;
	GLCelRes _highlightedCel;

	Common::Point _position;
	int _state;
};

} // End of namespace Sci

#endif
