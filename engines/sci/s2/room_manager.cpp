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
#include "sci/s2/rooms/10000.h"
#include "sci/s2/rooms/global.h"
#include "sci/s2/system/glpanorama.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/glvr_plane.h"

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
	_savedPanX(0),
	_savedPanY(0),
	_currentGlobalRoomNo(0),
	_lastNonGlobalRoomNo(0),
	_numRoomTransitions(0) {
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

void S2RoomManager::saveLoadWithSerializer(Common::Serializer &s) {
	const auto oldRoomNo = _currentRoomNo;
	const auto oldPreviousRoomNo = _previousRoomNo;

	auto roomNo = _currentRoomNo;
	auto previousRoomNo = _previousRoomNo;

	if (s.isSaving()) {
		switch (roomNo) {
		case 6222:
			previousRoomNo = roomNo;
			roomNo = 6220;
			break;
		case 6292:
			previousRoomNo = roomNo;
			roomNo = 6290;
			break;
		case 6122:
			previousRoomNo = roomNo;
			roomNo = 6120;
			break;
		case 6272:
			previousRoomNo = roomNo;
			roomNo = 6279;
			break;
		case 6422:
			previousRoomNo = roomNo;
			roomNo = 6420;
			break;
		case 6351:
		case 6353:
			previousRoomNo = roomNo;
			roomNo = 6350;
			break;
		case 6371:
			previousRoomNo = roomNo;
			roomNo = 6375;
			break;
		case 11140:
			previousRoomNo = roomNo;
			roomNo = 11100;
			break;
		case 14240:
			previousRoomNo = roomNo;
			roomNo = 14200;
			break;
		case 14430:
			previousRoomNo = roomNo;
			roomNo = 14420;
			break;
		case 15601:
			previousRoomNo = roomNo;
			roomNo = 15600;
			break;
		case 15370:
			previousRoomNo = roomNo;
			roomNo = 15300;
			break;
		case 36132:
			previousRoomNo = roomNo;
			roomNo = 36100;
			break;
		case 17450:
			previousRoomNo = roomNo;
			roomNo = 17410;
			break;
		case 19450:
			previousRoomNo = roomNo;
			roomNo = 19420;
			break;
		case 21390:
		case 21321:
			previousRoomNo = roomNo;
			roomNo = 21300;
			break;
		case 21113:
			previousRoomNo = roomNo;
			roomNo = 21100;
			break;
		case 21290:
			previousRoomNo = roomNo;
			roomNo = 21200;
			break;
		case 22730:
			previousRoomNo = roomNo;
			roomNo = 22600;
			break;
		case 23150:
			previousRoomNo = roomNo;
			roomNo = 23140;
			break;
		case 24531:
			previousRoomNo = roomNo;
			roomNo = 24500;
			break;
		case 48201:
		case 48205:
		case 48207:
		case 48212:
			previousRoomNo = roomNo;
			roomNo = 48000;
			break;
		case 48202:
		case 48206:
		case 48210:
		case 48211:
			previousRoomNo = roomNo;
			roomNo = 28400;
			break;
		case 48203:
		case 48204:
		case 48208:
		case 48209:
			previousRoomNo = roomNo;
			roomNo = 28700;
			break;
		}
	}

	s.syncAsByte(_autoHighlight);
	s.syncAsSint32LE(roomNo);
	s.syncAsSint32LE(previousRoomNo);

	if (s.isLoading()) {
		_currentRoomNo = roomNo;
		_previousRoomNo = previousRoomNo;
	}

	if (_panorama) {
		s.syncAsSint16LE(_panorama->panX());
		s.syncAsSint16LE(_panorama->panY());
	} else {
		s.syncAsSint16LE(_savedPanX);
		s.syncAsSint16LE(_savedPanY);
	}

	// This code was originally in S2Game::Load
	if (s.isLoading()) {
		_game.getCursor().endHighlight();
		if (roomNo) {
			disposeRoom(oldRoomNo);
			unloadRoom();
		}

		loadRoom(roomNo);
		_nextRoomNo = 0;
		_currentRoomNo = oldPreviousRoomNo;
		initRoom(roomNo);

		if (_currentGlobalRoomNo) {
			unloadGlobalRoom();
		}
	}

	_isSaved = true;
}

bool S2RoomManager::inInteractiveRoom() const {
	return _currentRoomNo >= 2000 &&
		_currentRoomNo != 26900 &&
		_currentRoomNo != 26901 &&
		_currentRoomNo != 26721 &&
		_currentRoomNo != 26850;
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

void S2RoomManager::newRoom(int roomNo) {
	if (_currentRoomNo != roomNo) {
		const bool baseRoomChanged = getBaseRoomNumber(_currentRoomNo) != getBaseRoomNumber(roomNo);

		_game.getCursor().endHighlight();
		if (_currentRoom) {
			_nextRoomNo = roomNo;
			disposeRoom(_currentRoomNo);
			if (baseRoomChanged) {
				unloadRoom();
			}
		}

		if (baseRoomChanged) {
			if (!loadRoom(roomNo)) {
				loadRoom(_currentRoomNo);
				roomNo = _currentRoomNo;
			}
		}

		_nextRoomNo = 0;
		initRoom(roomNo);

		if (_panoramaIsVisible) {
			_panorama->updatePanorama();
			_panorama->getPlane().updateScreen();
		}

		_game.getPhoneManager().notifyRoomChange(baseRoomChanged);
		_isSaved = false;
		if (++_numRoomTransitions == 5) {
			_numRoomTransitions = 0;
			if (_game.getInventoryManager().getPrayerStickId() != S2PrayerStick::None) {
				_game.getInterface().changeLife(-1);
			}
		}
	}

	if (_currentGlobalRoomNo != 0 && _currentGlobalRoomNo != 4200) {
		unloadGlobalRoom();
		if (_panoramaIsVisible) {
			_panorama->getPlane().updateScreen();
		}
	}
}

bool S2RoomManager::loadRoom(const int roomNo) {
	assert(!_currentRoom);

	// In SSCI this loaded DLLs
	switch (getBaseRoomNumber(roomNo)) {
	case 1000:
		_currentRoom.reset(new S2Room1000(_kernel, _game));
		break;
	case 10000:
		_currentRoom.reset(new S2Room10000(_kernel, _game));
		break;
	case 6000:
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

void S2RoomManager::disposeRoom(const int roomNo) {
	if (_currentRoom) {
		_currentRoom->dispose(roomNo);
	}
}

void S2RoomManager::unloadRoom() {
	if (_currentRoom) {
		_currentRoom.reset();
	}
}

void S2RoomManager::activateRoom() {
	_roomIsActive = true;
	if (_panoramaIsVisible) {
		_game.getExtras().push_back(_panorama.get());
		_game.getUser().getOrphans().push_back(_panorama.get());
		_panorama->getPlane().setPriority(2, true);
		_kernel.graphicsManager._palette.loadPalette(_panorama->getResourceNo());
		_panorama->updatePanorama(true);
	}
	if (_pictureIsVisible) {
		_picture->setPriority(2, true);
		if (!_game.getPlanes().contains(*_picture)) {
			_game.getPlanes().add(*_picture);
			_picture->repaint();
		}
	}
}

void S2RoomManager::deactivateRoom() {
	_roomIsActive = false;
	if (_panorama && _panoramaIsVisible) {
		_game.getExtras().remove(_panorama.get());
		_game.getUser().getOrphans().remove(_panorama.get());
		_panorama->getPlane().setPriority(-1, true);
	}
	if (_picture && _pictureIsVisible) {
		_picture->setPriority(-1, true);
		_game.getPlanes().remove(*_picture);
	}
}

void S2RoomManager::unloadGlobalRoom() {
	if (!_currentGlobalRoomNo) {
		return;
	}
	_globalRoom->dispose(_currentGlobalRoomNo);
	_game.getSoundManager().fade(30004, 0, 15, 12, true);
	_game.getFlags().clear(kGameFlag44);
	_game.getInterface().putText(0);
	_game.getPlanes().remove(*_globalPlane);
	_globalPlane.reset();
	activateRoom();
	_game.getMovieManager().resumeRobot();
	_kernel.graphicsManager.kernelFrameOut(true);
	_currentGlobalRoomNo = 0;
	if (_lastNonGlobalRoomNo == _currentRoomNo) {
		_game.getSoundManager().createAmbient(_lastSoundRoomNo);
	} else {
		_game.getSoundManager().stopAllSounds();
		_game.getSoundManager().createAmbient(getCurrentAmbientRoomNo());
	}
}

int S2RoomManager::getBaseRoomNumber(const int roomNo) const {
	if (roomNo < 30000) {
		return roomNo / 1000 * 1000;
	} else {
		return (roomNo / 1000 - 20) * 1000;
	}
}

void S2RoomManager::loadGlobalRoom(const int roomNo, const bool fullscreen) {
	const Common::Rect fullscreenRect(_kernel.graphicsManager.getScriptWidth(),
									  _kernel.graphicsManager.getScriptHeight());

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
		_lastNonGlobalRoomNo = getCurrentRoomNo();
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
		// There was check here that appeared to be trying to avoid doing work
		// if the global plane was already fullscreen, but it was broken due to
		// an off-by-one error, so the check always failed in SSCI and the path
		// was always taken. The "broken" behaviour is needed to get a new
		// picture to display when switching between fullscreen global rooms, so
		// the check is omitted entirely
		if (fullscreen) {
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

void S2RoomManager::drawPan(const uint16 resourceNo) {
	_game.getInterface().show();

	if (_picture && _pictureIsVisible) {
		_game.getPlanes().remove(*_picture);
		_picture.reset();
		_pictureIsVisible = false;
	}

	if (!_panorama) {
		_panorama.reset(new GLPanorama({ 64, 0, 576, 384 }));
		_panorama->panX() = _savedPanX;
		if (_savedPanY) {
			_panorama->panY() = _savedPanY;
		}
		_savedPanX = _savedPanY = 0;
	}

	_kernel.graphicsManager._palette.loadPalette(resourceNo);
	_panorama->drawPic(resourceNo);
	// SSCI loaded the palette again here and set a flag which is already set by
	// drawPic
	if (_panoramaIsVisible) {
		_panorama->getPlane().repaint();
	} else {
		_game.getExtras().push_back(_panorama.get());
		_game.getUser().getOrphans().push_back(_panorama.get());
		_panorama->getPlane().setPriority(2, true);
		_panoramaIsVisible = true;
	}

	_currentPanoramaNo = resourceNo;
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
		_panorama->getPlane().setPriority(-1, true);
		_panoramaIsVisible = false;
	}

	if (!_picture) {
		const Common::Rect fullscreenRect(0, 0,
										  _kernel.graphicsManager.getScriptWidth(),
										  _kernel.graphicsManager.getScriptHeight());
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
			if ((exit->getCursorCel() != S2Cursor::kHighlightCel || _autoHighlight) &&
				exit->getIsEnabled() &&
				exit->checkIsOnMe(localMousePosition) &&
				exit->getCursorCel() != S2Cursor::kNormalCel) {
				hit = true;

				if (highlightedCel.celNo != exit->getCursorCel()) {
					highlightedCel.celNo = exit->getCursorCel();
					_game.getCursor().setHighlightedCelRes(highlightedCel);
				}

				break;
			}
		}

		if (!hit && _autoHighlight) {
			for (const auto &hotspot : _hotspots) {
				if (hotspot->getIsEnabled() && hotspot->checkIsOnMe(localMousePosition)) {
					hit = true;

					if (highlightedCel.celNo != S2Cursor::kHighlightCel) {
						highlightedCel.celNo = S2Cursor::kHighlightCel;
						_game.getCursor().setHighlightedCelRes(highlightedCel);
					}

					break;
				}
			}
			if (!hit) {
				for (const auto &cel : _cels) {
					if (cel->checkIsOnMe(localMousePosition)) {
						hit = true;

						if (highlightedCel.celNo != S2Cursor::kHighlightCel) {
							highlightedCel.celNo = S2Cursor::kHighlightCel;
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
