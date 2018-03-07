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

#include "sci/s2/rooms/50000.h"

namespace Sci {

#define self(name) this, &S2Room50000::name

void S2Room50000::init(const int roomNo) {
	switch (roomNo) {
	case 50000:
	case 50100:
		sound().createAmbient(50);
		enterFrom(48200, 241, 0);
		enterFrom(50500, 1314, 0);
		room().drawPan(50000);
		addPanoramaExit(48200, 1445, 238, 1589, 455);
		addPanoramaExit(50500, 424, 207, 553, 404);
		break;

	case 50500:
		enterFrom(50000, 14, 0);
		enterFrom(50100, 14, 0);
		enterFrom(50600, 1016, 0);
		room().drawPan(50500);
		addPanoramaExit(50000, 1070, 152, 1361, 511);
		addPanoramaExit(50600, 155, 218, 270, 356);
		break;

	case 50600:
		enterFrom(50500, 330, 0);
		room().drawPan(50600);
		addPanoramaExit(50500, 1396, 169, 1645, 430);
		addPanoramaExit(50800, 909, 182, 1099, 414);
		addPanoramaExit(50700, 0, 174, 115, 440);
		addPanoramaExit(50700, 1925, 174, 2047, 440);
		break;

	case 50700:
		sound().createAmbient(50);
		enterFrom(26110, 660, 12602);
		room().drawPan(50700);
		if (!_sprite) {
			uint16 spriteNo;
			int16 celNo;
			if (flags().get(kGameFlag220)) {
				spriteNo = 50703;
				celNo = 0;
			} else {
				spriteNo = 50701;
				celNo = 4;
			}
			_sprite.reset(new S2PanoramaSprite(spriteNo, GLPoint(500, 150), celNo, 5, true));
			room().getPanorama().addSprite(*_sprite);
		}
		addPanoramaExit(50600, 908, 173, 1145, 477);
		if (flags().get(kGameFlag220)) {
			if (!_boxIsOpen) {
				addPanoramaExit(50701, 505, 233, 604, 338, S2Cursor::kHighlightCel);
			}
		} else {
			if (_boxIsOpen) {
				if (_skullIsOpen) {
					addPanoramaExit(50710, 121, 187, 230, 396);
				} else {
					addPanoramaExit(50702, 505, 233, 604, 338, S2Cursor::kHighlightCel);
				}
			} else {
				addPanoramaExit(50701, 505, 233, 604, 338, S2Cursor::kHighlightCel);
			}
		}
		break;

	case 50701:
		setScript(self(openBox));
		break;

	case 50702:
		if (checkPrayerStick()) {
			setScript(self(moveSkull));
		} else {
			setScript(self(stuckSkull));
		}
		break;

	case 50710:
		room().drawPic(50710);
		enter(50300, 12601, 12602, true);
		emplaceExit(true, 26100, 237, 101, 612, 383);
		break;

	case 50800:
		room().drawPan(50800);
		addPanoramaExit(50600, 0, 162, 126, 424);
		addPanoramaExit(50600, 1941, 162, 2047, 424);
		break;

	case 50999:
		setScript(self(cancelScript));
		break;

	default:
		error("Invalid room %d", roomNo);
	}
}

void S2Room50000::dispose(const int roomNo) {
	switch (roomNo) {
	case 50700:
	case 50710: {
		const auto nextRoomNo = room().getNextRoomNo();
		if (nextRoomNo < 50700 || nextRoomNo > 50799) {
			_skullIsOpen = _boxIsOpen = false;
			room().getPanorama().removeSprite(*_sprite);
			_sprite.reset();
			_panoramaCycler.reset();
		}
	}
	}

	if (room().getNextRoomNo() != 50999) {
		_cel.reset();
	}

	S2Room::dispose(roomNo);
}

void S2Room50000::cancelScript(GLScript &script, const int state) {
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
		_script.reset();
		_cycler.reset();
		switch (room().getPreviousRoomNo()) {
		case 50710:
			room().setNextRoomNo(50700);
			user().setIsHandsOn(true);
			break;
		default:
			error("Stuck in cancel room 50999");
		}
		break;
	}
}

void S2Room50000::openBox(GLScript &script, const int state) {
	switch (state) {
	case 0:
		if (flags().get(kGameFlag220)) {
			_panoramaCycler.reset(new S2PanoramaEndForwardCycler());
		} else {
			_panoramaCycler.reset(new S2PanoramaEndBackCycler());
		}
		script.setCycles(1);
		break;

	case 1:
		_panoramaCycler->add(*_sprite);
		_panoramaCycler->start(script);
		sound().play(10601, false, 100);
		break;

	case 2:
		if (!flags().get(kGameFlag19) && flags().get(kGameFlag220)) {
			flags().set(kGameFlag19);
			room().drawPic(2);
			movie().play(5010, nullptr, roomTop);
			flags().set(kGameFlag62);
		}
		_script.reset();
		_panoramaCycler.reset();
		_boxIsOpen = true;
		room().newRoom(50700);
		break;
	}
}

void S2Room50000::moveSkull(GLScript &script, const int state) {
	score().doEvent(kScore201);
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		_sprite1 = &emplaceSprite(true, 50702, GLPoint(490, 233), 0, 18);
		_panoramaCycler.reset(new S2PanoramaEndResetCycler());
		script.setCycles(1);
		break;

	case 1:
		_panoramaCycler->add(*_sprite1);
		sound().play(12609, false, 100);
		_panoramaCycler->start(script);
		break;

	case 2:
		_panoramaCycler.reset();
		_script.reset();
		_skullIsOpen = true;
		user().setIsHandsOn(true);
		room().newRoom(50710);
		break;
	}
}

void S2Room50000::stuckSkull(GLScript &script, const int state) {
	switch (state) {
	case 0:
		user().setIsHandsOn(false);
		_sprite1 = &emplaceSprite(true, 50702, GLPoint(490, 233), 0, 18);
		_panoramaCycler.reset(new S2PanoramaCycleToCycler(2));
		script.setCycles(1);
		break;

	case 1:
		_panoramaCycler->add(*_sprite1);
		_panoramaCycler->start(script);
		sound().play(10613, false, 100);
		break;

	case 2:
		_panoramaCycler.reset();
		script.setCycles(1);
		break;

	case 3:
		_panoramaCycler.reset(new S2PanoramaEndBackCycler());
		_panoramaCycler->add(*_sprite1);
		_panoramaCycler->start(script);
		break;

	case 4:
		_panoramaCycler.reset();
		_script.reset();
		_skullIsOpen = false;
		user().setIsHandsOn(true);
		room().newRoom(50700);
		break;
	}
}

bool S2Room50000::checkPrayerStick() {
	using PS = S2PrayerStick;
	switch (inventory().getPrayerStickId()) {
	case PS::Arrowhead:
		return flags().get(kGameFlag212);
	case PS::Claw:
		return flags().get(kGameFlag205);
	case PS::Bird:
		return flags().get(kGameFlag211);
	case PS::Hourglass:
		return flags().get(kGameFlag206);
	case PS::Jack:
		return flags().get(kGameFlag216);
	case PS::Cow:
		return flags().get(kGameFlag210);
	case PS::Star:
		return flags().get(kGameFlag214);
	case PS::Spider:
		return flags().get(kGameFlag208);
	case PS::Rain:
		return flags().get(kGameFlag209);
	case PS::Butterfly:
		return flags().get(kGameFlag213);
	case PS::Stairs:
		return flags().get(kGameFlag207);
	case PS::Shield:
		return flags().get(kGameFlag215);
	default:
		return false;
	}
}

} // End of namespace Sci
