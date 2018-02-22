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
#include "gui/saveload.h"
#include "sci/event.h"
#include "sci/s2/button.h"
#include "sci/s2/game.h"
#include "sci/s2/kernel.h"
#include "sci/s2/message_box.h"
#include "sci/s2/rooms/global.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/types.h"
#include "sci/s2/savegame.h"

namespace Sci {

GLPicturePlane &S2GlobalRoom::getPlane() const {
	return *_game.getRoomManager().getGlobalPlane();
}

void S2GlobalRoom::init(const int roomNo) {
	_game.getInterface().putText(0);
	_kernel.eventManager.flushEvents();

	switch (roomNo) {
	case 4000:
		initMainMenu();
		break;
	case 4010:
		initNewGame();
		break;
	case 4020:
		initLoadGame();
		break;
	case 4100:
		initOptions();
		break;
	default:
		error("Unknown global room %d", roomNo);
	}
}

void S2GlobalRoom::dispose(const int roomNo) {
	switch (roomNo) {
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

	S2Room::dispose(roomNo);
	_buttons.clear();
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
				// SSCI did not check the size of the game name before allowing
				// this to trigger
				startNewGame();
				break;
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
	auto makeButton = [&](const int16 loopNo) -> S2Button * {
		auto &button = addButton(4000, loopNo, 0, GLPoint(0, 479), 202);
		button.setHighlightedFace(4000, loopNo, 2);
		button.setDepressedFace(4000, loopNo, 2);
		return &button;
	};

	auto *button = makeButton(0);
	button->enable();
	button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
		_game.getSoundManager().play(10913, false, 100);
		// TODO: There is a check here to make sure there are no more than 24
		// saves before going to the new room; do we need this?
		_game.getRoomManager().loadGlobalRoom(4010, true);
	});

	button = makeButton(1);
	if (_game.hasSaveGames()) {
		button->enable();
	}
	button->setMouseUpHandler(this, &S2GlobalRoom::showOldGames);

	button = makeButton(2);
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
	button->setMouseUpHandler(this, &S2GlobalRoom::quitGame);

	addCel(4000, 5, 4, GLPoint(0, 479), 201).show();
}

void S2GlobalRoom::initNewGame() {
	auto *button = &addButton(4010, 0, 0, GLPoint(0, 479), 202);
	button->setDisabledFace(4010, 0, 0);
	button->setHighlightedFace(4010, 0, 1);
	button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
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
	_screenItems.emplace_back(new GLScreenItem(getPlane(), *bitmap, GLPoint(115, 135), 202))->show();
}

void S2GlobalRoom::startNewGame() {
	for (const auto &metadata : _game.getSaveGameList()) {
		if (_newGameName == metadata.name) {
			S2MessageBox message("That name is already registered. "
								 "Please type in a unique game name.",
								 S2MessageBox::Type::OK);
			message.createS2Dialog();
			return;
		}
	}

	_game.setSaveGameName(_newGameName);
	_game.getRoomManager().unloadGlobalRoom();
	_game.getRoomManager().newRoom(1010);
}

void S2GlobalRoom::initOptions() {
	auto makeButton = [&](int16 loop) -> S2Button * {
		auto &button = addButton(4100, loop, 0, GLPoint(64, 383), 202);
		button.setHighlightedFace(4100, loop, 2);
		button.setDepressedFace(4100, loop, 2);
		return &button;
	};

	auto *button = makeButton(0);
	button->enable();
	button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
		returnToGame();
	});

	button = makeButton(1);
	if (_game.hasSaveGames()) {
		button->enable();
	}
	button->setMouseUpHandler(this, &S2GlobalRoom::showOldGames);

	button = makeButton(2);
	button->enable();
	button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
		_game.getSoundManager().play(10913, false, 100);
		// SSCI used a script to delay this call for one second for some reason
		_game.getRoomManager().loadGlobalRoom(4120);
	});

	button = makeButton(3);
	button->enable();
	button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
		_game.getSoundManager().play(10913, false, 100);

		// SSCI used an ugly-looking kEditText control to do this, and this was
		// the only place in the game that used it, so we will just use the
		// normal ScummVM picker instead
		if (_game.save(-1, true)) {
			_game.getRoomManager().unloadGlobalRoom();
			_game.getInterface().resetButtons();
		}
	});

	button = makeButton(4);
	button->enable();
	button->setMouseUpHandler(this, &S2GlobalRoom::quitGame);

	auto &bitmap = _bitmaps.emplace_back(new S2Bitmap(252, 22, 255, 255));
	auto &textBox = _screenItems.emplace_back(new GLScreenItem(getPlane(), *bitmap, GLPoint(139, 357), 202));
	const Common::String score(Common::String::format("%d", _game.getScoringManager().getCurrentScore()));
	bitmap->drawText(score, Common::Rect(2, 2, 251, 21), 202, 255, 255, 503, kTextAlignLeft, 255);
	textBox->forceUpdate();
}

void S2GlobalRoom::returnToGame() {
	_game.getSoundManager().play(10902, false, 100);
	_game.getRoomManager().unloadGlobalRoom();
	_game.getInterface().resetButtons();
}

void S2GlobalRoom::showOldGames(GLEvent &, GLTarget &) {
	_game.getSoundManager().play(10913, false, 100);
	_lastRoomBeforeRestore = _game.getRoomManager().getCurrentGlobalRoomNo();
	_game.getRoomManager().loadGlobalRoom(4020, true);
}

void S2GlobalRoom::quitGame(GLEvent &, GLTarget &) {
	// SSCI did not wait until the sample finished playing before quitting
	_kernel.timeManager.sleepTicks(_game.getSoundManager().play(10903, false, 100));
	_game.getRoomManager().unloadGlobalRoom();
	_game.quit();
}

void S2GlobalRoom::initLoadGame() {
	_selectedSlot = -1;

	auto *button = &addButton(4020, 0, 0, GLPoint(0, 479), 202);
	button->setHighlightedFace(4020, 0, 1);
	button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
		_game.getUser().setIsHandsOn(false);
		_game.getCursor().goHandsOff();
		_game.getSoundManager().play(10902, false, 100);
		_game.getRoomManager().setLastSoundRoomNo(0);
		_game.getRoomManager().unloadGlobalRoom();
		flushEvents();
		_game.load(_selectedSlot);
		_game.getCursor().goHandsOn();
		_game.getUser().setIsHandsOn(true);
	});

	button = &addButton(4020, 1, 0, GLPoint(0, 479), 202);
	button->setHighlightedFace(4020, 1, 1);
	button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
		_game.getSoundManager().play(10913, false, 100);
		_game.deleteGame(_selectedSlot);
		dispose(4020);
		init(4020);
	});

	if (_lastRoomBeforeRestore == 4000) {
		button = &addButton(4020, 2, 0, GLPoint(0, 479), 202);
		button->setHighlightedFace(4020, 2, 1);
		button->enable();
		button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			_game.getSoundManager().play(10913, false, 100);
			_game.getRoomManager().loadGlobalRoom(4000, true);
		});
	} else if (_lastRoomBeforeRestore < 4200 || _lastRoomBeforeRestore >= 4300) {
		button = &addButton(4020, 3, 0, GLPoint(0, 479), 202);
		button->setHighlightedFace(4020, 3, 1);
		button->enable();
		button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			_game.getSoundManager().play(10913, false, 100);
			_game.getRoomManager().loadGlobalRoom(4100);
		});
	}

	auto index = 0;
	auto position(GLPoint(115, 135));
	auto hotspotPosition(GLPoint(98, 134));
	for (const auto &save : _game.getSaveGameList()) {
		const int slotNo = save.slotNo;

		if (index == 12) {
			position.x = 430;
			position.y = 135;
			hotspotPosition.x = 408;
			hotspotPosition.y = 135;
		}

		S2Bitmap &bitmap = *_bitmaps.emplace_back(new S2Bitmap(512, 22, 255, 255));
		GLScreenItem &screenItem = *_screenItems.emplace_back(new GLScreenItem(getPlane(), bitmap, position, 202));
		screenItem.show();
		bitmap.drawText(save.name, Common::Rect(2, 2, 510, 21), 202, 255, 255, 503, kTextAlignLeft, 255);
		screenItem.forceUpdate();

		auto &hotspot = *_hotspots.emplace_back(new S2Hotspot(getPlane(),
															  hotspotPosition.x,
															  hotspotPosition.y,
															  hotspotPosition.x + 254,
															  hotspotPosition.y + 20));

		hotspot.setMouseUpHandler([=](GLEvent &event, GLTarget &target) {
			_game.getSoundManager().play(10913, false, 100);
			_buttons[0]->enable();
			_buttons[1]->enable();

			if (!_cels.size() || !_cels[0]) {
				resetCel(0, 4020, 4, 0, GLPoint(0, 0), 201).show();
			}

			if (index < 12) {
				_cels[0]->setPosition(GLPoint(100, 152 + index * 28));
			} else {
				_cels[0]->setPosition(GLPoint(415, 152 + index * 28));
			}

			_cels[0]->forceUpdate();
			_selectedSlot = slotNo;
		});

		if (++index == 24) {
			break;
		}

		position.y += 28;
		hotspotPosition.y += 28;
	}
}

} // End of namespace Sci
