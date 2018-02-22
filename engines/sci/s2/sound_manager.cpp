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

void S2SoundManager::saveLoadWithSerializer(Common::Serializer &s) {
	int roomNo = _roomNo;
	if (s.isSaving() && _game.getRoomManager().getLastSoundRoomNo()) {
		roomNo = _game.getRoomManager().getLastSoundRoomNo();
	}
	s.syncAsSint32LE(roomNo);
	if (s.isLoading()) {
		createAmbient(roomNo);
	}
}

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

	const auto ambientNo(roomNo ? roomNo : _game.getRoomManager().getCurrentAmbientRoomNo());

	switch (ambientNo) {
	// This case did not exist in SSCI, it exists because we raise an error for
	// unknown ambient room numbers, whereas SSCI silently did nothing, and this
	// room number will be used when loading a save game from the main menu at
	// game startup
	case 1:
		break;

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
		play(10630, true, 0);
		fade(10630, 80, 15, 12);

		_tracks[0] = &createSoundTrack();
		_tracks[0]->addWaitNode(30, 50);
		_tracks[0]->addPRSNode(10631, 80, 60);
		_tracks[0]->addPRSNode(10632, 80, 60);

		_tracks[1] = &createSoundTrack();
		_tracks[1]->addWaitNode(90, 120);
		_tracks[1]->addPRSNode(10634, 100, 30, _game.getRandomNumber(0, 100));
		_tracks[1]->addPRSNode(10635, 100, 30, _game.getRandomNumber(0, 100));
		_tracks[1]->addPRSNode(10636, 100, 30, _game.getRandomNumber(0, 100));
		_tracks[1]->addPRSNode(10637, 100, 30, _game.getRandomNumber(0, 100));

		_tracks[2] = &createSoundTrack();
		_tracks[2]->addWaitNode(30);
		_tracks[2]->addPRSNode(10638, 80, 30, _game.getRandomNumber(0, 100));
		break;

	case 7:
		play(10633, true, 0);
		fade(10633, 80, 15, 12);

		_tracks[0] = &createSoundTrack();
		_tracks[0]->addWaitNode(30, 50);
		_tracks[0]->addPRSNode(10631, 80, 60);
		_tracks[0]->addPRSNode(10632, 80, 60);

		_tracks[1] = &createSoundTrack();
		_tracks[1]->addWaitNode(90, 120);
		_tracks[1]->addPRSNode(10634, 80, 30, _game.getRandomNumber(0, 100));
		_tracks[1]->addPRSNode(10635, 80, 30, _game.getRandomNumber(0, 100));
		_tracks[1]->addPRSNode(10636, 80, 30, _game.getRandomNumber(0, 100));
		_tracks[1]->addPRSNode(10637, 80, 30, _game.getRandomNumber(0, 100));

		_tracks[2] = &createSoundTrack();
		_tracks[2]->addWaitNode(30);
		_tracks[2]->addPRSNode(10638, 80, 30, _game.getRandomNumber(0, 100));
		break;

	case 8:
		play(10639, true, 0);
		fade(10639, 80, 15, 12);

		_tracks[0] = &createSoundTrack();
		_tracks[0]->addWaitNode(30, 50);
		_tracks[0]->addPRSNode(10631, 80, 60);
		_tracks[0]->addPRSNode(10632, 80, 60);

		_tracks[1] = &createSoundTrack();
		_tracks[1]->addWaitNode(90, 120);
		_tracks[1]->addPRSNode(10634, 80, 30, _game.getRandomNumber(0, 100));
		_tracks[1]->addPRSNode(10635, 80, 30, _game.getRandomNumber(0, 100));
		_tracks[1]->addPRSNode(10636, 80, 30, _game.getRandomNumber(0, 100));
		_tracks[1]->addPRSNode(10637, 80, 30, _game.getRandomNumber(0, 100));

		_tracks[2] = &createSoundTrack();
		_tracks[2]->addWaitNode(30);
		_tracks[2]->addPRSNode(10638, 80, 30, _game.getRandomNumber(0, 100));
		break;

	case 9:
		play(12920, true, 0);
		fade(12920, 80, 15, 12);

		_tracks[0] = &createSoundTrack();
		_tracks[0]->addWaitNode(10, 90);
		_tracks[0]->addPRSNode(12921, 80, 100, _game.getRandomNumber(0, 100));
		_tracks[0]->addPRSNode(12922, 80, 100, _game.getRandomNumber(0, 100));
		_tracks[0]->addPRSNode(12923, 80, 100, _game.getRandomNumber(0, 100));
		_tracks[0]->addPRSNode(12924, 80, 100, _game.getRandomNumber(0, 100));
		_tracks[0]->addPRSNode(12925, 80, 100, _game.getRandomNumber(0, 100));
		_tracks[0]->addPRSNode(12926, 80, 100, _game.getRandomNumber(0, 100));
		_tracks[0]->addPRSNode(12927, 80, 100, _game.getRandomNumber(0, 100));

		_tracks[1] = &createSoundTrack();
		_tracks[1]->addWaitNode(120);
		_tracks[1]->addPRSNode(12928, 80, 30, _game.getRandomNumber(0, 100));

		_tracks[2] = &createSoundTrack();
		_tracks[2]->addWaitNode(2, 5);
		_tracks[2]->addSoundNode(32901, 80);
		_tracks[2]->addWaitNode(2, 5);
		_tracks[2]->addSoundNode(32901, 80);
		_tracks[2]->addWaitNode(2, 5);
		_tracks[2]->addSoundNode(32902, 80);
		_tracks[2]->addWaitNode(2, 5);
		_tracks[2]->addSoundNode(32903, 80);
		break;

	case 10:
	case 11:
	case 12:
	case 30:
	case 31:
	case 32:
		play(31001, true, 0);
		fade(31001, 100, 15, 16, false);
		break;

	case 13:
	case 33:
		play(31301, true, 80);
		break;

	case 14:
	case 34:
		play(31401, true, 0);
		fade(31401, 100, 15, 12);
		if (_game.getFlags().get(kGameFlag158) && !_game.getFlags().get(kGameFlag159)) {
			_tracks[0] = &createSoundTrack();
			_tracks[0]->addPRSNode(21401, 75, 100);
			_tracks[0]->addPRSNode(21402, 75, 100);
			_tracks[0]->addPRSNode(21403, 75, 100);
			_tracks[0]->addPRSNode(21404, 75, 100);
			_tracks[0]->addPRSNode(21405, 75, 100);
			_tracks[0]->addPRSNode(21406, 75, 100);
			_tracks[0]->addWaitNode(3, _game.getRandomNumber(3, 15));
		}
		break;

	case 15:
	case 35:
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

	case 16:
	case 36:
		play(31601, true, 0);
		fade(31601, 80, 15, 12, false);
		break;

	case 17:
	case 37:
		play(31701, true, 0);
		fade(31701, 100, 15, 12, false);
		play(31703, true, 0);
		fade(31703, 100, 15, 12, false);
		play(31704, true, 0);
		fade(31704, 100, 15, 12, false);
		break;

	case 18:
	case 38:
		play(31801, true, 0);
		fade(31801, 100, 15, 16, false);
		_tracks[0] = &createSoundTrack();
		_tracks[0]->addWaitNode(8, 12);
		_tracks[0]->addPRSNode(31802, 100, 64, _game.getRandomNumber(0, 100));
		_tracks[0]->addPRSNode(31803, 100, 64, _game.getRandomNumber(0, 100));
		_tracks[0]->addPRSNode(31804, 100, 64, _game.getRandomNumber(0, 100));
		_tracks[0]->addPRSNode(31805, 100, 64, _game.getRandomNumber(0, 100));
		_tracks[0]->addPRSNode(31806, 100, 64, _game.getRandomNumber(0, 100));
		_tracks[0]->addPRSNode(31807, 100, 64, _game.getRandomNumber(0, 100));
		break;

	case 19:
	case 39:
		play(31901, true, 100);
		break;

	case 20:
	case 40:
		_tracks[0] = &createSoundTrack();
		_tracks[0]->addWaitNode(3, 6);
		_tracks[0]->addPRSNode(32001, 100, 80, 20);
		_tracks[0]->addPRSNode(32001, 100, 80, 20);
		_tracks[0]->addPRSNode(32002, 100, 80, 20);
		_tracks[0]->addPRSNode(32003, 100, 80, 20);

		_tracks[1] = &createSoundTrack();
		_tracks[1]->addWaitNode(1, 3);
		_tracks[1]->addPRSNode(32004, 100, 80, 80);
		_tracks[1]->addPRSNode(32004, 100, 80, 80);
		_tracks[1]->addPRSNode(32005, 100, 80, 80);
		_tracks[1]->addPRSNode(32006, 100, 80, 80);
		break;

	case 21:
	case 41:
		play(32101, true, 0);
		fade(32101, 100, 15, 16, false);
		break;

	case 22:
	case 42:
		play(32201, true, 100);
		break;

	case 23:
	case 43:
		play(32301, true, 0);
		fade(32301, 100, 15, 16, false);
		break;

	case 24:
	case 44:
		play(32401, true, 0);
		fade(32401, 80, 15, 12, false);
		break;

	case 25:
	case 45:
		play(32501, true, 80);
		break;

	case 26:
	case 46:
		play(32601, true, 100);
		pan(32602, 0);
		play(32602, true, 100);
		pan(32607, 100);
		play(32607, true, 100);

		_tracks[0] = &createSoundTrack();
		_tracks[0]->addWaitNode(3, 7);
		_tracks[0]->addPRSNode(32603, 100, 64, _game.getRandomNumber(25, 75));
		_tracks[0]->addPRSNode(32605, 100, 64, _game.getRandomNumber(25, 75));
		_tracks[0]->addPRSNode(32606, 100, 64, _game.getRandomNumber(25, 75));
		break;

	case 27:
	case 47:
		play(32701, true, 0);
		fade(32701, 80, 15, 12);

		_tracks[0] = &createSoundTrack();
		_tracks[0]->addWaitNode(3, 7);
		_tracks[0]->addPRSNode(32702, 100, 72);
		_tracks[0]->addPRSNode(32703, 100, 72);
		_tracks[0]->addPRSNode(32704, 100, 72);
		_tracks[0]->addPRSNode(32705, 100, 72);
		break;

	case 28:
	case 48:
		play(12840, true, 0);
		fade(12840, 80, 15, 12, false);
		break;

	case 29:
	case 49:
		break;

	case 50:
		_tracks[0] = &createSoundTrack();
		_tracks[0]->addPRSNode(32901, 100, 72);
		_tracks[0]->addWaitNode(2, 5);
		_tracks[0]->addPRSNode(32904, 100, 72);
		_tracks[0]->addPRSNode(32905, 100, 72);
		_tracks[0]->addPRSNode(32901, 100, 72);
		break;

	default:
		error("Invalid ambient sound %d", ambientNo);
	}

	for (auto &&track : _tracks) {
		if (track) {
			track->play();
		}
	}

	_roomNo = roomNo;
}

void S2SoundManager::deleteAmbient(int roomNo) {
	if (!roomNo) {
		roomNo = _roomNo;
	}

	for (auto &&track : _tracks) {
		if (track) {
			deleteSoundTrack(track->getTrackId());
			track = nullptr;
		}
	}

	for (auto &&sound : _sounds) {
		if (sound) {
			fade(sound, 0, 15, 12, true);
			sound = 0;
		}
	}

	switch (roomNo) {
	case 6:
		fade(10630, 0, 15, 12, true);
		break;

	case 7:
		fade(10633, 0, 15, 12, true);
		break;

	case 8:
		fade(10639, 0, 15, 12, true);
		break;

	case 9:
		fade(12920, 0, 15, 12, true);
		break;

	case 10:
	case 11:
	case 12:
	case 30:
	case 31:
	case 32:
		fade(31001, 0, 15, 12, true);
		break;

	case 13:
	case 33:
		fade(31301, 0, 10, 16, true);
		break;

	case 14:
	case 34:
		fade(31401, 0, 15, 12, true);
		break;

	case 16:
	case 36:
		fade(31601, 0, 15, 12, true);
		break;

	case 17:
	case 37:
		fade(31701, 0, 15, 12, true);
		fade(31703, 0, 15, 12, true);
		fade(31704, 0, 15, 12, true);
		break;

	case 18:
	case 38:
		fade(31801, 0, 10, 16, true);
		break;

	case 19:
	case 39:
		fade(31901, 0, 15, 12, true);
		break;

	case 21:
	case 41:
		fade(32101, 0, 15, 12, true);
		break;

	case 22:
	case 42:
		fade(32201, 0, 10, 16, true);
		break;

	case 23:
	case 43:
		fade(32301, 0, 10, 16, true);
		break;

	case 24:
	case 44:
		fade(32401, 0, 15, 12, true);
		break;

	case 25:
	case 45:
		fade(32501, 0, 15, 12, true);
		break;

	case 26:
	case 46:
		fade(32601, 0, 15, 12, true);
		fade(32602, 0, 15, 12, true);
		fade(32607, 0, 15, 12, true);
		break;

	case 27:
	case 47:
		fade(32701, 0, 15, 12, true);
		break;

	case 28:
	case 48:
		// TODO: Does not stop on fade, is this a bug?
		fade(12840, 0, 15, 12);
		break;
	}
}

} // End of namespace Sci
