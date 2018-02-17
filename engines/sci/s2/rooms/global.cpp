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
#include "sci/s2/message_box.h"
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
	case 4010:
		initNewGame();
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

	_buttons.clear();
	_cels.clear();
	_bitmaps.clear();
	_screenItems.clear();
	_rects.clear();
	_newGameName.clear();
}

bool S2GlobalRoom::handleEvent(GLEvent &event) {
	switch (_game.getRoomManager().getCurrentGlobalRoomNo()) {
	case 4010:
		if (event.getType() == kSciEventKeyDown) {
			const uint16 key = event.getMessage();
			if (key == kSciKeyBackspace || (key >= ' ' && key <= 'z')) {
				if (key == kSciKeyBackspace) {
					_newGameName.deleteLastChar();
					_bitmaps[0]->fill(_rects[0], 255);
					_screenItems[0]->forceUpdate();
				} else if (_newGameName.size() >= 20) {
					return true;
				} else {
					_newGameName += key;
				}

				_bitmaps[0]->drawText(_newGameName, _rects[0], 202, 255, 255, 503);
				_screenItems[0]->forceUpdate();
			} else if (key == kSciKeyEnter && _newGameName.size()) {
				// SSCI did not check the size
				startNewGame();
			}
			if (_newGameName.size()) {
				_buttons[0]->enable();
			} else {
				_buttons[0]->disable();
			}
		}
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
	auto *button = &addButton(4000, 0, 0, GLPoint(0, 479), 202);
	button->setHighlightedFace(4000, 0, 2);
	button->setDepressedFace(4000, 0, 2);
	button->enable();
	button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
		_game.getSoundManager().play(10913, false, 100);
		// TODO: There is a check here to make sure there are no more than 24
		// saves before going to the new room; do we need this?
		_game.getRoomManager().loadGlobalRoom(4010, true);
	});

	button = &addButton(4000, 1, 0, GLPoint(0, 479), 202);
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

	button = &addButton(4000, 2, 0, GLPoint(0, 479), 202);
	button->setHighlightedFace(4000, 2, 2);
	button->setDepressedFace(4000, 2, 2);
	button->enable();
	button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
		const char *text = "Unfortunately, Internet Archive does not have a "
			"saved copy of the Shivers 2 web site. If you do, please get in "
			"touch!";

		S2MessageBox message(text, S2MessageBox::Type::OK);
		message.createS2Dialog();
	});

	button = &addButton(4000, 3, 0, GLPoint(0, 479), 202);
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

	button = &addButton(4000, 4, 0, GLPoint(0, 479), 202);
	button->setHighlightedFace(4000, 4, 2);
	button->enable();
	button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
		// SSCI did not wait until the sample finished playing before quitting
		_kernel.timeManager.sleepTicks(_game.getSoundManager().play(10903, false, 100));
		_game.getRoomManager().unloadGlobalRoom();
		_game.quit();
	});

	addCel(4000, 5, 4, GLPoint(0, 479), 201).show();
}

void S2GlobalRoom::initNewGame() {
	auto *button = &addButton(4010, 0, 0, GLPoint(0, 479), 202);
	button->setDisabledFace(4010, 0, 0);
	button->setHighlightedFace(4010, 0, 1);
	button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
		startNewGame();
	});

	button = &addButton(4010, 1, 0, GLPoint(0, 479), 202);
	button->setHighlightedFace(4010, 1, 1);
	button->enable();
	button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
		_game.getSoundManager().play(10913, false, 100);
		_game.getRoomManager().loadGlobalRoom(4000, true);
	});

	_rects.emplace_back(2, 2, 510, 21);
	auto &bitmap = _bitmaps.emplace_back(new S2Bitmap(512, 22, 255, 255));
	_screenItems.emplace_back(new GLScreenItem(*_plane, *bitmap, GLPoint(115, 135), 202))->show();
}

void S2GlobalRoom::startNewGame() {
	warning("New game time!");
}

} // End of namespace Sci
