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
#include "sci/s2/system/glpanorama.h"

namespace Sci {

S2PhoneManager::S2PhoneManager(S2Game &game) :
	GLScript(),
	_game(game),
	_motelState(0),
	_isCalling(false),
	_currentSoundNo(0),
	_phoneIndex(0),
	_lastMessageType(0) {}

void S2PhoneManager::init() {
	// Trying to initialise the script at construction time is invalid since the
	// object is not fully initialised so trying to capture it will result in
	// the wrong vtable being used; because S2PhoneManager is an inline member
	// of S2Game it also cannot be fully initialised immediately since GLCue has
	// to be able to access the global game object
	GLScript::init(this, &S2PhoneManager::changeState);
}

void S2PhoneManager::saveLoadWithSerializer(Common::Serializer &s) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2PhoneManager::pushedMotelMessageButton() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2PhoneManager::addAnsweringMachineLight(const int roomNo) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2PhoneManager::addAnsweringMachineLight(const int roomNo, const GLPoint &position) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2PhoneManager::removeAnsweringMachineLight() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
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

void S2PhoneManager::notifyRoomChange(const bool baseRoomChanged) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2PhoneManager::processMessage(const int type, const int action) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
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
	warning("TODO: %s", __PRETTY_FUNCTION__);
	return false;
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

	case 11:
		warning("TODO: state 11");
		break;

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

		if (_lightPosition.x) {
			addAnsweringMachineLight(_lightResourceNo, _lightPosition);
		} else {
			addAnsweringMachineLight(_lightResourceNo);
		}

		_incomingMessage = 0;
		_isCalling = false;
		script.setState(-1);
		break;
	}
}

void S2PhoneManager::playPanorama(const uint16 soundNo, GLScript &script, const int16 volumeAdjust) {
		_game.getRoomManager().getPanorama().detachSound(_currentSoundNo);
		const auto &panSounds = _phonePan[_roomPan];
		_currentSoundNo = 0;
		if (_roomPan < 53) {
			_game.getRoomManager().getPanorama().attachSound(soundNo, panSounds[1], panSounds[2]);
		}
		playMessage(soundNo, script, panSounds[2] + volumeAdjust);
}

void S2PhoneManager::playMessage(const uint16 soundNo, GLScript &script, const int16 volume, const bool loop) {
	_game.getSoundManager().stop(_currentSoundNo);
	_currentSoundNo = soundNo;
	_game.getSoundManager().play(soundNo, loop, volume, false, &script);
	_game.getInterface().putText(soundNo);
}

} // End of namespace Sci
