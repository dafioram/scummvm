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

#include "sci/s2/game.h"
#include "sci/s2/phone_manager.h"
#include "sci/s2/room.h"
#include "sci/s2/system/glpanorama.h"

namespace Sci {

constexpr Common::FixedArray<S2PhoneManager::PanoramaSound, 53> S2PhoneManager::_panoramaSounds;

S2PhoneManager::S2PhoneManager(S2Game &game) :
	GLScript(),
	_game(game) {
	_randomMessages1.reserve(5);
	for (int i = 65064; i < 65064 + 5; ++i) {
		_randomMessages1.push_back(i);
	}
	_randomMessages2.reserve(8);
	for (int i = 65052; i < 65052 + 8; ++i) {
		_randomMessages2.push_back(i);
	}
	_randomMessages3.reserve(2);
	_randomMessages3.push_back(65060);
	_randomMessages3.push_back(65061);
}

void S2PhoneManager::init() {
	// Trying to initialise the script at construction time is invalid since the
	// object is not fully initialised so trying to capture it will result in
	// the wrong vtable being used; because S2PhoneManager is an inline member
	// of S2Game it also cannot be fully initialised immediately since GLCue has
	// to be able to access the global game object
	GLScript::init(this, &S2PhoneManager::changeState);
}

void S2PhoneManager::saveLoadWithSerializer(Common::Serializer &s) {
	s.syncAsUint16LE(_incomingMessage);
	s.syncAsUint16LE(_nextRoomEnterMessage);
	s.syncAsByte(_randomness);
	s.syncAsUint16LE(_phones[11].answeringMachineMessage);
	for (auto &&phone : _phones) {
		s.syncArray(phone.newMessages.data(), phone.newMessages.size(), Common::Serializer::Uint16LE);
		s.syncArray(phone.savedMessages.data(), phone.savedMessages.size(), Common::Serializer::Uint16LE);
	}

	uint8 numElements = _randomMessages1.size();
	s.syncAsByte(numElements);
	if (s.isLoading()) {
		_randomMessages1.resize(numElements);
	}
	for (int i = 0; i < numElements; ++i) {
		s.syncAsUint16LE(_randomMessages1[i]);
	}

	numElements = _randomMessages2.size();
	s.syncAsByte(numElements);
	if (s.isLoading()) {
		_randomMessages2.resize(numElements);
	}
	for (int i = 0; i < numElements; ++i) {
		s.syncAsUint16LE(_randomMessages2[i]);
	}

	numElements = _randomMessages3.size();
	s.syncAsByte(numElements);
	if (s.isLoading()) {
		_randomMessages3.resize(numElements);
	}
	for (int i = 0; i < numElements; ++i) {
		s.syncAsUint16LE(_randomMessages3[i]);
	}
}

void S2PhoneManager::pushedMotelMessageButton() {
	_game.getInterface().putText(0);

	if (getMotelState()) {
		_game.getSoundManager().stop(_currentSoundNo);
		_game.getSoundManager().play(20003, true, 80);
		setMotelState(0);
		return;
	}

	_game.getSoundManager().play(20016, false, 80);
	_game.getSoundManager().stop(20003);
	setMotelState(1);

	_phoneIndex = getCurrentPhoneIndex();
	assert(_phoneIndex < _phones.size());

	if (messageExists(1)) {
		setState(17);
	} else {
		setState(15);
	}

	cue();
}

void S2PhoneManager::addAnsweringMachineLight(const int resourceNo) {
	if (!resourceNo || _lightScript) {
		return;
	}

	_lightResourceNo = resourceNo;
	auto &phone = getPhoneForRoom(_game.getRoomManager().getCurrentAmbientRoomNo());
	if (phone.newMessages[0]) {
		_lightCel.reset(new GLCel(_game.getRoomManager().getGamePlane(), resourceNo, 0, 0, S2Room::roomBottom));
		// SSCI unconditionally created the script, which was a bug that would
		// cause a read of garbage pointer
		_lightScript.reset(new GLScript(this, &S2PhoneManager::blinkingLight));
	}
}

void S2PhoneManager::removeAnsweringMachineLight() {
	_lightResourceNo = 0;
	if (_lightScript) {
		_lightScript.reset();
		// SSCI had a check for a panorama light, but the game never used this
		// so it is omitted
		_lightCel.reset();
	}
}

void S2PhoneManager::resetPhone() {
	_game.getInterface().putText(0);
	_isOffHook = false;
	_isCalling = false;
	// SSCI checked if panorama is null here, which is impossible, so is omitted
	_game.getRoomManager().getPanorama().detachSound(_currentSoundNo);
	_game.getSoundManager().stop(_currentSoundNo);
	_game.getSoundManager().stop(20003);
	_game.getSoundManager().play(20005, false, 80);
	setState(-1);
}

void S2PhoneManager::resetAnsweringMachine() {
	if (!_isCalling) {
		_game.getSoundManager().stop(_currentSoundNo);
	}
	_game.getInterface().putText(0);
	auto &phone = getPhoneForRoom(_game.getRoomManager().getCurrentAmbientRoomNo());
	phone.lastSavedMessageIndex = 0;
	setState(-1);
}

void S2PhoneManager::cancelCall() {
	if (_isCalling) {
		_nextRoomEnterMessage = _incomingMessage;
		_incomingMessage = 0;
		_isOffHook = false;
		_isCalling = false;
		_game.getRoomManager().getPanorama().detachSound(_currentSoundNo);
		_game.getSoundManager().stop(_currentSoundNo);
		_game.getInterface().putText(0);
		setState(-1);
	}
}

template <typename T, typename U>
static int binarySearch(T array, U value) {
	int min = 0, max = array.size() - 1, result;
	while (max >= min) {
		result = (max - min) / 2 + min;
		if (array[result].roomNo == value) {
			return result;
		}
		if (array[result].roomNo > value) {
			max = result - 1;
		} else {
			min = result + 1;
		}
	}
	return array.size();
}

void S2PhoneManager::notifyRoomChange(const bool baseRoomChanged) {
	if (!_isCalling && _incomingMessage) {
		const auto phoneIndex = getCurrentPhoneIndex();
		if (phoneIndex < _phones.size()) {
			_isCalling = true;
			_phoneIndex = phoneIndex;
			setState(39);
			cue();
		}
	}

	if (_isCalling) {
		const auto roomNo = _game.getRoomManager().getCurrentRoomNo();
		_roomPan = binarySearch(_panoramaSounds, roomNo);
		if (_roomPan < _panoramaSounds.size()) {
			const auto &sound = _panoramaSounds[_roomPan];
			_game.getRoomManager().getPanorama().attachSound(_currentSoundNo, sound.panX, sound.volume);
		}

		if (baseRoomChanged) {
			_isCalling = false;
			_game.getRoomManager().getPanorama().detachSound(_currentSoundNo);
			_game.getSoundManager().stop(_currentSoundNo);
			setState(-1);
			_game.getInterface().putText(0);

			if (_nextRoomEnterMessage) {
				_incomingMessage = _nextRoomEnterMessage;
				_nextRoomEnterMessage = 0;
			}
		}
	} else if (baseRoomChanged) {
		randomDcCall();
	}
}

void S2PhoneManager::randomDcCall() {
	if (_randomness > _game.getRandomNumber(0, 100)) {
		int numFlags = 0;
		for (int flag = kGameFlag115; flag <= kGameFlag126; ++flag) {
			if (_game.getFlags().get(GameFlag(flag))) {
				++numFlags;
			}
		}
		if (numFlags == 12 && _randomMessages3.size()) {
			const auto index = _game.getRandomNumber(0, _randomMessages3.size() - 1);
			_incomingMessage = _randomMessages3[index];
			_randomMessages3.remove_at(index);
		} else if (_game.getFlags().get(kGameFlag195) &&
				   _game.getFlags().get(kGameFlag196) &&
				   _randomMessages2.size()) {
			const auto index = _game.getRandomNumber(0, _randomMessages2.size() - 1);
			_incomingMessage = _randomMessages2[index];
			_randomMessages2.remove_at(index);
		} else if (_randomMessages1.size()) {
			const auto index = _game.getRandomNumber(0, _randomMessages1.size() - 1);
			_incomingMessage = _randomMessages1[index];
			_randomMessages1.remove_at(index);
		}
	}

	// SSCI just kept incrementing forever, even though once it gets above 100
	// it will always trigger
	if (_randomness < 101) {
		++_randomness;
	}
}

void S2PhoneManager::processMessage(int type, const int action) {
	const auto roomNo = _game.getRoomManager().getCurrentAmbientRoomNo();

	_phoneIndex = getCurrentPhoneIndex();
	assert(_phoneIndex < _phones.size());

	if (roomNo == 10 || roomNo == 12) {
		if (!_phones[_phoneIndex].newMessages[0]) {
			type = 2;
		}

		setState(10 + action);
	} else {
		setState(action);
	}

	_lastMessageType = type;

	if (action == 3) {
		saveMessage(type);
	} else if (action == 6) {
		deleteMessage(type);
	}

	cue();
}

void S2PhoneManager::saveMessage(const int type) {
	auto &phone = getPhoneForRoom(_game.getRoomManager().getCurrentAmbientRoomNo());
	if (type == 1) {
		for (auto &message : phone.savedMessages) {
			if (!message) {
				message = phone.newMessages[0];
				break;
			}
		}
		for (auto it = phone.newMessages.begin(); it != phone.newMessages.end() - 1; ++it) {
			*it = *(it + 1);
		}
		phone.newMessages.back() = 0;
	} else {
		++phone.lastSavedMessageIndex;
	}
}

void S2PhoneManager::deleteMessage(const int type) {
	auto &phone = getPhoneForRoom(_game.getRoomManager().getCurrentAmbientRoomNo());
	auto &collection = (type == 1 ? phone.newMessages : phone.savedMessages);
	const int indexToDelete = (type == 1 ? 0 : phone.lastSavedMessageIndex);
	for (auto it = collection.begin() + indexToDelete; it != collection.end() - 1; ++it) {
		*it = *(it + 1);
	}
	collection.back() = 0;
	if (type != 1 && collection[indexToDelete] == 0) {
		phone.lastSavedMessageIndex = 0;
	}
}

int S2PhoneManager::getCurrentPhoneIndex() const {
	const auto roomNo = _game.getRoomManager().getCurrentAmbientRoomNo();
	int i;
	for (i = 0; i < _phones.size(); ++i) {
		if (_phones[i].roomNo == roomNo) {
			break;
		}
	}

	return i;
}

void S2PhoneManager::blinkingLight(GLScript &script, const int state) {
	switch (state) {
	case 0:
		if (_panoramaLight) {
			_game.getRoomManager().getPanorama().addSprite(*_panoramaLight, true);
		} else {
			_lightCel->show();
		}

		script.setSeconds(1);
		break;

	case 1:
		if (_panoramaLight) {
			_game.getRoomManager().getPanorama().removeSprite(*_panoramaLight);
		} else {
			_lightCel->hide();
		}

		script.setState(-1);
		script.setSeconds(1);
		break;
	}
}

void S2PhoneManager::callPhoneNumber(const uint32 number) {
	uint i;
	for (i = 0; i < _phones.size(); ++i) {
		if (_phones[i].phoneNo == number) {
			break;
		}
	}

	if (i == _phones.size()) {
		setState(29);
	} else {
		if (number == 5559547) {
			if (_game.getFlags().get(kGameFlag2)) {
				if (_game.getFlags().get(kGameFlag3)) {
					_phones[11].answeringMachineMessage = 0;
				} else {
					_phones[11].answeringMachineMessage = 65051;
				}
				_game.getFlags().set(kGameFlag3);
			} else if (_phones[11].answeringMachineMessage == 65050) {
				_game.getFlags().set(kGameFlag2);
			}
		}

		if (_phones[i].roomNo == _game.getRoomManager().getCurrentAmbientRoomNo()) {
			setState(34);
		} else {
			_phoneIndex = i;
			setState(19);
		}
	}

	cue();
}

bool S2PhoneManager::messageExists(const int type) {
	const auto &phone = getPhoneForRoom(_game.getRoomManager().getCurrentAmbientRoomNo());
	if (phone.roomNo == 10 || phone.roomNo == 12) {
		return phone.newMessages[0] || phone.savedMessages[phone.lastSavedMessageIndex];
	} else if (type == 1) {
		return phone.newMessages[0];
	} else {
		return phone.savedMessages[phone.lastSavedMessageIndex];
	}
}

void S2PhoneManager::changeState(GLScript &script, const int state) {
	switch (state) {
	case 0:
		_game.getUser().setIsHandsOn(true);
		break;

	case 1: {
		_game.getSoundManager().stop(_currentSoundNo);
		_game.getCursor().endHighlight();
		auto &phone = _phones[_phoneIndex];
		uint16 soundNo;
		if (_lastMessageType == 1) {
			soundNo = phone.newMessages[0];
		} else {
			if (!phone.savedMessages[phone.lastSavedMessageIndex]) {
				phone.lastSavedMessageIndex = 0;
			}

			soundNo = phone.savedMessages[phone.lastSavedMessageIndex];
		}
		if (soundNo) {
			_currentSoundNo = soundNo;
			_game.getSoundManager().play(soundNo, false, Audio32::kMaxVolume, false, &script);
			_game.getInterface().putText(0);
			_game.getInterface().putText(soundNo, true, false);
		} else {
			script.setState(7);
			script.setCycles(1);
		}
		break;
	}

	case 2:
		script.setCycles(1);
		break;

	case 3:
		_game.getUser().setIsHandsOn(true);
		if (_lastMessageType == 1) {
			playMessage(65010, script);
		} else {
			playMessage(65011, script);
		}
		script.setState(-1);
		break;

	case 4:
		playMessage(65004, script);
		if (messageExists(_lastMessageType)) {
			script.setState(-1);
		}
		break;

	case 5:
		script.setCycles(1);
		break;

	case 6:
		playMessage(65007, script);
		if (_lastMessageType == 1) {
			removeAnsweringMachineLight();
		}
		script.setState(-1);
		break;

	case 7:
		playMessage(65005, script);
		script.setState(-1);
		break;

	case 8:
		_game.getUser().setIsHandsOn(true);
		_game.getSoundManager().stop(_currentSoundNo);
		if (_lastMessageType == 1) {
			playMessage(65013, script);
			removeAnsweringMachineLight();
		} else {
			playMessage(65012, script);
		}
		script.setState(-1);
		break;

	case 11: {
		auto &phone = _phones[_phoneIndex];
		uint16 soundNo;
		if (_lastMessageType == 1) {
			// SSCI did some stuff with a new message index, but the new message
			// index was never set to any value other than zero, so we just
			// always check only for new message zero
			soundNo = phone.newMessages[0];
			if (!soundNo) {
				removeAnsweringMachineLight();
				_lastMessageType = 2;
				phone.lastSavedMessageIndex = 0;
				soundNo = phone.savedMessages[0];
			}
		} else {
			if (!phone.savedMessages[phone.lastSavedMessageIndex]) {
				phone.lastSavedMessageIndex = 0;
			}
			soundNo = phone.savedMessages[phone.lastSavedMessageIndex];
		}

		if (soundNo) {
			playMessage(soundNo, script);
			setMotelState(2);
		} else {
			script.setState(7);
			script.setCycles(1);
		}

		break;
	}

	case 12:
		script.setCycles(1);
		break;

	case 13:
		_game.getUser().setIsHandsOn(true);
		playMessage(65003, script);
		script.setState(-1);
		break;

	case 14:
		setMotelState(3);
		playMessage(65004, script);
		break;

	case 15:
		script.setCycles(1);
		break;

	case 16:
		setMotelState(3);
		if (messageExists(1)) {
			playMessage(65006, script);
			script.setState(10);
		} else {
			auto &phone = _phones[_phoneIndex];
			phone.lastSavedMessageIndex = 0;
			if (!phone.newMessages[0]) {
				removeAnsweringMachineLight();
			}
			playMessage(65002, script);
			script.setState(-1);
		}
		break;

	case 17:
		setMotelState(3);
		playMessage(65005, script);
		script.setState(15);
		break;

	case 18:
		_game.getUser().setIsHandsOn(true);
		playMessage(65001, script);
		script.setState(-1);
		break;

	case 20:
	case 22:
	case 24:
	case 26:
		_game.getUser().setIsHandsOn(true);
		playMessage(20015, script);
		break;

	case 21:
	case 23:
	case 25:
		script.setSeconds(1);
		break;

	case 27:
		if (_phones[_phoneIndex].answeringMachineMessage) {
			playMessage(_phones[_phoneIndex].answeringMachineMessage, script);
			script.setState(-1);
		} else {
			script.setState(19);
			script.setCycles(1);
		}
		break;

	case 30:
		_game.getUser().setIsHandsOn(true);
		playMessage(20015, script, 60);
		break;

	case 31:
		script.setSeconds(1);
		break;

	case 32:
		_game.getUser().setIsHandsOn(true);
		playMessage(65008, script);
		script.setState(-1);
		break;

	case 35:
		_game.getUser().setIsHandsOn(true);
		playMessage(20022, script, 60, true);
		break;

	case 36:
		script.setState(-1);
		script.setCycles(1);
		break;

	case 40:
	case 42:
	case 44:
	case 46:
		if (!_isOffHook) {
			playPanorama(20009, script);
		} else {
			script.setState(47);
			script.setCycles(1);
		}
		break;

	case 41:
	case 43:
	case 45:
		script.setSeconds(1);
		break;

	case 47: {
		playPanorama(_phones[_phoneIndex].answeringMachineMessage, script);
		break;
	}

	case 48:
		playPanorama(_incomingMessage, script, 16);
		break;

	case 49:
		_game.getRoomManager().getPanorama().detachSound(_currentSoundNo);
		if (!_isOffHook) {
			_phones[_phoneIndex].add(1, _incomingMessage);
		}

		// SSCI had a check for a light position which would indicate a panorama
		// sprite, but panorama sprites were never used by the game so this is
		// omitted
		addAnsweringMachineLight(_lightResourceNo);

		_incomingMessage = 0;
		_isCalling = false;
		script.setState(-1);
		break;
	}
}

void S2PhoneManager::playPanorama(const uint16 soundNo, GLScript &script, const int16 volumeAdjust) {
		_game.getRoomManager().getPanorama().detachSound(_currentSoundNo);
		_currentSoundNo = 0;
		// SSCI checked if _roomPan was in bounds of _phonePan here, but then it
		// went ahead and used it unconditionally later, so the bounds check is
		// omitted here
		const auto &panSounds = _panoramaSounds[_roomPan];
		_game.getRoomManager().getPanorama().attachSound(soundNo, panSounds.panX, panSounds.volume);
		playMessage(soundNo, script, panSounds.volume + volumeAdjust);
}

void S2PhoneManager::playMessage(const uint16 soundNo, GLScript &script, const int16 volume, const bool loop) {
	_game.getSoundManager().stop(_currentSoundNo);
	_currentSoundNo = soundNo;
	_game.getSoundManager().play(soundNo, loop, volume, false, &script);
	_game.getInterface().putText(soundNo);
}

} // End of namespace Sci
