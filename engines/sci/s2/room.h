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

#ifndef SCI_S2_ROOM_H
#define SCI_S2_ROOM_H

#include "common/ptr.h"

namespace Sci {

class S2Game;
class S2Kernel;
class GLEvent;
class GLScript;

class S2Room {
public:
	S2Room(S2Kernel &kernel, S2Game &game) : _kernel(kernel), _game(game) {}
	virtual ~S2Room();
	virtual void init(const int roomNo) = 0;
	virtual void dispose(const int roomNo) = 0;
	virtual void doIt() = 0;
	virtual bool handleEvent(GLEvent &event) = 0;

protected:
	S2Kernel &_kernel;
	S2Game &_game;
	Common::ScopedPtr<GLScript> _script;
};

} // End of namespace Sci

#endif
