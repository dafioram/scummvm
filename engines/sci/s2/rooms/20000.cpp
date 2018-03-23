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

#include "sci/s2/rooms/20000.h"

namespace Sci {

#define self(name) this, &S2Room20000::name

void S2Room20000::init(const int roomNo) {
	switch (roomNo) {
	case 20100:
		sound().createAmbient(20);
		flags().set(kGameFlag96);
		enterFrom(6251, 1842, 21802);
		room().drawPan(20100);
		addPanoramaExit(20200, 0, 179, 173, 452);
		addPanoramaExit(20200, 1907, 179, 2047, 452);
		addPanoramaExit(20110, 957, 196, 1094, 400, S2Cursor::kHighlightCel);
		break;

	case 20110:
		room().drawPic(20110);
		enter(roomNo, 21801, 21802, true);
		emplaceExit(true, 6250, 258, 114, 399, 364);
		break;

	case 20200:
		room().drawPan(20200);
		addPanoramaExit(20100, 876, 235, 1022, 382);
		addPanoramaExit(20300, 1880, 219, 2047, 343);
		addPanoramaExit(20300, 0, 219, 116, 343);
		addPanoramaExit(20400, 218, 182, 337, 385);
		addPanoramaExit(20210, 1692, 252, 1760, 312, S2Cursor::kHighlightCel);
		break;

	case 20210:
		room().drawPic(20210);
		emplaceExit(true, 20999, 64, 0, 575, 80, S2Cursor::kBackCel);
		emplaceExit(true, 20999, 64, 329, 575, 383, S2Cursor::kBackCel);
		_exitSoundNo = 12007;
		_boxHotspot = &emplaceHotspot(true, 401, 179, 459, 221);
		_boxHotspot->setMouseUpHandler([&](GLEvent &, GLTarget &target) {
			// moved up from callee
			removeChild(static_cast<S2Hotspot &>(target));

			const auto showPawn = (flags().get(kGameFlag51) &&
								   inventory().isPlaced(S2Inventory::kInv25));
			setScript(self(openBox), 0, showPawn);
		});
		break;

	case 20300: {
		room().drawPan(20300);
		if (flags().get(kGameFlag183)) {
			emplaceSprite(false, 20301, GLPoint(1471, 252));
		}
		enterFrom(20740, -1, 22102);

		// SSCI set a flag here which was never read

		addPanoramaExit(20200, 910, 210, 1040, 375);
		addPanoramaExit(20600, 232, 172, 335, 359);
		addPanoramaExit(20500, 1478, 204, 1596, 377);

		int exitNo;
		if (flags().get(kGameFlag185)) {
			exitNo = 20310;
		} else if (allKeysUsed()) {
			exitNo = 20330;
		} else {
			exitNo = 20320;
		}
		addPanoramaExit(exitNo, 1920, 227, 1979, 326);
		break;
	}

	case 20310:
		room().drawPic(20310);
		enter(roomNo, 22101, 22102, true);
		emplaceExit(true, 20700, 238, 103, 407, 381);
		break;

	case 20320: {
		room().drawPic(20320);

		struct Key {
			S2Inventory item;
			int16 x1, y1, x2, y2;
		};

		constexpr Key keys[] = {
			{ S2Inventory::kInv26, 274, 104, 308, 135 },
			{ S2Inventory::kInv15, 277, 147, 312, 179 },
			{ S2Inventory::kInv6, 274, 188, 310, 217 },
			{ S2Inventory::kInv18, 276, 225, 313, 255 }
		};

		exitBorder(20300);

		for (auto i = 0; i < ARRAYSIZE(keys); ++i) {
			const auto &key = keys[i];
			GLCel *cel = nullptr;
			if (inventory().isUsed(key.item)) {
				cel = &emplaceCel(false, 20320, i, 0, GLPoint(68, i < 2 ? 374 : 372));
				cel->show();
				getPlane().getCast().remove(*cel);
			}

			emplaceHotspot(true, key.x1, key.y1, key.x2, key.y2).setMouseUpHandler([this, i, cel, item = key.item](GLEvent &, GLTarget &target) {
				if (!inventory().isInUse(item)) {
					return;
				}

				sound().play(12001 + i, false, 100);
				if (cel) {
					removeChild(*cel);
				}
				removeChild(static_cast<S2Hotspot &>(target));
				inventory().setState(item, S2InventoryState::Used);
				if (allKeysUsed()) {
					// SSCI set a flag here which was never read
					room().setNextRoomNo(20330);
				}
			});
		}
		break;
	}

	case 20330:
		room().drawPic(20330);
		exitBorder(20300);
		warning("TODO: Room %d", roomNo);
		break;

	case 20400:
		room().drawPan(20400);
		if (flags().get(kGameFlag183)) {
			emplaceSprite(false, 20401, GLPoint(1580, 267));
		}

		addPanoramaExit(20200, 1431, 209, 1578, 409);
		addPanoramaExit(20410, 520, 205, 745, 330);
		addPanoramaExit(20420, 0, 272, 67, 300, S2Cursor::kHighlightCel);
		addPanoramaExit(20420, 1992, 270, 2047, 297, S2Cursor::kHighlightCel);
		addPanoramaExit(20430, 1051, 239, 1091, 297, S2Cursor::kHighlightCel);
		addPanoramaExit(20440, 925, 278, 1073, 373, S2Cursor::kHighlightCel);
		addPanoramaExit(20450, 168, 239, 283, 270, S2Cursor::kHighlightCel);
		break;

	case 20410:
		room().drawPic(20410);
		exitBorder(20400);
		emplaceExit(true, 20410, 196, 52, 213, 124, S2Cursor::kHighlightCel);
		emplaceExit(true, 20413, 326, 208, 341, 261, S2Cursor::kHighlightCel);
		break;

	case 20411:
		room().drawPic(20411);
		exitBorder(20500, false);
		score().doEvent(kScore178);
		emplaceHotspot(true, 197, 39, 428, 372).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			warning("TODO: 20411");
		});
		break;

	case 20412:
		room().drawPic(20412);
		exitBorder(20410, false);
		score().doEvent(kScore177);
		flags().set(kGameFlag160);
		emplaceHotspot(true, 197, 39, 428, 372).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			warning("TODO: 20412");
		});
		break;

	case 20413:
		room().drawPic(20414);
		exitBorder(20410, false);
		break;

	case 20420:
		room().drawPic(20420);
		emplaceExit(true, 20400, S2Cursor::kBackCel);
		score().doEvent(kScore124);
		break;

	case 20430:
		room().drawPic(20430);
		emplaceExit(true, 20400, S2Cursor::kBackCel);
		emplaceExit(true, 20431, 244, 114, 395, 251, S2Cursor::kHighlightCel);
		break;

	case 20431:
		room().drawPic(20431);
		emplaceExit(true, 20400, S2Cursor::kBackCel);
		score().doEvent(kScore125);
		break;

	case 20440:
		room().drawPic(20440);
		enterFrom(20441, -1, 12015);
		enterFrom(20442, -1, 12015);
		enterFrom(20443, -1, 12015);
		emplaceExit(true, 20400, 64, 0, 129, 383, S2Cursor::kBackCel);
		emplaceExit(true, 20400, 130, 0, 494, 80, S2Cursor::kBackCel);
		emplaceExit(true, 20400, 495, 0, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 20441, 130, 120, 220, 154, S2Cursor::kHighlightCel);
		emplaceExit(true, 20441, 222, 116, 236, 138, S2Cursor::kHighlightCel);
		emplaceExit(true, 20442, 252, 120, 332, 153, S2Cursor::kHighlightCel);
		emplaceExit(true, 20443, 336, 339, 502, 383, S2Cursor::kHighlightCel);
		break;

	case 20441:
		score().doEvent(kScore126);
		// fall through
	case 20442:
	case 20443:
		room().drawPic(20440);
		emplaceExit(true, 20440, S2Cursor::kBackCel);
		sound().play(12012, false, 100);
		emplaceCel(false, 20440, 0, roomNo - 20441, roomBottom).show();
		break;

	case 20450:
		room().drawPic(20450);
		emplaceExit(true, 20400, 64, 0, 575, 80, S2Cursor::kBackCel);
		emplaceExit(true, 20400, 64, 303, 575, 383, S2Cursor::kBackCel);
		emplaceExit(true, 20451, 461, 153, 547, 213, S2Cursor::kHighlightCel);
		emplaceExit(true, 20452, 84, 141, 159, 226, S2Cursor::kHighlightCel);
		emplaceExit(true, 20453, 194, 142, 282, 223, S2Cursor::kHighlightCel);
		emplaceExit(true, 20454, 308, 124, 426, 240, S2Cursor::kHighlightCel);
		break;

	case 20451:
		score().doEvent(kScore123);
		// fall through
	case 20452:
	case 20453:
	case 20454:
		room().drawPic(roomNo);
		emplaceExit(true, 20450, S2Cursor::kBackCel);
		break;

	case 20500:
		room().drawPan(20500);
		addPanoramaExit(20300, 493, 111, 729, 485);
		if (flags().get(kGameFlag183)) {
			emplaceSprite(false, 20501, GLPoint(1223, 103));
			emplaceSprite(true, 20504, GLPoint(719, 421));
			addPanoramaExit(20411, 726, 426, 794, 464, S2Cursor::kHighlightCel);
		} else {
			addPanoramaExit(20510, 1315, 189, 1540, 427);
		}
		break;

	case 20501:
		_cel.reset();
		room().drawPic(2);
		emplaceSprite(true, 20504, GLPoint(719, 421));
		addPanoramaExit(20300, 493, 111, 729, 485);
		addPanoramaExit(20411, 726, 426, 794, 464, S2Cursor::kHighlightCel);
		if (flags().get(kGameFlag183)) {
			emplaceSprite(false, 20501, GLPoint(1223, 103));
		}
		if (!flags().get(kGameFlag22)) {
			flags().set(kGameFlag22);
			room().drawPic(2);
			movie().play(5040);
			room().getPanorama().setPanX(300);
			room().drawPan(20500);
		}
		break;

	case 20510:
		room().drawPic(20510);
		emplaceExit(true, 20500, 204, 13, 458, 371, S2Cursor::kBackCel);
		emplaceHotspot(true, 524, 120, 544, 185).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			warning("TODO: 20510");
		});
		break;

	case 20600:
		room().drawPan(20600);
		if (flags().get(kGameFlag183)) {
			emplaceSprite(false, 20601, GLPoint(1383, 244));
		}
		addPanoramaExit(20300, 1244, 130, 1457, 363);
		addPanoramaExit(20610, 1883, 233, 1990, 390, S2Cursor::kHighlightCel);
		break;

	case 20610:
		room().drawPic(20610);
		emplaceExit(true, 20600, S2Cursor::kBackCel);
		emplaceExit(true, 20611, 270, 163, 313, 203, S2Cursor::kHighlightCel);
		break;

	case 20611:
		room().drawPic(20611);
		emplaceExit(true, 20600, S2Cursor::kBackCel);
		break;

	case 20700:
		room().drawPan(20700);
		if (!inventory().isPlaced(S2Inventory::kInv3)) {
			emplaceSprite(false, 20701, GLPoint(1807, 351));
		}
		addPanoramaExit(20740, 835, 110, 1080, 493);
		addPanoramaExit(20800, 427, 199, 510, 340);
		addPanoramaExit(20710, 1536, 273, 1651, 290, S2Cursor::kHighlightCel);
		addPanoramaExit(20730, 1191, 355, 1283, 443, S2Cursor::kHighlightCel);
		if (inventory().isPlaced(S2Inventory::kInv3)) {
			addPanoramaExit(20720, 1814, 349, 1844, 369, S2Cursor::kHighlightCel);
		}
		warning("TODO: Flags in 20700");
		break;

	case 20710:
		room().drawPic(20710);
		emplaceExit(true, 20700, S2Cursor::kBackCel);
		emplaceExit(true, 20711, 331, 156, 516, 251, S2Cursor::kHighlightCel);
		break;

	case 20711: {
		room().drawPic(20711);
		emplaceExit(true, 20700, S2Cursor::kBackCel);
		score().doEvent(kScore127);
		auto &cel = emplaceCel(false, 20711, 0, 0, roomBottom);
		cel.show();
		getPlane().getCast().remove(cel);
		emplaceHotspot(true, 201, 232, 426, 302).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			warning("TODO: 20711");
		});
		emplaceHotspot(true, 201, 232, 426, 302).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			warning("TODO: 20711");
		});
		break;
	}

	case 20720:
		room().drawPic(20720);
		emplaceExit(true, 20700, S2Cursor::kBackCel);
		emplaceHotspot(true, 286, 267, 446, 311).setMouseUpHandler([&](GLEvent &, GLTarget &) {
			warning("TODO: 20720");
		});
		break;

	case 20730:
		room().drawPic(20730);
		emplaceExit(true, 20700, S2Cursor::kBackCel);
		break;

	case 20740:
		room().drawPic(20740);
		enter(roomNo, 22101, 22102, true);
		emplaceExit(true, 20300, 254, 25, 441, 308);
		break;

	case 20800:
		room().drawPan(20800);
		addPanoramaExit(20700, 1357, 231, 1462, 439);
		addPanoramaExit(20811, 459, 187, 527, 259, S2Cursor::kHighlightCel);
		break;

	case 20811:
		room().drawPic(20811);
		emplaceCel(false, 20811, 0, 0, roomBottom).show();
		emplaceExit(true, 20800, S2Cursor::kBackCel);
		if (!flags().get(kGameFlag184)) {
			warning("TODO: Script 20811");
		}
		break;

	case 20999:
		if (_cel) {
			setScript(self(cancelScript));
		} else {
			switch (room().getPreviousRoomNo()) {
			case 20210:
				room().newRoom(20200);
				break;

			default:
				error("Stuck in room 20999");
			}
		}
		break;

	default:
		error("Invalid room %d", roomNo);
	}
}

void S2Room20000::cancelScript(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		_cycler.reset(new GLEndBackCycler());
		_cycler->add(*_cel);
		_cycler->start(script);
		sound().play(_exitSoundNo, false, 100);
		break;

	case 1:
		getPlane().getCast().remove(*_cel);
		_cel->hide();
		script.setSeconds(1);
		break;

	case 2:
		_cel.reset();
		_cycler.reset();
		_script.reset();

		user().setIsHandsOn(true);

		switch (room().getPreviousRoomNo()) {
		case 20210:
			_boxIsOpen = false;
			// fall through
		case 20110:
		case 20310:
		case 20740:
			room().setNextRoomNo(room().getPreviousRoomNo() / 100 * 100);
			break;
		default:
			error("Stuck in room 20999");
		}
	}
}

bool S2Room20000::allKeysUsed() {
	return (inventory().isUsed(S2Inventory::kInv26) &&
			inventory().isUsed(S2Inventory::kInv15) &&
			inventory().isUsed(S2Inventory::kInv6) &&
			inventory().isUsed(S2Inventory::kInv18));
}

void S2Room20000::openBox(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		uint16 soundNo;
		if (!_boxIsOpen) {
			int16 loopNo;
			soundNo = 12006;
			if (script.getData()) {
				loopNo = 0;
			} else {
				loopNo = 1;
			}
			_boxHotspot->setPoints(490, 267, 545, 312);
			_cel.reset(new GLCel(getPlane(), 20210, loopNo, 0, roomBottom));
			_cel->show();
			_cycler.reset(new GLEndCycler(*_cel, script));
		} else {
			soundNo = 12007;
			_boxHotspot->setPoints(401, 179, 459, 221);
			_cycler.reset(new GLEndBackCycler(*_cel, script));
		}
		_boxIsOpen = !_boxIsOpen;
		sound().play(soundNo, false, 100);
		break;

	case 1:
		getPlane().getCast().remove(*_cel);
		if (!_boxIsOpen) {
			_cel.reset();
		}
		_script.reset();
		_cycler.reset();
		user().setIsHandsOn(true);
		break;
	}
}

} // End of namespace Sci
