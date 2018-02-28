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

#ifndef SCI_S2_PHONE_MANAGER_H
#define SCI_S2_PHONE_MANAGER_H

#include "common/array.h"
#include "common/serializer.h"
#include "sci/s2/panorama_sprite.h"
#include "sci/s2/system/glscript.h"

namespace Sci {

class S2Game;

class S2PhoneManager : public GLScript, public Common::Serializable {
public:
	S2PhoneManager(S2Game &game);
	void init();

	virtual void saveLoadWithSerializer(Common::Serializer &) override;

	void pushedMotelMessageButton();

	void addAnsweringMachineLight(const int viewNo);
	void addAnsweringMachineLight(const int bitmapNo, const GLPoint &position);
	void removeAnsweringMachineLight();
	void resetPhone();
	void cancelCall();

	void notifyRoomChange(const bool baseRoomChanged);

	// TODO: Use an enum?
	int getMotelState() const { return _motelState; }
	void setMotelState(const int state) { _motelState = state; }

	// TODO: Use an enum?
	void processMessage(const int type, const int action);

	void callPhoneNumber(const uint32 number);

	bool getIsCalling() const { return _isCalling; }
	void setIsCalling(const bool isCalling) { _isCalling = isCalling; }

	bool getIsOffHook() const { return _isOffHook; }
	void setIsOffHook(const bool isOffHook) { _isOffHook = isOffHook; }

private:
	struct RoomPhone {
		int roomNo = 0;
		uint32 phoneNo = 0;
		int answeringMachineMessage = 0;
		Common::FixedArray<uint16, 20> newMessages = {};
		Common::FixedArray<uint16, 20> savedMessages = {};
		int lastSavedMessageIndex = 0;

		void add(const int type, const uint16 soundNo) {
			auto &collection = (type == 1 ? newMessages : savedMessages);
			for (auto &message : collection) {
				if (!message) {
					message = soundNo;
					return;
				}
			}
		}
	};

	bool messageExists(const int type);

	void changeState(GLScript &script, const int state);

	void playMessage(const uint16 soundNo, GLScript &script, const int16 volume = 103, const bool loop = false);

	void playPanorama(const uint16 soundNo, GLScript &script, const int16 volumeAdjust = 0);

	void blinkingLight(GLScript &, const int state);

	int getCurrentPhoneIndex() const;

	void saveMessage(const int type);

	void deleteMessage(const int type);

	RoomPhone &getPhoneForRoom(const int roomNo) {
		for (auto &phone : _phones) {
			if (phone.roomNo == roomNo) {
				return phone;
			}
		}

		error("Could not find phone for ambient room %d", roomNo);
	}

	Common::FixedArray<RoomPhone, 13> _phones = {{
		{ 10,       0,     0, {{ 61001, 61002 }} },
		{ 11, 5556009, 61100, {}, {{ 61101, 61102, 61103 }} },
		{ 12 },
		{ 13, 5551029, 61301, {{ 61302, 61304, 61303 }}, {{ 61305, 61306 }} },
		{ 14, 5554190, 61402, {{ 61403 }}, {{ 61401, 61404 }} },
		{ 15, 5553982, 61503, {{ 61502 }}, {{ 61501 }} },
		{ 16, 5552938, 61603, {{ 61604, 61606 }}, {{ 61602, 61605, 61601 }} },
		{ 17, 5551253, 61701, {{ 61702, 61703 }}, {{ 61750 }} },
		{ 18, 5556321, 61803, {}, {{ 61802, 61804, 61801 }} },
		{ 19, 5553200, 61901, {{ 61904, 61905, 61902, 61903, 61951 }}, {{ 61906, 61950 }} },
		{ 21, 5559999, 62103, {}, {{ 62101, 62102 }} },
		{ 95, 5559547 },
		{ 90, 5556000, 65024 }
	}};

	S2Game &_game;
	uint16 _currentSoundNo = 0;
	int _phoneIndex = 0; // index into _phones
	int _motelState = 0;
	int _lastMessageType = 0;
	bool _isCalling = false;
	bool _isOffHook = false;
	uint16 _nextRoomEnterMessage = 0;
	uint16 _incomingMessage = 0;
	uint8 _randomness = 0;
	Common::Array<uint16> _randomMessages1;
	Common::Array<uint16> _randomMessages2;
	Common::Array<uint16> _randomMessages3;

	struct PanoramaSound {
		int roomNo;
		int16 panX;
		int16 volume;
	};

	static constexpr Common::FixedArray<PanoramaSound, 53> _panoramaSounds = {{
		{ 10100, 1720, 47 },
		{ 10200, 996, 23 },
		{ 10300, 78, 79 },
		{ 10400, 144, 103 },
		{ 11100, 2039, 79 },
		{ 11200, 1263, 87 },
		{ 11300, 708, 119 },
		{ 12100, 1720, 47 },
		{ 12200, 996, 23 },
		{ 12300, 78, 79 },
		{ 12400, 144, 103 },
		{ 13100, 1933, 79 },
		{ 13200, 1817, 119 },
		{ 13300, 1324, 111 },
		{ 13400, 492, 63 },
		{ 13500, 568, 31 },
		{ 13501, 568, 31 },
		{ 14100, 1917, 103 },
		{ 14200, 196, 111 },
		{ 14300, 1598, 55 },
		{ 14400, 1582, 55 },
		{ 14500, 1659, 111 },
		{ 15000, 1308, 103 },
		{ 15300, 950, 95 },
		{ 15400, 701, 31 },
		{ 15600, 515, 31 },
		{ 16000, 620, 79 },
		{ 16100, 997, 23 },
		{ 16200, 901, 23 },
		{ 16400, 973, 47 },
		{ 16600, 877, 87 },
		{ 16700, 650, 71 },
		{ 16900, 752, 111 },
		{ 36100, 1588, 95 },
		{ 36200, 698, 55 },
		{ 36300, 816, 23 },
		{ 17100, 1528, 111 },
		{ 17200, 1159, 87 },
		{ 17300, 703, 87 },
		{ 17400, 865, 47 },
		{ 18100, 227, 23 },
		{ 18200, 2023, 47 },
		{ 18300, 449, 79 },
		{ 18400, 277, 111 },
		{ 19100, 122, 87 },
		{ 19200, 940, 103 },
		{ 19300, 1021, 55 },
		{ 21100, 1934, 63 },
		{ 21200, 1671, 103 },
		{ 21300, 627, 119 },
		{ 21400, 1033, 37 },
		{ 21600, 1058, 103 }
	}};

	int _roomPan = 0; // index into _phonePan

	uint16 _lightResourceNo = 0;
	GLPoint _lightPosition;
	Common::ScopedPtr<GLScript> _lightScript;
	Common::ScopedPtr<GLCel> _lightCel;
	Common::ScopedPtr<S2PanoramaSprite> _panoramaLight;
};

} // End of namespace Sci

#endif
