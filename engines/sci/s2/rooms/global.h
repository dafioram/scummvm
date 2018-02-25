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

#ifndef SCI_S2_ROOMS_GLOBAL_H
#define SCI_S2_ROOMS_GLOBAL_H

#include "common/array.h"
#include "common/ptr.h"
#include "common/rect.h"
#include "sci/s2/room.h"
#include "sci/s2/bitmap.h"
#include "sci/s2/button.h"
#include "sci/s2/system/glcel.h"
#include "sci/s2/system/glscreen_item.h"

namespace Sci {

class GLPicturePlane;

class S2GlobalRoom : public S2Room {
public:
	using S2Room::S2Room;
	virtual void init(const int roomNo) override;
	virtual bool handleEvent(GLEvent &event) override;

	void showOldGames(GLEvent &, GLTarget &);
	void quitGame(GLEvent &, GLTarget &);
	void returnToGame();

	int _lastRoomBeforeRestore;

private:
	virtual GLPicturePlane &getPlane() const override {
		return *_game.getRoomManager().getGlobalPlane();
	}
};

} // End of namespace Sci

#endif
