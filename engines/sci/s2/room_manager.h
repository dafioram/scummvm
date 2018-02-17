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

#ifndef SCI_S2_ROOM_MANAGER_H
#define SCI_S2_ROOM_MANAGER_H

#include "common/ptr.h"
#include "common/scummsys.h"
#include "sci/s2/exit.h"
#include "sci/s2/hotspot.h"
#include "sci/s2/system/glcel.h"
#include "sci/s2/system/globject.h"

namespace Sci {

class GLPanorama;
class GLPicturePlane;
class S2Game;
class S2GlobalRoom;
class S2Kernel;
class S2Room;

class S2RoomManager : public GLObject {
public:
	S2RoomManager(S2Kernel &kernel, S2Game &game);
	virtual ~S2RoomManager();

	int getCurrentRoomNo() const { return _currentRoomNo; }
	bool getIsSaved() const { return _isSaved; }

	void doIt() override;
	bool handleEvent(GLEvent &event) override;

	void newRoom(const int roomNo);
	bool loadRoom(const int roomNo);
	void initRoom(const int roomNo);
	void disposeRoom(const int roomNo);
	void unloadRoom();
	void activateRoom();
	void deactivateRoom();

	GLPicturePlane *getGlobalPlane() const { return _globalPlane.get(); }
	void loadGlobalRoom(const int roomNo, const bool fullscreen);
	void unloadGlobalRoom();

	void drawPan(const uint16 resourceNo);
	void drawPic(const uint16 resourceNo, const bool fullscreen);

	int getLastSoundRoomNo() const { return _lastSoundRoomNo; }
	void setLastSoundRoomNo(const int roomNo) { _lastSoundRoomNo = roomNo; }

	int getCurrentGlobalRoomNo() const { return _currentGlobalRoomNo; }

private:
	// If a button callback calls to load a new room it will cause a
	// use-after-free. We can either switch everything to shared pointers to
	// avoid disposing of objects early, or we can defer the load until the next
	// doIt loop when the event handler has cleared. We do the latter for now.
	void deferredLoadGlobalRoom(const int roomNo, const bool fullscreen);

	void checkMouse();

	S2Kernel &_kernel;
	S2Game &_game;
	bool _isSaved;
	bool _roomIsActive;
	bool _autoHighlight;

	int _previousRoomNo;
	int _currentRoomNo;
	int _nextRoomNo;

	Common::ScopedPtr<S2Room> _currentRoom;

	int _lastSoundRoomNo;

	Common::ScopedPtr<GLPicturePlane> _picture;
	bool _pictureIsVisible;
	uint16 _currentPictureNo;
	Common::ScopedPtr<GLPanorama> _panorama;
	bool _panoramaIsVisible;
	Common::Array<S2Exit> _exits;
	Common::Array<S2Hotspot> _hotspots;
	Common::Array<GLCel> _cels;

	// Keep these in order; `_globalPlane` must be destroyed *after*
	// `_globalRoom` (or else changed to use a SharedPtr)
	Common::ScopedPtr<GLPicturePlane> _globalPlane;
	Common::ScopedPtr<S2GlobalRoom> _globalRoom;
	int _currentGlobalRoomNo;
	int _nextGlobalRoomNo;
	bool _nextGlobalRoomFullscreen;
	int _lastNonGlobalRoomNo;
};

} // End of namespace Sci

#endif
