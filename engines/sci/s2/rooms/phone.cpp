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

#include "sci/s2/rooms/phone.h"
#include "sci/s2/system/glevent.h"

namespace Sci {

void S2PhoneRoom::setUpPhone(const int roomNo) {
	_game.getPhoneManager().setMotelState(0);
	_number.clear();

	if (_game.getSoundManager().isPlaying(20009)) {
		_game.getSoundManager().stop(20009);
		_game.getPhoneManager().setState(47);
		_game.getPhoneManager().cue();
	}

	_game.getSoundManager().play(20014, false, 80);

	switch (roomNo) {
	case 10411:
		emplaceDigit(0, 318, 266, 343, 290);
		emplaceDigit(1, 295, 180, 316, 199);
		emplaceDigit(2, 325, 180, 344, 199);
		emplaceDigit(3, 353, 180, 374, 199);
		emplaceDigit(4, 293, 207, 313, 229);
		emplaceDigit(5, 322, 207, 342, 229);
		emplaceDigit(6, 352, 207, 372, 229);
		emplaceDigit(7, 292, 235, 311, 257);
		emplaceDigit(8, 321, 235, 341, 257);
		emplaceDigit(9, 352, 235, 373, 257);
		break;

	case 11331:
		emplaceDigit(280, 329, 255, 356, 280);
		emplaceDigit(188, 294, 166, 319, 188);
		emplaceDigit(188, 331, 166, 355, 188);
		emplaceDigit(188, 365, 166, 391, 188);
		emplaceDigit(217, 293, 193, 317, 217);
		emplaceDigit(217, 331, 193, 357, 217);
		emplaceDigit(217, 365, 193, 392, 217);
		emplaceDigit(247, 291, 223, 317, 247);
		emplaceDigit(247, 329, 224, 356, 247);
		emplaceDigit(248, 366, 224, 392, 248);
		break;

	case 12411:
		emplaceDigit(290, 318, 266, 343, 290);
		emplaceDigit(199, 295, 180, 316, 199);
		emplaceDigit(199, 325, 180, 344, 199);
		emplaceDigit(199, 353, 180, 374, 199);
		emplaceDigit(229, 293, 207, 313, 229);
		emplaceDigit(229, 322, 207, 342, 229);
		emplaceDigit(229, 352, 207, 372, 229);
		emplaceDigit(257, 292, 235, 311, 257);
		emplaceDigit(257, 321, 235, 341, 257);
		emplaceDigit(257, 352, 235, 373, 257);
		break;

	case 13320:
		emplaceDigit(301, 334, 279, 357, 301);
		emplaceDigit(214, 301, 193, 324, 214);
		emplaceDigit(214, 332, 193, 352, 214);
		emplaceDigit(214, 362, 193, 382, 214);
		emplaceDigit(242, 302, 221, 324, 242);
		emplaceDigit(242, 332, 221, 353, 242);
		emplaceDigit(242, 362, 221, 383, 242);
		emplaceDigit(270, 302, 250, 323, 270);
		emplaceDigit(270, 333, 250, 353, 270);
		emplaceDigit(270, 364, 250, 385, 270);
		break;

	case 14251:
		emplaceDigit(301, 339, 281, 363, 301);
		emplaceDigit(214, 308, 191, 331, 214);
		emplaceDigit(214, 339, 191, 363, 214);
		emplaceDigit(214, 369, 191, 392, 214);
		emplaceDigit(242, 308, 221, 331, 242);
		emplaceDigit(242, 339, 221, 363, 242);
		emplaceDigit(242, 369, 221, 392, 242);
		emplaceDigit(273, 308, 251, 331, 273);
		emplaceDigit(273, 339, 251, 363, 273);
		emplaceDigit(273, 369, 251, 392, 273);
		break;

	case 15371:
		emplaceDigit(295, 371, 275, 393, 295);
		emplaceDigit(216, 343, 196, 365, 216);
		emplaceDigit(216, 371, 196, 393, 216);
		emplaceDigit(216, 399, 196, 421, 216);
		emplaceDigit(243, 343, 223, 365, 243);
		emplaceDigit(243, 371, 223, 393, 243);
		emplaceDigit(243, 399, 223, 421, 243);
		emplaceDigit(269, 343, 249, 365, 269);
		emplaceDigit(269, 371, 249, 393, 269);
		emplaceDigit(269, 399, 249, 391, 269);
		break;

	case 16923:
		emplaceDigit(326, 398, 305, 421, 326);
		emplaceDigit(235, 364, 214, 386, 235);
		emplaceDigit(235, 394, 214, 415, 235);
		emplaceDigit(235, 424, 214, 444, 235);
		emplaceDigit(264, 364, 243, 387, 264);
		emplaceDigit(264, 395, 243, 416, 264);
		emplaceDigit(264, 424, 243, 445, 264);
		emplaceDigit(295, 367, 273, 388, 295);
		emplaceDigit(295, 396, 273, 417, 295);
		emplaceDigit(295, 427, 273, 448, 295);
		break;

	case 17140:
		emplaceDigit(292, 349, 268, 374, 292);
		emplaceDigit(201, 315, 181, 338, 201);
		emplaceDigit(201, 349, 181, 370, 201);
		emplaceDigit(201, 378, 181, 401, 201);
		emplaceDigit(229, 317, 209, 338, 229);
		emplaceDigit(229, 347, 209, 371, 229);
		emplaceDigit(229, 380, 209, 402, 229);
		emplaceDigit(260, 316, 237, 340, 260);
		emplaceDigit(260, 350, 237, 372, 260);
		emplaceDigit(260, 381, 237, 404, 260);
		break;

	case 18410:
		emplaceDigit(289, 361, 268, 386, 289);
		emplaceDigit(200, 327, 179, 350, 200);
		emplaceDigit(201, 359, 180, 381, 201);
		emplaceDigit(200, 390, 180, 413, 200);
		emplaceDigit(228, 328, 208, 350, 228);
		emplaceDigit(229, 360, 207, 383, 229);
		emplaceDigit(229, 392, 208, 414, 229);
		emplaceDigit(259, 328, 237, 351, 259);
		emplaceDigit(257, 361, 237, 384, 257);
		emplaceDigit(259, 393, 238, 417, 259);
		break;

	case 19213:
		emplaceDigit(265, 387, 243, 410, 265);
		emplaceDigit(175, 352, 154, 373, 175);
		emplaceDigit(173, 383, 154, 404, 173);
		emplaceDigit(174, 413, 155, 435, 174);
		emplaceDigit(203, 352, 183, 377, 203);
		emplaceDigit(201, 384, 183, 406, 201);
		emplaceDigit(201, 415, 182, 437, 201);
		emplaceDigit(234, 354, 212, 375, 234);
		emplaceDigit(233, 385, 212, 408, 233);
		emplaceDigit(232, 415, 212, 439, 232);
		break;

	case 21341:
		emplaceDigit(313, 384, 289, 410, 313);
		emplaceDigit(205, 346, 178, 370, 205);
		emplaceDigit(205, 383, 178, 408, 205);
		emplaceDigit(205, 419, 178, 446, 205);
		emplaceDigit(240, 346, 216, 371, 240);
		emplaceDigit(240, 383, 216, 408, 240);
		emplaceDigit(240, 421, 216, 446, 240);
		emplaceDigit(278, 346, 252, 371, 278);
		emplaceDigit(278, 383, 252, 408, 278);
		emplaceDigit(278, 421, 252, 446, 278);
		break;
	}

	if (_game.getPhoneManager().getState() <= 0) {
		_game.getSoundManager().play(20003, true, 80);
		_game.getInterface().putText(20003);
	}

	_game.getPhoneManager().setIsOffHook(true);
}

bool S2PhoneRoom::handleEvent(GLEvent &event) {
	// Event type check brought down here from callers since it was always the
	// same in SSCI
	if (event.getType() == kSciEventKeyDown &&
		event.getMessage() >= '0' && event.getMessage() <= '9') {
		pushButton(event.getMessage() - '0');
	}

	return false;
}

void S2PhoneRoom::emplaceDigit(const int digit, const int16 x1, const int16 y1, const int16 x2, const int16 y2) {
	emplaceHotspot(true, x1, y1, x2, y2).setMouseUpHandler([this, digit](GLEvent &, GLTarget &) {
		pushButton(digit);
	});
}

void S2PhoneRoom::pushButton(const int digit) {
	_game.getSoundManager().stop(20003);

	uint16 soundNo;
	const reg_t soundNode = make_reg(0, _number.size());

	switch (digit) {
	case 0:
		soundNo = 20013;
		break;
	case 1:
		soundNo = 20010;
		if (_game.getPhoneManager().getMotelState() == 1) {
			_game.getPhoneManager().processMessage(1, 0);
		}
		break;
	case 2:
	case 3:
		soundNo = 20010;
		break;
	case 4:
		soundNo = 20011;
		if (_game.getPhoneManager().getMotelState() == 2) {
			_game.getPhoneManager().processMessage(1, 0);
		}
		break;
	case 5:
	case 6:
		soundNo = 20011;
		break;
	case 7:
		soundNo = 20012;
		if (_game.getPhoneManager().getMotelState() == 2) {
			_game.getPhoneManager().processMessage(1, 6);
		}
		break;
	case 8:
		soundNo = 20012;
		break;
	case 9:
		soundNo = 20012;
		if (_game.getPhoneManager().getMotelState() == 2) {
			_game.getPhoneManager().processMessage(1, 3);
		}
		break;
	}

	_game.getSoundManager().play(soundNo, false, 80, false, nullptr, soundNode);

	if (_game.getSoundManager().isPlaying(65008) ||
		_game.getPhoneManager().getIsCalling() ||
		_game.getPhoneManager().getMotelState() != 0) {
		return;
	}

	if (_number.size() < 7) {
		// SSCI used sprintf to bring up the number into ASCII range
		_number += digit + '0';
	}

	if (_number.size() == 7) {
		_game.getCursor().endHighlight();
		_game.getCursor().setCel(0);
		_game.getPhoneManager().callPhoneNumber(_number.asUint64());
		_number += 'x';
	}
}

} // End of namespace Sci
