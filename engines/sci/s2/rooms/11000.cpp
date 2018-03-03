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

#include "sci/s2/rooms/11000.h"

namespace Sci {

void S2Room11000::init(const int roomNo) {
	switch (roomNo) {
	case 11100: {
		enterFrom(6311, 1846, 11112);
		room().drawPan(11100);
		sound().createAmbient(11);
		flags().set(kGameFlag91);

		if (inventory().isPlaced(S2Inventory::kInv7)) {
			emplaceSprite(false, 11101, GLPoint(1814, 245));
		}

		addPanoramaExit(11200, 110, 222, 240, 500);
		addPanoramaExit(11101, 860, 103, 1213, 511, S2Cursor::kHighlightCel);

		int exitNo;
		if (inventory().isPlaced(S2Inventory::Batteries)) {
			exitNo = 11150;
		} else {
			exitNo = 11110;
		}
		addPanoramaExit(exitNo, 470, 226, 600, 295, S2Cursor::kHighlightCel);
		addPanoramaExit(11120, 693, 259, 769, 442, S2Cursor::kHighlightCel);
		addPanoramaExit(11130, 1584, 210, 1649, 280, S2Cursor::kHighlightCel);
		addPanoramaExit(11140, 1732, 222, 1851, 267, S2Cursor::kHighlightCel);
		break;
	}

	case 11101:
		room().drawPic(11101);
		enter(11100, 11111, 11112, true);
		emplaceExit(true, 6310, 253, 51, 421, 383);
		break;

	case 11110:
		room().drawPic(11110);
		emplaceExit(true, 11100, 64, 303, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 11100, 64, 0, 445, 80, S2Cursor::kBackCel);
		if (inventory().isTaken(S2Inventory::Batteries) ||
			inventory().isUsed(S2Inventory::Batteries)) {
		} else {

		}
		warning("TODO: Finish room 11110");
		break;

	case 11120:
		room().drawPic(11120);
		exitBorder(11100);
		break;

	case 11121:
		room().drawPic(11121);
		emplaceExit(true, 11120, S2Cursor::kBackCel);
		score().doEvent(kScore82);
		break;

	case 11130:
		room().drawPic(11130);
		emplaceExit(true, 11100, S2Cursor::kBackCel);
		score().doEvent(kScore79);
		break;

	case 11140:
		room().drawPic(11140);
		phone().cancelCall();
		exitBorder(11141);

		warning("TODO: Finish room 11140");
		break;

	case 11141:
	case 11150:
	case 11199:
		warning("TODO: Finish room %d", roomNo);
		break;

	case 11200:
		room().drawPan(11200);
		if (inventory().isPlaced(S2Inventory::kInv7)) {
			emplaceSprite(false, 11201, GLPoint(1349, 246));
		}
		addPanoramaExit(11100, 1084, 211, 1170, 360);
		addPanoramaExit(11300, 1473, 188, 1634, 452);
		addPanoramaExit(11210, 187, 227, 260, 278, S2Cursor::kHighlightCel);
		break;

	case 11210:
	case 11211:
		warning("TODO: Finish room %d", roomNo);
		break;

	case 11300:
		room().drawPan(11300);
		enterFrom(11311, -1, 12305);
		addPanoramaExit(11200, 186, 183, 320, 508);
		addPanoramaExit(11330, 666, 308, 755, 363, S2Cursor::kHighlightCel);
		addPanoramaExit(11341, 378, 316, 453, 372, S2Cursor::kHighlightCel);
		addPanoramaExit(11332, 778, 331, 821, 369, S2Cursor::kHighlightCel);
		addPanoramaExit(11311, 1713, 279, 1739, 311, S2Cursor::kHighlightCel);

		if (inventory().isPlaced(S2Inventory::kInv7)) {
			emplaceSprite(false, 11301, GLPoint(1172, 331));
			addPanoramaExit(11301, 1170, 333, 1241, 365, S2Cursor::kHighlightCel);
		}
		break;

	case 11301:
		if (inventory().setState(S2Inventory::kInv7, S2InventoryState::Taken)) {
			sound().play(11115, false, 80);
			inventory().addItem(S2Inventory::kInv7);
		} else {
			emplaceSprite(false, 11301, GLPoint(1171, 330));
		}
		room().newRoom(11300);
		break;

	case 11311:
	case 11330:
	case 11331:
	case 11332:
	case 11341:
	case 11350:
	case 11351:
	case 11352:
	case 11353:
	case 11355:
	case 11999:
		warning("TODO: Finish room %d", roomNo);
		break;

	default:
		warning("Unimplemented room %d", roomNo);
	}
}

void S2Room11000::dispose(const int roomNo) {
	S2PhoneRoom::dispose(roomNo);
}

bool S2Room11000::handleEvent(GLEvent &event) {
	if (room().getCurrentRoomNo() == 11331) {
		S2PhoneRoom::handleEvent(event);
	}
	return false;
}

} // End of namespace Sci
