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

#ifndef SCI_S2_ENGINE_H
#define SCI_S2_ENGINE_H

#include "common/array.h"
#include "common/error.h"
#include "common/ptr.h"
#include "engines/engine.h"

#include "sci/engine/features.h"
#include "sci/resource/manager.h"
#include "sci/event.h"
#include "sci/s2/cursor.h"
#include "sci/s2/debugger.h"
#include "sci/s2/flags.h"
#include "sci/s2/interface.h"
#include "sci/s2/inventory_manager.h"
#include "sci/s2/movie_manager.h"
#include "sci/s2/phone_manager.h"
#include "sci/s2/room_manager.h"
#include "sci/s2/sound_manager.h"
#include "sci/s2/system/globject.h"
#include "sci/s2/system/glplane_manager.h"
#include "sci/s2/system/gluser.h"
#include "sci/sci.h"
#include "sci/time.h"

class OSystem;

namespace Sci {

class Audio32;
class GfxFrameout;
class S2Game;

/**
 * This class combines SCI initialisation/kernel, GLGame, and S2Game classes.
 */
class S2Engine : public Engine {
public:
	S2Engine(OSystem &system, const GameMetadata &metadata);
	virtual void initializePath(const Common::FSNode &gamePath) override;
	virtual Common::Error run() override;
	virtual GUI::Debugger *getDebugger() override { return &_debugger; }

	const Common::Point &getMousePosition() const { error("TODO: %s", __PRETTY_FUNCTION__); }
	void setCursor(const S2Cursor &cursor) { _cursor = cursor; }

	// In SSCI this was Win32 LoadMessage
	Common::String getMessage(const uint16 resourceNo) const;

private:
	OSystem &_system;
	GameMetadata _metadata;
	ResourceManager _resourceManager;
	GameFeatures _features;
	S2Debugger _debugger;
	EventManager _eventManager;
	TimeManager _timeManager;
	Common::ScopedPtr<Audio32> _audioMixer;
	Common::ScopedPtr<GfxFrameout> _graphicsManager;

#pragma mark -
#pragma mark GLGame
private:
	void play();
	void doIt();

	GLPlaneManager _planes;
	GLUser _user;
	S2Cursor _cursor;
	GLSetAsArray<GLObject> _extras;

#pragma mark -
#pragma mark S2Game
private:
	void init();

	S2SoundManager _soundManager;
	S2RoomManager _roomManager;
	S2Interface _interface;
	S2MovieManager _movieManager;
	S2InventoryManager _inventoryManager;
	S2PhoneManager _phoneManager;
	GameFlags _flags;

	int _volume; // ?
	int _field_54;
	int _field_58;
	int _field_5c;
	int _field_60;
};

} // End of namespace Sci

#endif
