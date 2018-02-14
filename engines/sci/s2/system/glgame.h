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

#ifndef SCI_S2_SYSTEM_GLGAME_H
#define SCI_S2_SYSTEM_GLGAME_H

#include "sci/s2/game.h"

namespace Sci {

// In SSCI, S2Game inherited from GLGame and parts of the GL system code would
// reference that superclass type. Since we have only one implementation of
// GLGame, we make S2Game and GLGame into a single class. For conceptual
// clarity, GLGame is set up as an alias to S2Game so that there are not obvious
// references to S2 code from GL code.
using GLGame = S2Game;

} // End of namespace Sci

#endif
