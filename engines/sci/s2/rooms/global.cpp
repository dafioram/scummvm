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
#include "common/translation.h"
#include "gui/saveload.h"
#include "sci/event.h"
#include "sci/s2/button.h"
#include "sci/s2/game.h"
#include "sci/s2/kernel.h"
#include "sci/s2/message_box.h"
#include "sci/s2/rooms/global.h"
#include "sci/s2/system/glcycler.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/types.h"
#include "sci/s2/savegame.h"

namespace Sci {

class S2MainMenuRoom : public S2SubRoom {
	auto &addButton(const int16 loopNo, const bool enable = true) {
		auto &button = emplaceButton(true, enable, 4000, loopNo, 0, absBottom, 202);
		button.setHighlightedFace(4000, loopNo, 2);
		button.setDepressedFace(4000, loopNo, 2);
		return button;
	};

public:
	using S2SubRoom::S2SubRoom;

	virtual void init(const int) override {
		// new game
		auto *button = &addButton(0);
		button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
			_game.getSoundManager().play(10913, false, 100);
			_game.getRoomManager().loadGlobalRoom(4010, true);
		});

		// old game
		button = &addButton(1, _game.hasSaveGames());
		button->setMouseUpHandler(&static_cast<S2GlobalRoom &>(_parent), &S2GlobalRoom::showOldGames);

		// web page
		button = &addButton(2);
		button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
			const char *text = "Unfortunately, Internet Archive does not have a "
				"saved copy of the Shivers 2 web site. If you do, please get in "
				"touch!";

			S2MessageBox message(text, S2MessageBox::Type::OK);
			message.createS2Dialog();
		});

		// credits
		button = &addButton(3);
		button->setHighlightedFace(4000, 3, 1);
		button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
			_game.getSoundManager().play(10913, false, 100);
			_game.getSoundManager().fade(30004, 0, 15, 12, true);
			const int soundNo = 30000 + _game.getRandomNumber(1, 3);
			static_cast<S2GlobalRoom &>(_parent)._creditsSoundNo = soundNo;
			_game.getSoundManager().play(soundNo, true, 0);
			_game.getSoundManager().fade(soundNo, Audio32::kMaxVolume, 15, 16);
			_game.getRoomManager().loadGlobalRoom(4400, true);
		});

		// quit
		button = &addButton(4);
		button->setHighlightedFace(4000, 4, 2);
		button->setMouseUpHandler(&static_cast<S2GlobalRoom &>(_parent), &S2GlobalRoom::quitGame);

		// sub-title
		emplaceCel(false, 4000, 5, 4, absBottom, 201).show();
	}
};

class S2NewGameRoom : public S2SubRoom {
public:
	using S2SubRoom::S2SubRoom;

	virtual void init(const int) override {
		auto *button = _startButton = &emplaceButton(true, false, 4010, 0, 0, GLPoint(0, 479), 202);
		button->setDisabledFace(4010, 0, 0);
		button->setHighlightedFace(4010, 0, 1);
		button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			startNewGame();
		});

		button = &emplaceButton(true, true, 4010, 1, 0, absBottom, 202);
		button->setHighlightedFace(4010, 1, 1);
		button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
			_game.getSoundManager().play(10913, false, 100);
			_game.getRoomManager().loadGlobalRoom(4000, true);
		});

		_newGameRect = { 2, 2, 510, 21 };
		_newGameBitmap.reset(new S2Bitmap(512, 22, 255, 255));
		_newGameView = &emplaceChild<GLScreenItem>(*_newGameBitmap, GLPoint(115, 135), 202);
		_newGameView->show();
	}

	virtual bool handleEvent(GLEvent &event) override {
		if (event.getType() == kSciEventKeyDown) {
			const uint16 key = event.getMessage();
			if (key == kSciKeyBackspace || (key >= ' ' && key <= 'z')) {
				if (key == kSciKeyBackspace) {
					_newGameName.deleteLastChar();
					_newGameBitmap->fill(_newGameRect, 255);
					_newGameView->forceUpdate();
				} else if (_newGameName.size() >= 20) {
					event.claim();
					return true;
				} else {
					_newGameName += key;
				}

				_newGameBitmap->drawText(_newGameName, _newGameRect, 202, 255, 255, 503);
				_newGameView->forceUpdate();
			} else if (key == kSciKeyEnter && _newGameName.size()) {
				// SSCI did not check the size of the game name before allowing
				// this to trigger
				startNewGame();
				event.claim();
				return true;
			}

			if (_newGameName.size()) {
				_startButton->enable();
			} else {
				_startButton->disable();
			}
		}
		event.claim();
		return true;
	}

private:
	void startNewGame() {
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

		auto &game = _game;
		_game.getRoomManager().unloadGlobalRoom();
		game.getRoomManager().newRoom(1010);
	}

	Common::String _newGameName;
	Common::Rect _newGameRect;
	Common::ScopedPtr<S2Bitmap> _newGameBitmap;
	GLScreenItem *_newGameView;
	S2Button *_startButton;
};

class S2LoadGameRoom : public S2SubRoom {
public:
	using S2SubRoom::S2SubRoom;

	virtual void init(const int) override {
		const int lastRoom = static_cast<S2GlobalRoom &>(_parent)._lastRoomBeforeRestore;

		if (!ConfMan.getBool("originalsaveload")) {
			GUI::SaveLoadChooser dialog(_("Load game:"), _("Load"), false);
			const int slotNo = dialog.runModalWithCurrentTarget();
			if (slotNo > -1) {
				_selectedSlot = slotNo;
				playSelectedSlot();
			} else if (lastRoom == 4000) {
				_game.getRoomManager().loadGlobalRoom(4000, true);
			} else if (lastRoom < 4200 || lastRoom >= 4300) {
				_game.getRoomManager().loadGlobalRoom(4100);
			} else {
				static_cast<S2GlobalRoom &>(_parent).returnToGame();
			}
			return;
		}

		S2Button *button;
		button = _playButton = &emplaceButton(true, false, 4020, 0, 0, absBottom, 202);
		button->setHighlightedFace(4020, 0, 1);
		button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			playSelectedSlot();
		});

		button = _deleteButton = &emplaceButton(true, false, 4020, 1, 0, absBottom, 202);
		button->setHighlightedFace(4020, 1, 1);
		button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			deleteSelectedSlot();
		});

		if (lastRoom == 4000) {
			button = &emplaceButton(true, true, 4020, 2, 0, absBottom, 202);
			button->setHighlightedFace(4020, 2, 1);
			button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
				_game.getSoundManager().play(10913, false, 100);
				_game.getRoomManager().loadGlobalRoom(4000, true);
			});
		} else if (lastRoom < 4200 || lastRoom >= 4300) {
			button = &emplaceButton(true, true, 4020, 3, 0, absBottom, 202);
			button->setHighlightedFace(4020, 3, 1);
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

			// Dimensions are reduced versus SSCI since we allow arbitrary game
			// name lengths instead of limiting to a certain number of characters,
			// so the names might overflow
			GfxFontFromResource font(&_kernel.resourceManager, 503);

			S2Bitmap &bitmap = _bitmaps.emplace_back(232, font.getHeight() + 1, 255, 255);
			GLScreenItem &screenItem = emplaceChild<GLScreenItem>(bitmap, position, 202);
			screenItem.show();
			bitmap.drawText(save.name, { 2, 2, 232, int16(font.getHeight() + 1) }, 202, 255, 255, 503, kTextAlignLeft, 255);
			screenItem.forceUpdate();

			auto &hotspot = emplaceHotspot(false,
										   hotspotPosition.x,
										   hotspotPosition.y,
										   hotspotPosition.x + 254,
										   hotspotPosition.y + 20);

			hotspot.setMouseUpHandler([=](GLEvent &event, GLTarget &target) {
				_game.getSoundManager().play(10913, false, 100);
				_playButton->enable();
				_deleteButton->enable();

				if (!_bullet) {
					_bullet = &emplaceCel(false, 4020, 4, 0, absTop, 201);
					_bullet->show();
				}

				const auto x = index < 12 ? 100 : 415;
				_bullet->setPosition(GLPoint(x, position.y + 18));
				_bullet->forceUpdate();
				_selectedSlot = slotNo;
			});

			if (++index == 24) {
				break;
			}

			position.y += 28;
			hotspotPosition.y += 28;
		}
	}

	virtual bool handleEvent(GLEvent &event) override {
		if (event.getType() == kSciEventKeyDown && _playButton->getIsEnabled()) {
			if (event.getMessage() == kSciKeyEnter) {
				playSelectedSlot();
			} else if (event.getMessage() == kSciKeyDelete) {
				deleteSelectedSlot();
			}
		}

		event.claim();
		return true;
	}

private:
	void playSelectedSlot() {
		_game.getUser().setIsHandsOn(false);
		_game.getCursor().goHandsOff();
		_game.getSoundManager().play(10902, false, 100);
		flushEvents();
		_game.getRoomManager().setLastSoundRoomNo(0);

		auto &game = _game;
		const auto slot = _selectedSlot;
		game.getRoomManager().unloadGlobalRoom();
		if (!game.load(slot)) {
			error("Attempt to load slot %d failed", slot);
		}
		game.getCursor().goHandsOn();
		game.getUser().setIsHandsOn(true);
	}

	void deleteSelectedSlot() {
		_game.getSoundManager().play(10913, false, 100);
		_game.deleteGame(_selectedSlot);
		_parent.reload(4020);
	}

	Common::Array<S2Bitmap> _bitmaps;
	S2Button *_playButton = nullptr;
	S2Button *_deleteButton = nullptr;
	GLCel *_bullet = nullptr;
	int _selectedSlot = -1;
};

class S2OptionsRoom : public S2SubRoom {
	auto &addButton(const int16 loopNo, const bool enable = true) {
		auto &button = emplaceButton(true, enable, 4100, loopNo, 0, gameBottom, 202);
		button.setHighlightedFace(4100, loopNo, 2);
		button.setDepressedFace(4100, loopNo, 2);
		return button;
	};

public:
	using S2SubRoom::S2SubRoom;

	virtual void init(const int) override {
		auto *button = &addButton(0);
		button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			static_cast<S2GlobalRoom &>(_parent).returnToGame();
		});

		button = &addButton(1, _game.hasSaveGames());
		button->setMouseUpHandler(&static_cast<S2GlobalRoom &>(_parent), &S2GlobalRoom::showOldGames);

		button = &addButton(2);
		button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			_game.getSoundManager().play(10913, false, 100);
			// SSCI used a script to delay this call for one second for some
			// reason
			_game.getRoomManager().loadGlobalRoom(4120);
		});

		button = &addButton(3);
		button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			_game.getSoundManager().play(10913, false, 100);

			// SSCI used an ugly-looking kEditText control to do this, and this
			// was the only place in the game that used it, so we will just use
			// the normal ScummVM picker instead
			if (_game.save(-1, true)) {
				auto &game = _game;
				_game.getRoomManager().unloadGlobalRoom();
				game.getInterface().resetButtons();
			}
		});

		button = &addButton(4);
		button->setMouseUpHandler(&static_cast<S2GlobalRoom &>(_parent), &S2GlobalRoom::quitGame);

		_scoreBitmap.reset(new S2Bitmap(252, 22, 255, 255));
		auto &textBox = emplaceChild<GLScreenItem>(*_scoreBitmap, GLPoint(139, 357), 202);
		using S = Common::String;
		const S score(S::format("%d", _game.getScoringManager().getCurrentScore()));
		_scoreBitmap->drawText(score, { 2, 2, 251, 21 }, 202, 255, 255, 503, kTextAlignLeft, 255);
		textBox.forceUpdate();
	}

private:
	Common::ScopedPtr<S2Bitmap> _scoreBitmap;
};

class S2ConfigurationRoom : public S2SubRoom {
public:
	using S2SubRoom::S2SubRoom;

	virtual void init(const int) override {
		auto *button = &emplaceCel(false, 4120, 0, _game.getInterface().getIsCaptioningOn(), gameBottom, 201);
		button->setSelectHandler([&](GLEvent &event, GLTarget &cel) {
			if (event.getType() == kSciEventMousePress) {
				_game.getSoundManager().play(10913, false, 100);
				const bool newState = !_game.getInterface().getIsCaptioningOn();
				_game.getInterface().setIsCaptioningOn(newState);
				dynamic_cast<GLCel &>(cel).setCel(newState, true);
			}
		});
		button->forceUpdate();

		button = &emplaceCel(false, 4120, 1, _game.getRoomManager().getAutoHighlight(), gameBottom, 201);
		button->setSelectHandler([&](GLEvent &event, GLTarget &cel) {
			if (event.getType() == kSciEventMousePress) {
				_game.getSoundManager().play(10913, false, 100);
				const bool newState = !_game.getRoomManager().getAutoHighlight();
				_game.getRoomManager().toggleAutoHighlight();
				dynamic_cast<GLCel &>(cel).setCel(newState, true);
			}
		});
		button->forceUpdate();

		button = &emplaceCel(false, 4120, 2, !_game.getMovieManager().getUseHalfScreen(), gameBottom, 201);
		button->setSelectHandler([&](GLEvent &event, GLTarget &cel) {
			if (event.getType() == kSciEventMousePress) {
				_game.getSoundManager().play(10913, false, 100);
				const bool newState = !_game.getMovieManager().getUseHalfScreen();
				_game.getMovieManager().toggleUseHalfScreen();
				dynamic_cast<GLCel &>(cel).setCel(!newState, true);
			}
		});
		button->forceUpdate();

		auto *slider = &emplaceCel(false, 4120, 3, 0, GLPoint(64 + 366 + 124 * ConfMan.getInt("sfx_volume") / (Audio::Mixer::kMaxMixerVolume + 1), 192), 201);
		slider->setSelectHandler([&](GLEvent &event, GLTarget &cel) {});
		slider->forceUpdate();

		slider = &emplaceCel(false, 4120, 3, 0, GLPoint(64 + 366 + 31 * _game.getGamma(), 223), 201);
		slider->setSelectHandler([&](GLEvent &, GLTarget &cel) {

		});
		slider->forceUpdate();

		slider = &emplaceCel(false, 4120, 3, 0, GLPoint(64 + 366 + 31 * _game.getGamma(), 286), 201);
		slider->setSelectHandler([&](GLEvent &, GLTarget &cel) {

		});
		slider->forceUpdate();

		auto &sign = emplaceCel(false, 4120, 4, 2, gameBottom, 201);
		sign.hide();
		_neonSign.reset(new GLScript([&](GLScript &script, int state) {
			switch (state) {
			case 0:
				sign.show();
				script.setSeconds(_game.getRandomNumber(1, 2));
				break;
			case 1:
				sign.hide();
				script.setSeconds(_game.getRandomNumber(1, 2));
				script.setState(-1);
				break;
			}
		}));

		_solveIt = false;
		auto &solver = emplaceCel(false, 4120, 5, 0, gameBottom, 201);
		solver.setSelectHandler(this, &S2ConfigurationRoom::solvePuzzle);
		_solverCycler.reset(new GLCycler());
		_solverCycler->add(solver, true);
	}

	virtual bool handleEvent(GLEvent &event) override {
		warning("TODO: Handle global 4120 event (destroy slider script on release)");
		event.claim();
		return true;
	}

private:
	void solvePuzzle(GLEvent &, GLTarget &) {
		warning("TODO: solve puzzle");
	}

	Common::ScopedPtr<GLScript> _neonSign;

	bool _solveIt;
	Common::ScopedPtr<GLCycler> _solverCycler;
};

class S2CreditsRoom : public S2SubRoom {
public:
	using S2SubRoom::S2SubRoom;

	virtual void init(const int roomNo) override {
		auto &hotspot = emplaceHotspot(false, 0, 0, 639, 479);
		hotspot.setMouseUpHandler([=](GLEvent &, GLTarget &) {
			if (roomNo == 4407) {
				finish();
			} else {
				next();
			}
		});
		setScript(this, &S2CreditsRoom::creditsScript);

		switch (roomNo) {
		case 4401:
		case 4402:
		case 4403:
		case 4406:
			GLPoint position;
			if (roomNo == 4401) {
				position = { 96, 135 };
			} else if (roomNo == 4402) {
				position = { 316, 360 };
			} else if (roomNo == 4403) {
				position = { 319, 380 };
			} else {
				position = { 322, 320 };
			}

			auto &cel = emplaceCel(false, roomNo, 0, 0, position);
			cel.setCel(_game.getRandomNumber(0, 40));
			cel.setCycleSpeed(6);
			cel.show();
			_cycler.reset(new GLPingPongCycler());
			_cycler->add(cel);
			break;
		}
	}

	bool handleEvent(GLEvent &event) override {
		if (event.getType() == kSciEventKeyDown) {
			const auto key = event.getMessage();
			if (key == kSciKeyEsc) {
				finish();
			} else if (key == kSciKeyEnter || key == ' ') {
				next();
			}
		}

		event.claim();
		return true;
	}

private:
	void finish() {
		const int soundNo = static_cast<S2GlobalRoom &>(_parent)._creditsSoundNo;
		_game.getSoundManager().fade(soundNo, 0, 15, 16, true);
		_game.getSoundManager().play(30004, true, 0);
		_game.getSoundManager().fade(30004, 80, 15, 12);
		_game.getRoomManager().loadGlobalRoom(4000, true);
	}

	void next() {
		int nextRoom = _game.getRoomManager().getCurrentGlobalRoomNo() + 1;
		if (nextRoom > 4407) {
			nextRoom = 4400;
		}
		_game.getRoomManager().loadGlobalRoom(nextRoom, true);
	}

	void creditsScript(GLScript &script, const int state) {
		switch (state) {
		case 0:
			script.setSeconds(10);
			break;
		case 1: {
			next();
			break;
		}
		}
	}

	Common::ScopedPtr<GLPingPongCycler> _cycler;
};

void S2GlobalRoom::init(const int roomNo) {
	_game.getInterface().putText(0);
	flushEvents();

	switch (roomNo) {
	case 4000:
		setSubRoom<S2MainMenuRoom>();
		break;
	case 4010:
		setSubRoom<S2NewGameRoom>();
		break;
	case 4020:
		setSubRoom<S2LoadGameRoom>();
		break;
	case 4100:
		setSubRoom<S2OptionsRoom>();
		break;
	case 4120:
		setSubRoom<S2ConfigurationRoom>();
		break;
	case 4400:
	case 4401:
	case 4402:
	case 4403:
	case 4404:
	case 4405:
	case 4406:
	case 4407:
		setSubRoom<S2CreditsRoom>();
		break;
	default:
		error("Unknown global room %d", roomNo);
	}

	if (_activeSubRoom) {
		_activeSubRoom->init(roomNo);
	}
}

bool S2GlobalRoom::handleEvent(GLEvent &event) {
	switch (_game.getRoomManager().getCurrentGlobalRoomNo()) {
	case 4120:
		break;
	case 4300:
		warning("TODO: Handle global 4300 event");
		break;
	}
	return _activeSubRoom->handleEvent(event);
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

} // End of namespace Sci
