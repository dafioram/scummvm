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

#include "common/textconsole.h"
#include "sci/s2/game.h"
#include "sci/s2/rooms/1000.h"
#include "sci/s2/system/types.h"

namespace Sci {

void S2Room1000::init(const int roomNo) {
	switch (roomNo) {
	case 1000:
		_game.getRoomManager().drawPan(10400);
		_game.getRoomManager().drawPic(1, true);
		_script.reset(new GLScript(GLScript::makeHandler(this, &S2Room1000::logoScript)));
		break;
	case 1010:
	case 1015:
	case 1020:
	case 1500:
	case 1600:
		warning("TODO: Room %d", roomNo);
		break;
	}
}

void S2Room1000::dispose(const int roomNo) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

bool S2Room1000::handleEvent(GLEvent &event) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
	return false;
}

void S2Room1000::logoScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_game.getMovieManager().play(1000, nullptr, GLPoint(0, 0));
		script.setTicks(1);
		break;
	case 1:
		_game.getRoomManager().loadGlobalRoom(4000, true);
		break;
	}
}

} // End of namespace Sci
