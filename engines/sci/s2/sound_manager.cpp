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

#include "common/textconsole.h"
#include "sci/s2/game.h"
#include "sci/s2/sound_manager.h"

namespace Sci {

void S2SoundManager::stopAllSounds() {
	deleteAmbient(_roomNo);
	stop();
}

void S2SoundManager::createAmbient(const int roomNo) {
	if (_roomNo == roomNo) {
		return;
	}

	if (_roomNo) {
		deleteAmbient(_roomNo);
	}

	switch (roomNo ? roomNo : _game.getRoomManager().getCurrentRoomNo()) {
	case 2:
		_tracks[0] = &createSoundTrack();
		_tracks[0]->addWaitNode(3, 6);
		_tracks[0]->addSoundNode(31501, 80);

		_tracks[1] = &createSoundTrack();
		_tracks[1]->addWaitNode(3, 5);
		_tracks[1]->addSoundNode(31502, 80, true, 30);

		_tracks[2] = &createSoundTrack();
		_tracks[2]->addWaitNode(3, 5);
		_tracks[2]->addPRSNode(31503, 100, 72, 70);
		_tracks[2]->addPRSNode(31504, 100, 72, 70);
		_tracks[2]->addPRSNode(31505, 100, 72, 70);
		break;
	case 6:

		break;

	case 7:
		break;

	case 8:

		break;

	case 9:

		break;

	case 10:
	case 11:
	case 12:
	case 30:
	case 31:
	case 32:
		break;

	case 13:
	case 33:
		break;

	case 14:
	case 34:
		break;

	case 15:
	case 35:

		break;

	case 16:
	case 36:
		break;

	case 17:
	case 37:
		break;

	case 18:
	case 38:
		break;

	case 19:
	case 39:
		break;

	case 20:
	case 40:
		break;

	case 21:
	case 41:
		break;

	case 22:
	case 42:
		break;

	case 23:
	case 43:
		break;

	case 24:
	case 44:
		break;

	case 25:
	case 45:
		break;

	case 26:
	case 46:
		break;

	case 27:
	case 47:
		break;


	case 28:
	case 48:
		break;

	case 29:
	case 49:
		break;

	case 50:
		break;
	}

	for (auto &&track : _tracks) {
		if (track) {
			track->play();
		}
	}

	_roomNo = roomNo;
}

void S2SoundManager::playAmbient(const int roomNo) {
	if (roomNo) {
		_roomNo = roomNo;
	} else {

	}
}

void S2SoundManager::deleteAmbient(const int roomNo) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

} // End of namespace Sci
