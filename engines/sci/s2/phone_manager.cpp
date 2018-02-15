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

#include "sci/s2/phone_manager.h"

namespace Sci {

S2PhoneManager::S2PhoneManager() : GLScript() {}

void S2PhoneManager::init() {
	// Trying to initialise the script at construction time is invalid since the
	// object is not fully initialised so trying to capture it will result in
	// the wrong vtable being used; because S2PhoneManager is an inline member
	// of S2Game it also cannot be fully initialised immediately since GLCue has
	// to be able to access the global game object
	GLScript::init(GLScript::makeHandler(this, &S2PhoneManager::changeState));
}

void S2PhoneManager::cancelCall() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2PhoneManager::changeState(GLScript &script, const int state) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

} // End of namespace Sci
