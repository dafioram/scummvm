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
#include "sci/s2/system/glscript.h"

namespace Sci {

class S2Game;

class S2PhoneManager : public GLScript, public Common::Serializable {
public:
	S2PhoneManager(S2Game &game);
	void init();

	virtual void saveLoadWithSerializer(Common::Serializer &) override;

	void pushedMotelMessageButton();

	void addAnsweringMachineLight(const int roomNo);
	void addAnsweringMachineLight(const int roomNo, const GLPoint &position);
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
		Common::FixedArray<uint16, 20> savedMessages = {};
		Common::FixedArray<uint16, 20> newMessages = {};
		int lastSavedMessageIndex = 0;
		int answeringMachineMessage = 0;
		uint32 phoneNo = 0;
		int roomNo = 0;

		void add(const int type, const uint16 soundNo) {
			warning("TODO: %s", __PRETTY_FUNCTION__);
		}
	};

	bool messageExists(const int type);

	void changeState(GLScript &script, const int state);

	void playMessage(const uint16 soundNo, GLScript &script, const int16 volume = 103, const bool loop = false);

	void playPanorama(const uint16 soundNo, GLScript &script, const int16 volumeAdjust = 0);

	Common::FixedArray<RoomPhone, 13> _phones = {};

	S2Game &_game;
	uint16 _currentSoundNo;
	int _phoneIndex; // index into _phones
	int _motelState;
	int _lastMessageType;
	bool _isCalling;
	bool _isOffHook;
	uint16 _nextRoomEnterMessage;
	uint16 _incomingMessage;

	Common::FixedArray<Common::FixedArray<int, 3>, 53> _phonePan; // room, position, volume
	int _roomPan; // index into _phonePan

	uint16 _lightResourceNo;
	GLPoint _lightPosition;
};

} // End of namespace Sci

#endif
