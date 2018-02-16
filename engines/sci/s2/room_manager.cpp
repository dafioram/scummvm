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
#include "sci/s2/kernel.h"
#include "sci/s2/room.h"
#include "sci/s2/room_manager.h"
#include "sci/s2/rooms/1000.h"
#include "sci/s2/rooms/global.h"
#include "sci/s2/system/glpanorama.h"
#include "sci/s2/system/glplane.h"

namespace Sci {

S2RoomManager::S2RoomManager(S2Kernel &kernel, S2Game &game) :
	_kernel(kernel),
	_game(game),
	_isSaved(true),
	_roomIsActive(true),
	_autoHighlight(true),
	_previousRoomNo(0),
	_currentRoomNo(0),
	_nextRoomNo(0),
	_lastSoundRoomNo(0),
	_pictureIsVisible(false),
	_currentPictureNo(0),
	_panoramaIsVisible(false),
	_currentGlobalRoomNo(0),
	_lastNonGlobalRoomNo(0) {
	game.getExtras().push_back(this);
	game.getUser().getOrphans().push_back(this);
}

S2RoomManager::~S2RoomManager() {
	_game.getExtras().remove(this);
	if (_picture && _pictureIsVisible) {
		_game.getPlanes().remove(*_picture);
		_pictureIsVisible = false;
		// TODO: Dtor was called explicitly here regardless of visibility; make
		// sure it has no side effects which are important
	}
	if (_panorama && _pictureIsVisible) {
		_game.getUser().getOrphans().remove(_panorama.get());
		_game.getExtras().remove(_panorama.get());
		// TODO: Dtor was called explicitly here regardless of visibility; make
		// sure it has no side effects which are important
	}
}

void S2RoomManager::doIt() {
	if (!_currentRoomNo) {
		return;
	}

	if (_nextRoomNo) {
		newRoom(_nextRoomNo);
		_nextRoomNo = 0;
	} else { 
		if (_pictureIsVisible && !_game.getCursor().hasInventory()) {
			if (_game.getUser().getIsHandsOn()) {
				if (_game.getCursor().isHandsOff()) {
					_game.getCursor().goHandsOn();
				}
				checkMouse();
			} else if (!_game.getCursor().isHandsOff()) {
				_game.getCursor().endHighlight();
				_game.getCursor().goHandsOff();
			}
		}

		if (_currentRoom) {
			_currentRoom->doIt();
		}
	}
}

bool S2RoomManager::handleEvent(GLEvent &event) {
	if (_currentGlobalRoomNo) {
		_globalRoom->handleEvent(event);
		return true;
	}

	return _currentRoom->handleEvent(event);
}

void S2RoomManager::newRoom(const int roomNo) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

bool S2RoomManager::loadRoom(const int roomNo) {
	// In SSCI this loaded DLLs
	switch (roomNo) {
	case 1000:
		_currentRoom.reset(new S2Room1000(_kernel, _game));
		break;
	case 6000:
	case 10000:
	case 11000:
	case 12000:
	case 13000:
	case 14000:
	case 15000:
	case 16000:
	case 17000:
	case 18000:
	case 19000:
	case 20000:
	case 21000:
	case 22000:
	case 23000:
	case 24000:
	case 25000:
	case 26000:
	case 27000:
	case 28000:
	case 30000:
	default:
		error("Invalid room %d", roomNo);
	}

	return true;
}

void S2RoomManager::initRoom(const int roomNo) {
	assert(_currentRoom);
	_kernel.eventManager.flushEvents();
	_previousRoomNo = _currentRoomNo;
	_currentRoomNo = roomNo;
	_currentRoom->init(roomNo);
}

void S2RoomManager::activateRoom() {
	_roomIsActive = true;
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2RoomManager::deactivateRoom() {
	_roomIsActive = false;
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2RoomManager::loadGlobalRoom(const int roomNo, const bool fullscreen) {
	const Common::Rect fullscreenRect(_kernel.graphicsManager.getScriptWidth() - 1,
									  _kernel.graphicsManager.getScriptHeight() - 1);

	// In SSCI the global room always existed and was baked into the main
	// executable image; we treat it more like a regular room and so it needs to
	// be initialised once when something tries to use it
	// TODO: It may be possible to delete the global room in unloadGlobalRoom to
	// avoid some extra deinitialisation code; check this once the room is
	// actually implemented
	if (!_globalRoom) {
		_globalRoom.reset(new S2GlobalRoom(_kernel, _game));
	}

	if (!_currentGlobalRoomNo) {
		_game.getMovieManager().pauseRobot();
		_game.getPhoneManager().cancelCall();
		_game.getInterface().putText(0);
		_lastSoundRoomNo = _game.getSoundManager().getRoomNo();
		_lastNonGlobalRoomNo = getCurrentRoom();
		_game.getSoundManager().deleteAmbient(_lastSoundRoomNo);
		_game.getSoundManager().stop();
		_game.getSoundManager().play(30004, true, 0);
		_game.getSoundManager().fade(30004, 80, 15, 12, false);
		if (roomNo >= 4200 && roomNo <= 4240 && _picture && _pictureIsVisible) {
			_game.getPlanes().remove(*_picture);
			_picture.reset();
			_pictureIsVisible = false;
		}
		deactivateRoom();

		if (fullscreen) {
			_game.getInterface().hide();
			_globalPlane.reset(new GLPicturePlane(fullscreenRect, roomNo, 200));
		} else {
			_globalPlane.reset(new GLPicturePlane(fullscreenRect, 1, 3));
			_globalPlane->addPicAt(roomNo, 64, 0);
		}
		_game.getPlanes().add(*_globalPlane);
	} else {
		_globalRoom->dispose(_currentGlobalRoomNo);
		const auto planeRect(_globalPlane->getRect());
		if (fullscreen && planeRect.height() != _kernel.graphicsManager.getScriptHeight()) {
			_game.getPlanes().remove(*_globalPlane);
			_globalPlane.reset(new GLPicturePlane(fullscreenRect, roomNo, 200));
			_game.getPlanes().add(*_globalPlane);
		} else if (!fullscreen) {
			if (planeRect.height() == _kernel.graphicsManager.getScriptHeight()) {
				_game.getPlanes().remove(*_globalPlane);
				_game.getInterface().show();
				_globalPlane.reset(new GLPicturePlane(fullscreenRect, 1, 3));
				_globalPlane->addPicAt(roomNo, 64, 0);
			} else {
				_globalPlane->addPicAt(roomNo, 64, 0);
			}
		}
	}

	_currentGlobalRoomNo = roomNo;
	_globalRoom->init(roomNo);
}

void S2RoomManager::unloadGlobalRoom() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2RoomManager::drawPan(const uint16 resourceNo) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2RoomManager::drawPic(const uint16 resourceNo, const bool fullscreen) {
	if (fullscreen) {
		_game.getInterface().hide();
	} else {
		_game.getInterface().show();
	}

	if (_panorama && _panoramaIsVisible) {
		_game.getExtras().remove(_panorama.get());
		_game.getUser().getOrphans().remove(_panorama.get());
		warning("TODO: Finish clearing panorama");
		_panoramaIsVisible = false;
	}

	if (!_picture) {
		const Common::Rect fullscreenRect(0, 0,
										  _kernel.graphicsManager.getScriptWidth() - 1,
										  _kernel.graphicsManager.getScriptHeight() - 1);
		if (fullscreen) {
			_picture.reset(new GLPicturePlane(fullscreenRect, resourceNo));
		} else {
			_picture.reset(new GLPicturePlane(fullscreenRect, 1));
			_picture->addPicAt(resourceNo, 64, 0);
		}
	} else if (resourceNo != _currentPictureNo) {
		if (fullscreen) {
			_picture->setPic(resourceNo, true);
		} else {
			// In SSCI this called through the kernel; for consistency this is
			// changed to use a plane method instead
			_picture->deletePic(_currentPictureNo);

			if (_picture->getPicNo() != 1) {
				_picture->setPic(1);
			}
			_picture->addPicAt(resourceNo, 64, 0);
		}
	}

	if (!_pictureIsVisible) {
		_picture->setPriority(2, true);
		_game.getPlanes().add(*_picture);
		_pictureIsVisible = true;
	} else {
		_picture->repaint();
	}

	_currentPictureNo = resourceNo;
}

void S2RoomManager::checkMouse() {
	if (!_roomIsActive) {
		return;
	}

	Common::Point mousePosition = _game.getUser().getMousePosition();
	bool hit = false;
	if (_picture->checkIsOnMe(mousePosition)) {
		GLCelRes highlightedCel = _game.getCursor().getHighlightedCelRes();
		const auto localMousePosition(_picture->toLocal(mousePosition));

		for (const auto &exit : _exits) {
			if ((exit.getCursorCel() != 1 || _autoHighlight) &&
				exit.getIsEnabled() &&
				exit.checkIsOnMe(localMousePosition) &&
				exit.getCursorCel() != 0) {
				hit = true;

				if (highlightedCel.celNo != exit.getCursorCel()) {
					highlightedCel.celNo = exit.getCursorCel();
					_game.getCursor().setHighlightedCelRes(highlightedCel);
				}

				break;
			}
		}

		if (!hit && _autoHighlight) {
			for (const auto &hotspot : _hotspots) {
				if (hotspot.getIsEnabled() && hotspot.checkIsOnMe(localMousePosition)) {
					hit = true;

					if (highlightedCel.celNo != 1) {
						highlightedCel.celNo = 1;
						_game.getCursor().setHighlightedCelRes(highlightedCel);
					}

					break;
				}
			}
			if (!hit) {
				for (const auto &cel : _cels) {
					if (cel.checkIsOnMe(localMousePosition)) {
						hit = true;

						if (highlightedCel.celNo != 1) {
							highlightedCel.celNo = 1;
							_game.getCursor().setHighlightedCelRes(highlightedCel);
						}

						break;
					}
				}
			}
		}
	}

	if (hit && !_game.getCursor().isHighlighted()) {
		_game.getCursor().beginHighlight();
	} else if (!hit && _game.getCursor().isHighlighted()) {
		_game.getCursor().endHighlight();
	}
}

} // End of namespace Sci
