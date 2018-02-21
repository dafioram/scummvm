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

#ifndef SCI_S2_GAME_H
#define SCI_S2_GAME_H

#include "common/random.h"
#include "common/serializer.h"
#include "sci/s2/cursor.h"
#include "sci/s2/flags.h"
#include "sci/s2/interface.h"
#include "sci/s2/inventory_manager.h"
#include "sci/s2/movie_manager.h"
#include "sci/s2/phone_manager.h"
#include "sci/s2/room_manager.h"
#include "sci/s2/savegame.h"
#include "sci/s2/scoring_manager.h"
#include "sci/s2/sound_manager.h"
#include "sci/s2/system/glplane_manager.h"
#include "sci/s2/system/glset.h"
#include "sci/s2/system/gluser.h"
#include "sci/s2/system/types.h"

class Engine;

namespace Sci {

class GLObject;
class S2Engine;
class S2Kernel;

/** This class combines the functionality of S2Game and GLGame from SSCI. */
class S2Game : public Common::Serializable {
public:
#pragma mark -
#pragma mark ScummVM extensions
	S2Game(S2Engine &engine, S2Kernel &kernel);

	virtual void saveLoadWithSerializer(Common::Serializer &) override;

	/**
	 * Runs the game. Returns once the game has finished.
	 */
	void run();

	void quit();

	bool hasSaveGames() const;
	Common::Array<S2SaveGameMetadata> getSaveGameList() const;
	bool canSaveNow() const;
	bool canLoadNow() const;

	uint getRandomNumber(const uint low, const uint high) {
		return _rng.getRandomNumberRng(low, high);
	}

private:
	S2Engine &_engine;
	S2Kernel &_kernel;

	Common::RandomSource _rng;

#pragma mark -
#pragma mark GLGame
public:
	GLPlaneManager &getPlanes() { return _planes; }
	GLUser &getUser() { return _user; }
	S2Cursor &getCursor() { return _cursor; }
	GLExtras &getExtras() { return _extras; }

	// In SSCI, this was Win32 LoadMessage
	Common::String getMessage(const uint16 resourceNo) const;

private:
	void play();
	void doIt();

	GLPlaneManager _planes;
	GLUser _user;
	S2Cursor _cursor;
	GLExtras _extras;

#pragma mark -
#pragma mark S2Game
public:
	S2SoundManager &getSoundManager() { return _soundManager; }
	S2RoomManager &getRoomManager() { return _roomManager; }
	S2Interface &getInterface() { return _interface; }
	S2PhoneManager &getPhoneManager() { return _phoneManager; }
	S2MovieManager &getMovieManager() { return _movieManager; }
	S2InventoryManager &getInventoryManager() { return _inventoryManager; }
	S2ScoringManager &getScoringManager() { return _scoringManager; }
	GameFlags &getFlags() { return _flags; }

	const Common::String &getSaveGameName() const { return _saveGameName; }
	void setSaveGameName(const Common::String &name) { _saveGameName = name; }

	void save(const bool autoSave);
	void load(const bool autoLoad);

	int getPanSpeed() const { return _panSpeed; }

private:
	void init();

	S2SoundManager _soundManager;
	S2RoomManager _roomManager;
	S2Interface _interface;
	S2PhoneManager _phoneManager;
	S2MovieManager _movieManager;
	S2InventoryManager _inventoryManager;
	S2ScoringManager _scoringManager;
	GameFlags _flags;

	Common::String _saveGameName;
	int _saveGameSlotNo;

	uint8 _gamma;
	uint8 _panSpeed;
};

} // End of namespace Sci

#endif
