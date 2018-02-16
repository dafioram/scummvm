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
#include "sci/event.h"
#include "sci/s2/button.h"
#include "sci/s2/game.h"
#include "sci/s2/kernel.h"
#include "sci/s2/rooms/global.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/types.h"

namespace Sci {

void S2GlobalRoom::init(const int roomNo) {
	_plane = _game.getRoomManager().getGlobalPlane();
	_game.getInterface().putText(0);
	_kernel.eventManager.flushEvents();

	switch (roomNo) {
	case 4000:
		initMainMenu();
		break;
	default:
		error("Unknown global room %d", roomNo);
	}
}

void S2GlobalRoom::dispose(const int roomNo) {
	switch (_game.getRoomManager().getCurrentGlobalRoomNo()) {
	case 4120:
		warning("TODO: Handle global 4120 dispose");
		break;
	case 4300:
		warning("TODO: Handle global 4300 dispose");
		break;
	case 4302:
		warning("TODO: Handle global 4302 dispose");
		break;
	case 5011:
		warning("TODO: Handle global 5011 dispose");
		break;
	case 11140:
		warning("TODO: Handle global 11140 dispose");
		break;
	}
}

bool S2GlobalRoom::handleEvent(GLEvent &event) {
	switch (_game.getRoomManager().getCurrentGlobalRoomNo()) {
	case 4010:
		warning("TODO: Handle global 4010 event");
		break;
	case 4020:
		warning("TODO: Handle global 4020 event");
		break;
	case 4120:
		warning("TODO: Handle global 4120 event");
		break;
	case 4300:
		warning("TODO: Handle global 4300 event");
		break;
	case 4400:
	case 4401:
	case 4402:
	case 4403:
	case 4404:
	case 4405:
	case 4406:
	case 4407:
		warning("TODO: Handle global 4400 event");
		break;
	}
	event.claim();
	return true;
}

void S2GlobalRoom::initMainMenu() {
	auto *button = &addButton(*_plane, 4000, 0, 0, GLPoint(0, 479), 202);
	button->setHighlightedFace(4000, 0, 2);
	button->setDepressedFace(4000, 0, 2);
	button->enable();
	button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
		_game.getSoundManager().play(10913, false, 100);
		// TODO: There is a check here to make sure there are no more than 24
		// saves before going to the new room; do we need this?
		_game.getRoomManager().loadGlobalRoom(4010, true);
	});

	button = &addButton(*_plane, 4000, 1, 0, GLPoint(0, 479), 202);
	button->setHighlightedFace(4000, 1, 2);
	button->setDepressedFace(4000, 1, 2);
	if (_game.hasSaveGames()) {
		button->enable();
	}
	button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
		_game.getSoundManager().play(10913, false, 100);
		_lastRoomBeforeRestore = _game.getRoomManager().getCurrentGlobalRoomNo();
		_game.getRoomManager().loadGlobalRoom(4020, true);
	});

	// Unfortunately, the Internet Archive does not have a saved copy of the
	// web site for this game, so this button cannot be enabled unless someone
	// happens to have downloaded it at the time.
	button = &addButton(*_plane, 4000, 2, 0, GLPoint(0, 479), 202);
	button->setHighlightedFace(4000, 2, 2);
	button->setDepressedFace(4000, 2, 2);

	button = &addButton(*_plane, 4000, 3, 0, GLPoint(0, 479), 202);
	button->setHighlightedFace(4000, 3, 1);
	button->enable();
	button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
		_game.getSoundManager().play(10913, false, 100);
		_game.getSoundManager().fade(30004, 0, 15, 12, true);
		const uint16 songNo = 30000 + _game.getRandomNumber(1, 3);
		_game.getSoundManager().play(songNo, true, 0);
		_game.getSoundManager().fade(songNo, Audio32::kMaxVolume, 15, 16);
		_game.getRoomManager().loadGlobalRoom(4400, true);
	});

	button = &addButton(*_plane, 4000, 4, 0, GLPoint(0, 479), 202);
	button->setHighlightedFace(4000, 4, 2);
	button->enable();
	button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
		// SSCI did not wait until the sample finished playing before quitting
		_kernel.timeManager.sleepTicks(_game.getSoundManager().play(10903, false, 100));
		_game.getRoomManager().unloadGlobalRoom();
		_game.quit();
	});

	addCel(*_plane, 4000, 5, 4, GLPoint(0, 479), 201).show();
}

} // End of namespace Sci
