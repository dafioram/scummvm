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

#ifndef SCI_S2_SYSTEM_GLUSER_H
#define SCI_S2_SYSTEM_GLUSER_H

#include "sci/s2/system/glevent.h"
#include "sci/s2/system/glevent_handler_set.h"
#include "sci/s2/system/globject.h"

namespace Sci {

class S2Game;
using GLGame = S2Game;

class GLUser : public GLObject {
public:
	GLUser(GLGame &game);

	bool getIsHandsOn() const { return _state & kIsHandsOn; }
	void setIsHandsOn(const bool set) { updateState(kIsHandsOn, set); }
	bool getHandlesNulls() const { return _state & kHandlesNulls; }
	void setHandlesNulls(const bool set) { updateState(kHandlesNulls, set); }
	bool getHogsAreModal() const { return _state & kHogsAreModal; }
	void setHogsAreModal(const bool set) { updateState(kHogsAreModal, set); }

	const Common::Point &getMousePosition() const { return _mousePosition; }

	virtual void doIt() override;
	virtual bool handleEvent(GLEvent &event) override;

	GLEventHandlerSet &getPrimaDonnas() { return _primaDonnas; }
	GLEventHandlerSet &getOrphans() { return _orphans; }

private:
	enum State {
		kIsHandsOn    = 1,
		kHandlesNulls = 2,
		kHogsAreModal = 4
	};

	bool handleHandsOff(GLEvent &event);
	bool processHogs(GLEvent &event);

	inline void updateState(const State value, const bool set) {
		if (set) {
			_state |= value;
		} else {
			_state &= ~value;
		}
	}

	GLGame &_game;
	Common::Point _mousePosition;
	GLEvent _event;
	GLEventHandlerSet _hogs;
	GLEventHandlerSet _primaDonnas;
	GLEventHandlerSet _orphans;
	int _state;
};

} // End of namespace Sci

#endif
