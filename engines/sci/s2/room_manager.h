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
class S2Kernel;
class S2Room;

class S2RoomManager : public GLObject {
public:
	S2RoomManager(S2Kernel &kernel, S2Game &game);
	virtual ~S2RoomManager();

	int getCurrentRoom() const { return _currentRoomNo; }
	bool getIsSaved() const { return _isSaved; }

	bool loadRoom(const int roomNo);
	void initRoom(const int roomNo);
	void disposeRoom(const int roomNo);
	void unloadRoom();
	void activateRoom();
	void deactivateRoom();

	void loadGlobalRoom(const int roomNo, const bool fullscreen);
	void initGlobalRoom(const int roomNo);
	void disposeGlobalRoom();
	void unloadGlobalRoom();

	void drawPan(const uint16 resourceNo);
	void drawPic(const uint16 resourceNo, const bool fullscreen);

	int getLastSoundRoomNo() const { return _lastSoundRoomNo; }
	void setLastSoundRoomNo(const int roomNo) { _lastSoundRoomNo = roomNo; }

private:
	S2Kernel &_kernel;
	S2Game &_game;
	bool _isSaved;

	int _previousRoomNo;
	int _currentRoomNo;

	Common::ScopedPtr<S2Room> _currentRoom;

	int _lastSoundRoomNo;

	Common::ScopedPtr<GLPicturePlane> _plane;
	bool _planeIsVisible;
	Common::ScopedPtr<GLPanorama> _panorama;
	Common::Array<S2Exit> _exits;
	Common::Array<S2Hotspot> _hotspots;
	Common::Array<GLCel> _cels;

	Common::ScopedPtr<GLPicturePlane> _globalPlane;
	int _currentGlobalRoomNo;
	int _lastNonGlobalRoomNo;
};

} // End of namespace Sci

#endif
