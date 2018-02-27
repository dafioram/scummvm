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

#ifndef SCI_S2_FLAGS_H
#define SCI_S2_FLAGS_H

#include "common/array.h"
#include "common/serializer.h"
#include "sci/s2/room_manager.h"

namespace Sci {

enum GameFlag {
	kGameFlag0,
	kGameFlag1,
	kGameFlag2,
	kGameFlag3,
	kGameFlag4,
	kGameFlag5,
	kGameFlag6,
	kGameFlag7,
	kGameFlag8,
	kGameFlag9,
	kGameFlag10,
	kGameFlag11,
	kGameFlag12,
	kGameFlag13,
	kGameFlag14,
	kGameFlag15,
	kGameFlag16,
	kGameFlag17,
	kGameFlag18,
	kGameFlag19,
	kGameFlag20,
	kGameFlag21,
	kGameFlag22,
	kGameFlag23,
	kGameFlag24,
	kGameFlag25,
	kGameFlag26,
	kGameFlag27,
	kGameFlag28,
	kGameFlag29,
	kGameFlag30,
	kGameFlag31,
	kGameFlag32,
	kGameFlag33,
	kGameFlag34,
	kGameFlag35,
	kGameFlag36,
	kGameFlag37,
	kGameFlag38,
	kGameFlag39,
	kGameFlag40,
	kGameFlag41,
	kGameFlag42,
	kGameFlag43,
	kGameFlag44,
	kGameFlag45,
	kGameFlag46,
	kGameFlag47,
	kGameFlag48,
	kGameFlag49,
	kGameFlag50,
	kGameFlag51,
	kGameFlag52,
	kGameFlag53,
	kGameFlag54,
	kGameFlag55,
	kGameFlag56,
	kGameFlag57,
	kGameFlag58,
	kGameFlag59,
	kGameFlag60,
	kGameFlag61,
	kGameFlag62,
	kGameFlag63,
	kGameFlag64,
	kGameFlag65,
	kGameFlag66,
	kGameFlag67,
	kGameFlag68,
	kGameFlag69,
	kGameFlag70,
	kGameFlag71,
	kGameFlag72,
	kGameFlag73,
	kGameFlag74,
	kGameFlag75,
	kGameFlag76,
	kGameFlag77,
	kGameFlag78,
	kGameFlag79,
	kGameFlag80,
	kGameFlag81,
	kGameFlag82,
	kGameFlag83,
	kGameFlag84,
	kGameFlag85,
	kGameFlag86,
	kGameFlag87,
	kGameFlag88,
	kGameFlag89,
	kGameFlag90,
	kGameFlag91,
	kGameFlag92,
	kGameFlag93,
	kGameFlag94,
	kGameFlag95,
	kGameFlag96,
	kGameFlag97,
	kGameFlag98,
	kGameFlag99,
	kGameFlag100,
	kGameFlag101,
	kGameFlag102,
	kGameFlag103,
	kGameFlag104,
	kGameFlag105,
	kGameFlag106,
	kGameFlag107,
	kGameFlag108,
	kGameFlag109,
	kGameFlag110,
	kGameFlag111,
	kGameFlag112,
	kGameFlag113,
	kGameFlag114,
	kGameFlag115,
	kGameFlag116,
	kGameFlag117,
	kGameFlag118,
	kGameFlag119,
	kGameFlag120,
	kGameFlag121,
	kGameFlag122,
	kGameFlag123,
	kGameFlag124,
	kGameFlag125,
	kGameFlag126,
	kGameFlag127,
	kGameFlag128,
	kGameFlag129,
	kGameFlag130,
	kGameFlag131,
	kGameFlag132,
	kGameFlag133,
	kGameFlag134,
	kGameFlag135,
	kGameFlag136,
	kGameFlag137,
	kGameFlag138,
	kGameFlag139,
	kGameFlag140,
	kGameFlag141,
	kGameFlag142,
	kUnlockedSuitcase,
	kGameFlag144,
	kGameFlag145,
	kGameFlag146,
	kGameFlag147,
	kGameFlag148,
	kGameFlag149,
	kGameFlag150,
	kGameFlag151,
	kGameFlag152,
	kGameFlag153,
	kGameFlag154,
	kGameFlag155,
	kGameFlag156,
	kGameFlag157,
	kGameFlag158,
	kGameFlag159,
	kGameFlag160,
	kGameFlag161,
	kGameFlag162,
	kGameFlag163,
	kGameFlag164,
	kGameFlag165,
	kGameFlag166,
	kGameFlag167,
	kGameFlag168,
	kGameFlag169,
	kGameFlag170,
	kGameFlag171,
	kGameFlag172,
	kGameFlag173,
	kGameFlag174,
	kGameFlag175,
	kGameFlag176,
	kGameFlag177,
	kGameFlag178,
	kGameFlag179,
	kGameFlag180,
	kGameFlag181,
	kGameFlag182,
	kGameFlag183,
	kGameFlag184,
	kGameFlag185,
	kGameFlag186,
	kGameFlag187,
	kGameFlag188,
	kGameFlag189,
	kGameFlag190,
	kGameFlag191,
	kGameFlag192,
	kGameFlag193,
	kGameFlag194,
	kGameFlag195,
	kGameFlag196,
	kGameFlag197,
	kGameFlag198,
	kGameFlag199,
	kGameFlag200,
	kGameFlag201,
	kGameFlag202,
	kGameFlag203,
	kGameFlag204,
	kGameFlag205,
	kGameFlag206,
	kGameFlag207,
	kGameFlag208,
	kGameFlag209,
	kGameFlag210,
	kGameFlag211,
	kGameFlag212,
	kGameFlag213,
	kGameFlag214,
	kGameFlag215,
	kGameFlag216,
	kGameFlag217,
	kGameFlag218,
	kGameFlag219,
	kGameFlag220,
	kGameFlag221,
	kNumGameFlags
};

class GameFlags : public Common::Serializable {
public:
	GameFlags(S2RoomManager &roomManager) :
		_roomManager(roomManager),
		_flags() {}

	virtual void saveLoadWithSerializer(Common::Serializer &s) {
		s.syncArray(_flags.data(), _flags.size(), Common::Serializer::Byte);
	}

	bool get(const GameFlag flag) const {
		return _flags[flag];
	}

	void set(const GameFlag flag) {
		_roomManager.setIsSaved(false);
		_flags[flag] = true;
	}

	void clear(const GameFlag flag) {
		// SSCI did not change the saved flag here
		_roomManager.setIsSaved(false);
		_flags[flag] = false;
	}

private:
	S2RoomManager &_roomManager;
	Common::FixedArray<bool, kNumGameFlags> _flags;
};

} // End of namespace Sci

#endif
