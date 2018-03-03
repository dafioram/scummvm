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

#define global(name) static_cast<S2GlobalRoom *>(&_parent), &S2GlobalRoom::name

class S2MainMenuRoom : public S2GlobalSubRoom {
	auto &addButton(const int16 loopNo, const bool enable = true) {
		auto &button = emplaceButton(true, enable, 4000, loopNo, 0, absBottom, 202);
		button.setHighlightedFace(4000, loopNo, 2);
		button.setDepressedFace(4000, loopNo, 2);
		return button;
	};

public:
	using S2GlobalSubRoom::S2GlobalSubRoom;

	virtual void init(const int) override {
		// new game
		auto *button = &addButton(0);
		button->setMouseUpHandler([&](GLEvent &event, GLTarget &target) {
			_game.getSoundManager().play(10913, false, 100);
			_game.getRoomManager().loadGlobalRoom(4010, true);
		});

		// old game
		button = &addButton(1, _game.hasSaveGames());
		button->setMouseUpHandler(global(showOldGames));

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
		button->setMouseUpHandler(global(quitGame));

		// sub-title
		emplaceCel(false, 4000, 5, 4, absBottom, 201).show();
	}
};

class S2NewGameRoom : public S2GlobalSubRoom {
public:
	using S2GlobalSubRoom::S2GlobalSubRoom;

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

class S2LoadGameRoom : public S2GlobalSubRoom {
public:
	using S2GlobalSubRoom::S2GlobalSubRoom;

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

class S2OptionsRoom : public S2GlobalSubRoom {
	auto &addButton(const int16 loopNo, const bool enable = true) {
		auto &button = emplaceButton(true, enable, 4100, loopNo, 0, roomBottom, 202);
		button.setHighlightedFace(4100, loopNo, 2);
		button.setDepressedFace(4100, loopNo, 2);
		return button;
	};

public:
	using S2GlobalSubRoom::S2GlobalSubRoom;

	virtual void init(const int) override {
		auto *button = &addButton(0);
		button->setMouseUpHandler([&](GLEvent &, GLTarget &) {
			static_cast<S2GlobalRoom &>(_parent).returnToGame();
		});

		button = &addButton(1, _game.hasSaveGames());
		button->setMouseUpHandler(global(showOldGames));

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
		button->setMouseUpHandler(global(quitGame));

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

#define self(name) this, &S2FlashbackRoom::name

class S2FlashbackRoom : public S2GlobalSubRoom {
public:
	using S2GlobalSubRoom::S2GlobalSubRoom;

	virtual void init(const int roomNo) override {
		switch (roomNo) {
		case 2:
			playMovie(true);
			break;
		case 1920:
			emplaceCel(false, 1920, 0, 1, roomBottom).show();
			break;
		case 4110:
		case 4111: {
			_game.getFlags().set(kGameFlag44);
			auto &button = emplaceButton(true, true, 4110, 1, 0, roomBottom, 202);
			button.setHighlightedFace(4110, 1, 1);
			button.setMouseUpHandler([&](GLEvent &, GLTarget &) {
				static_cast<S2GlobalRoom &>(_parent).returnToGame();
			});

			if (roomNo == 4110) {
				initFlashbackList();
			} else {
				initMovieList();
			}
			break;
		}
		case 5010:
		case 5120:
			_game.getSoundManager().stop(30004);
			warning("TODO: caption %d", roomNo);
			_game.getMovieManager().play(roomNo, nullptr, roomTop, false, true);
			_game.getSoundManager().play(30004, true, 0);
			_game.getSoundManager().fade(30004, 80, 15, 12);
			_game.getRoomManager().loadGlobalRoom(4110);
			break;
		case 5011:
			_game.getSoundManager().stop(30004);
			// SSCI stopped the same sound twice here
			_game.getSoundManager().play(59016);
			_game.getInterface().putText(59016);
			break;
		case 6274:
		case 10121:
		case 10128:
		case 11352:
		case 13211:
		case 14525:
		case 15021:
		case 16113:
		case 16931:
			break;
		case 6353:
			emplaceCel(false, 6353, 1, 1, roomBottom).show();
			break;
		case 11140:
			emplaceCel(false, 11140, 0, 10, roomBottom).show();
			_game.getSoundManager().play(41150);
			_game.getInterface().putText(41150);
			break;
		case 15341:
			playMovie(false);
			break;
		case 18131:
			emplaceCel(false, 18131, 0, 1, roomBottom).show();
			break;
		case 18240:
			emplaceCel(false, 18240, 1, 7, GLPoint(316, 135)).show();
			break;
		case 18423:
			break;
		case 20412: {
			getParent()._flashbackPageNo = 20412;
			emplaceCel(false, 20412, 0, 0, roomBottom).show();
			emplaceHotspot(false, 334, 34, 549, 358).setMouseUpHandler(self(nextPage));
			break;
		}
		case 20413: {
			getParent()._flashbackPageNo = 20413;
			emplaceCel(false, 20412, 0, 1, roomBottom).show();
			emplaceHotspot(false, 91, 34, 318, 358).setMouseUpHandler(self(previousPage));
			break;
		}
		case 21321:
			emplaceCel(false, 21321, 25, 0, roomBottom).show();
			break;
		case 21354:
			getParent()._flashbackPageNo = 21354;
			emplaceHotspot(false, 184, 37, 497, 362).setMouseUpHandler(self(nextPage));
			break;
		case 21355:
			getParent()._flashbackPageNo = 21355;
			emplaceHotspot(false, 184, 37, 497, 362).setMouseUpHandler(self(previousPage));
			break;
		case 22511:
			emplaceCel(false, 22511, 0, 3, roomBottom).show();
			break;
		case 24321:
		case 24322:
		case 24323:
		case 24324:
		case 24325:
		case 24326:
		case 24327:
		case 24328:
		case 24329:
		case 24330:
		case 24331:
		case 24332:
		case 24333:
		case 24334:
		case 24335:
			getParent()._flashbackPageNo = roomNo;
			if (roomNo > 24321) {
				emplaceHotspot(false, 64, 18, 315, 356).setMouseUpHandler(self(previousPage));
			}
			if (roomNo < 24335) {
				emplaceHotspot(false, 332, 18, 575, 356).setMouseUpHandler(self(nextPage));
			}
			break;
		}
	}

	virtual void dispose(const int roomNo) override {
		switch (roomNo) {
		case 5011:
			_game.getInterface().putText(0);
			_game.getSoundManager().stop();
			break;
		case 11140:
			_game.getSoundManager().stop(41150);
			break;
		}
	}

private:
	S2GlobalRoom &getParent() const { return static_cast<S2GlobalRoom &>(_parent); }

	void initFlashbackList() {
		auto &cel = emplaceCel(false, 4110, 0, 0, roomBottom, 202);
		cel.setSelectHandler([&](GLEvent &event, GLTarget &) {
			if (event.getType() == kSciEventMousePress) {
				_game.getSoundManager().play(10908, false, 100);
				_game.getRoomManager().loadGlobalRoom(4111);
			}
		});
		cel.show();
		cel.forceUpdate();

		auto *button = &emplaceButton(true, true, 4110, 2, 0, GLPoint(77, 40), 202);
		// SSCI set depressed face to its already-defaulted value
		button->setMouseUpHandler(self(showFlashback));

		// SSCI counted indexes and multiplied on each step instead of
		// maintaining a single point object
		GLPoint position(77, 70);
		for (int flag = kGameFlag45, loopNo = 3; flag <= kGameFlag70; ++flag, ++loopNo) {
			if (!_game.getFlags().get(GameFlag(flag))) {
				continue;
			}
			button = &emplaceButton(true, true, 4110, loopNo, 0, position, 202);
			// SSCI set depressed face to its already-defaulted value
			button->setMouseUpHandler(self(showFlashback));

			if (position.y == 250) {
				position.y = 40;
				if (position.x == 77) {
					position.x = 241;
				} else if (position.x == 241) {
					position.x = 410;
				}
			} else {
				position.y += 30;
			}
		}
	}

	void initMovieList() {
		_game.getFlags().set(kGameFlag44);

		auto &backButton = emplaceCel(false, 4110, 0, 1, roomBottom, 202);
		backButton.setSelectHandler([&](GLEvent &event, GLTarget &) {
			if (event.getType() == kSciEventMousePress) {
				_game.getSoundManager().play(10908, false, 100);
				_game.getRoomManager().loadGlobalRoom(4110);
			}
		});
		backButton.show();
		backButton.forceUpdate();

		GLPoint position(159, 50);
		for (int flag = kGameFlag71, loopNo = 29; flag <= kGameFlag84; ++flag, ++loopNo) {
			if (!_game.getFlags().get(GameFlag(flag))) {
				continue;
			}

			auto &button = emplaceButton(true, true, 4110, loopNo, 0, position, 202);
			button.setMouseUpHandler(self(showMovie));

			if (position.y == 290) {
				position.x = 334;
				position.y = 50;
			} else {
				position.y += 40;
			}
		}
	}

	void showFlashback(GLEvent &, GLTarget &target) {
		auto &button = static_cast<S2Button &>(target);
		_game.getSoundManager().play(10913, false, 100);
		const int roomNo = _loopToRoom[button.getLoop()];
		if (roomNo) {
			_game.getRoomManager().loadGlobalRoom(roomNo);
		}
	}

	void showMovie(GLEvent &, GLTarget &target) {
		auto &button = static_cast<S2Button &>(target);
		const Movie &movie = _loopToMovie[button.getLoop() - 29];
		warning("TODO: Captions in _loopToMovie");
		getParent()._flashbackMovieNo = movie.movieNo;
		getParent()._flashbackMovieCaptioner = movie.captioner;
		_game.getRoomManager().loadGlobalRoom(movie.roomNo);
	}

	void playMovie(const bool specialPlayback) {
		_game.getSoundManager().stop(30004);
		const auto movieNo = getParent()._flashbackMovieNo;
		const auto captioner = getParent()._flashbackMovieCaptioner;
		GLPoint position;
		bool forceDouble;
		if (specialPlayback && movieNo == 1020) {
			position = roomTop;
			forceDouble = false;
		} else if (specialPlayback && movieNo == 4020) {
			position = { 182, 44 };
			forceDouble = true;
		} else {
			position = { 159, 70 };
			forceDouble = true;
		}

		_game.getMovieManager().play(movieNo, captioner ? *captioner : nullptr, position, forceDouble, true);
		_game.getSoundManager().play(30004, true, 0);
		_game.getSoundManager().fade(30004, 80, 15, 12);
		_game.getRoomManager().loadGlobalRoom(4111);
	}

	void previousPage(GLEvent &, GLTarget &) {
		_game.getSoundManager().play(10908, false, 100);
		_game.getRoomManager().loadGlobalRoom(getParent()._flashbackPageNo - 1);
	}

	void nextPage(GLEvent &, GLTarget &) {
		_game.getSoundManager().play(10908, false, 100);
		_game.getRoomManager().loadGlobalRoom(getParent()._flashbackPageNo + 1);
	}

	struct Movie {
		int movieNo;
		S2MovieManager::Captioner *captioner;
		int roomNo;
	};

	static constexpr Movie _loopToMovie[] = {
		// starts at 29
		{ 1020, nullptr, 2 },
		{ 2002, nullptr, 15341 },
		{ 2008, nullptr, 15341 },
		{ 2004, nullptr, 15341 },
		{ 2003, nullptr, 15341 },
		{ 2006, nullptr, 15341 },
		{ 2001, nullptr, 15341 },
		{ 2005, nullptr, 15341 },
		{ 2000, nullptr, 15341 },
		{ 2007, nullptr, 15341 },
		{ 4000, nullptr, 2 },
		{ 4010, nullptr, 2 },
		{ 0, nullptr, 0 },
		{ 4020, nullptr, 2 },
		{ 5120, nullptr, 2 },
		{ 5010, nullptr, 2 }
	};

	static constexpr int _loopToRoom[] = {
		0,
		0,
		1920,
		11140,
		11352,
		0,
		10128,
		10121,
		0,
		13211,
		16113,
		16931,
		5011,
		15021,
		18423,
		18131,
		18240,
		0,
		20412,
		14525,
		5010, // + special stuff for movie & caption
		24321,
		6274,
		6353,
		22511,
		5120, // + special stuff for movie & caption
		0,
		21321,
		21354
	};
};

constexpr int S2FlashbackRoom::_loopToRoom[];
constexpr S2FlashbackRoom::Movie S2FlashbackRoom::_loopToMovie[];

#undef self
#define self(name) this, &S2ConfigurationRoom::name

class S2ConfigurationRoom : public S2GlobalSubRoom {
	enum {
		kSliderX = 430,
		kSliderSize = 124,
		kMinPanSpeed = 10,
		kMaxPanSpeed = 50
	};

public:
	using S2GlobalSubRoom::S2GlobalSubRoom;

	virtual void init(const int) override {
		// captioning
		auto *button = &emplaceCel(false, 4120, 0, _game.getInterface().getIsCaptioningOn(), roomBottom, 201);
		button->setSelectHandler([&](GLEvent &event, GLTarget &cel) {
			if (event.getType() == kSciEventMousePress) {
				_game.getSoundManager().play(10913, false, 100);
				const bool newState = !_game.getInterface().getIsCaptioningOn();
				_game.getInterface().setIsCaptioningOn(newState);
				dynamic_cast<GLCel &>(cel).setCel(newState, true);
			}
		});
		button->forceUpdate();

		// smart cursor
		button = &emplaceCel(false, 4120, 1, _game.getRoomManager().getAutoHighlight(), roomBottom, 201);
		button->setSelectHandler([&](GLEvent &event, GLTarget &cel) {
			if (event.getType() == kSciEventMousePress) {
				_game.getSoundManager().play(10913, false, 100);
				const bool newState = !_game.getRoomManager().getAutoHighlight();
				_game.getRoomManager().toggleAutoHighlight();
				dynamic_cast<GLCel &>(cel).setCel(newState, true);
			}
		});
		button->forceUpdate();

		// video size
		button = &emplaceCel(false, 4120, 2, !_game.getMovieManager().getUseHalfScreen(), roomBottom, 201);
		button->setSelectHandler([&](GLEvent &event, GLTarget &cel) {
			if (event.getType() == kSciEventMousePress) {
				_game.getSoundManager().play(10913, false, 100);
				const bool newState = !_game.getMovieManager().getUseHalfScreen();
				_game.getMovieManager().toggleUseHalfScreen();
				dynamic_cast<GLCel &>(cel).setCel(!newState, true);
			}
		});
		button->forceUpdate();

		// volume
		auto *slider = &emplaceCel(false, 4120, 3, 0, GLPoint(kSliderX + kSliderSize * ConfMan.getInt("sfx_volume") / (Audio::Mixer::kMaxMixerVolume + 1), 192), 201);
		slider->setSelectHandler([&](GLEvent &event, GLTarget &cel) {
			startSlider(Slider::Volume, event, static_cast<GLCel &>(cel));
		});
		slider->forceUpdate();

		// brightness
		slider = &emplaceCel(false, 4120, 3, 0, GLPoint(kSliderX + kSliderSize * _game.getGamma() / (GfxPalette32::numGammaTables - 1), 223), 201);
		slider->setSelectHandler([&](GLEvent &event, GLTarget &cel) {
			startSlider(Slider::Gamma, event, static_cast<GLCel &>(cel));
		});
		slider->forceUpdate();

		// sound performance, does nothing in ScummVM
		emplaceCel(false, 4120, 3, 0, GLPoint(kSliderX + kSliderSize, 254), 201).forceUpdate();

		// pan speed (10-50, in SSCI it was 20 to 60)
		slider = &emplaceCel(false, 4120, 3, 0, GLPoint(kSliderX + kSliderSize * _game.getPanSpeed() / 60, 286), 201);
		slider->setSelectHandler([&](GLEvent &event, GLTarget &cel) {
			startSlider(Slider::PanSpeed, event, static_cast<GLCel &>(cel));
		});
		slider->forceUpdate();

		// pan window size, does nothing in ScummVM
		emplaceCel(false, 4120, 3, 0, GLPoint(kSliderX + kSliderSize, 317), 201).forceUpdate();

		auto &sign = emplaceCel(false, 4120, 4, 2, roomBottom, 201);
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

		_solvedPuzzle = false;
		auto &solver = emplaceCel(false, 4120, 5, 0, roomBottom, 201);
		solver.setSelectHandler(self(solvePuzzle));
		_solveButton = &solver;
		_solverCycler.reset(new GLCycler());
		_solverCycler->add(solver, true);
	}

	virtual bool handleEvent(GLEvent &event) override {
		if (event.getType() == kSciEventMouseRelease) {
			stopSlider();
		}
		event.claim();
		return true;
	}

private:
	enum class Slider {
		Volume,
		Gamma,
		PanSpeed
	};

	void startSlider(const Slider type, GLEvent &event, GLCel &cel) {
		if (event.getType() == kSciEventMousePress) {
			_game.getSoundManager().play(12608, true, Audio32::kMaxVolume);
			_sliderType = type;
			_sliderCel = &cel;
			_sliderScript.reset(new GLScript(self(pollSlider)));
		} else if (event.getType() == kSciEventMouseRelease) {
			stopSlider();
		}
	}

	void stopSlider() {
		_game.getSoundManager().stop(12608);
		_sliderScript.reset();
	}

	void pollSlider(GLScript &script, const int) {
		// In SSCI, values were not applied until the configuration room was
		// disposed; we always just live-update the configuration since the
		// other way is unnecessarily complicated and gives a worse UX
		auto value = CLIP(_game.getUser().getMousePosition().x - 430, 0, 124);
		int16 quantisedPosition;

		switch (_sliderType) {
		case Slider::Volume:
			value = value * Audio32::kMaxVolume / 124;
			quantisedPosition = kSliderX + value * 124 / Audio32::kMaxVolume;
			_kernel.audioMixer.setMasterVolume(value);
			break;
		case Slider::Gamma:
			value = value * (GfxPalette32::numGammaTables - 1) / 124;
			quantisedPosition = kSliderX + value * 124 / (GfxPalette32::numGammaTables - 1);
			_kernel.graphicsManager._palette.setGamma(value);
			break;
		case Slider::PanSpeed:
			value = value * (kMaxPanSpeed - kMinPanSpeed) / 124 + kMinPanSpeed;
			quantisedPosition = kSliderX + (value - kMinPanSpeed) * 124 / (kMaxPanSpeed - kMinPanSpeed);
			_game.setPanSpeed(value);
			break;
		}

		_sliderCel->setPosition(GLPoint(quantisedPosition, _sliderCel->getPosition().y), true);

		// was 10 cycles in SSCI; it really does not need to update more than
		// once per tick
		script.setTicks(1);
		script.setState(-1);
	}

	void solvePuzzle(GLEvent &event, GLTarget &) {
		if (event.getType() != kSciEventMousePress || _solvedPuzzle) {
			return;
		}

		_solvedPuzzle = false;
		int nextRoomNo;

		const auto currentRoomNo = room().getCurrentRoomNo();
		switch (currentRoomNo) {
		case 6122:
			_solvedPuzzle = true;
			flags().set(kGameFlag133);
			nextRoomNo = 6121;
			break;

		case 6222:
			_solvedPuzzle = true;
			flags().set(kGameFlag135);
			nextRoomNo = 6221;
			break;

		case 6272:
			_solvedPuzzle = true;
			flags().set(kGameFlag134);
			nextRoomNo = 6271;
			break;

		case 6350:
		case 6351:
		case 6353:
			_solvedPuzzle = true;
			flags().set(kGameFlag219);
			nextRoomNo = 6354;
			inventory().removeAll(S2Inventory::kInv23);
			inventory().setState(S2Inventory::kInv23, S2InventoryState::Used);
			break;

		case 6371:
			_solvedPuzzle = true;
			flags().set(kGameFlag218);
			nextRoomNo = 6375;
			break;

		case 14430:
			_solvedPuzzle = true;
			flags().set(kGameFlag160);
			nextRoomNo = flags().get(kGameFlag107) ? 14420 : 14431;
			break;

		case 15601:
			_solvedPuzzle = true;
			flags().set(kGameFlag169);
			flags().set(kGameFlag167);
			nextRoomNo = flags().get(kGameFlag108) ? 15600 : 15605;
			break;

		case 17450:
			_solvedPuzzle = true;
			nextRoomNo = flags().get(kGameFlag175) ? 17410 : 17431;
			flags().set(kGameFlag175);
			break;

		case 19450:
			_solvedPuzzle = true;
			flags().set(kGameFlag182);
			nextRoomNo = flags().get(kGameFlag109) ? 19420 : 19236;
			break;

		case 21113:
			_solvedPuzzle = true;
			flags().set(kGameFlag188);
			nextRoomNo = 21110;
			break;

		case 21321:
			_solvedPuzzle = true;
			flags().set(kGameFlag69);
			nextRoomNo = 21300;
			break;

		case 21390:
			_solvedPuzzle = true;
			flags().set(kGameFlag186);
			nextRoomNo = 21301;
			break;

		case 22730:
			_solvedPuzzle = true;
			flags().set(kGameFlag190);
			nextRoomNo = 22711;
			break;

		case 23150:
			_solvedPuzzle = true;
			flags().set(kGameFlag192);
			nextRoomNo = 23143;
			break;

		case 24531:
			_solvedPuzzle = true;
			flags().set(kGameFlag194);
			nextRoomNo = flags().get(kGameFlag104) ? 24530 : 24532;
			break;

		case 36132:
			_solvedPuzzle = true;
			flags().set(kGameFlag172);
			nextRoomNo = 36100;
			break;

		case 48201:
		case 48205:
		case 48207:
		case 48212:
			_solvedPuzzle = true;
			nextRoomNo = 48000;
			flags().set(GameFlag(205 + currentRoomNo - 48201));
			break;
		case 48202:
		case 48206:
		case 48210:
		case 48211:
			_solvedPuzzle = true;
			nextRoomNo = 28400;
			flags().set(GameFlag(205 + currentRoomNo - 48201));
			break;
		case 48203:
		case 48204:
		case 48208:
		case 48209:
			_solvedPuzzle = true;
			nextRoomNo = 28700;
			flags().set(GameFlag(205 + currentRoomNo - 48201));
			break;

		default:
			_solvedPuzzle = false;
		}

		if (_solvedPuzzle) {
			user().setIsHandsOn(false);
			for (auto i = 0; i < 30; ++i) {
				score().doEvent(kScore6);
			}
			setScript(self(animateUnlock), 0, nextRoomNo);
		}
	}

	void animateUnlock(GLScript &script, const int state) {
		switch (state) {
		case 0:
			_solverCycler.reset(new GLEndBackCycler());
			_solverCycler->add(*_solveButton);
			_solverCycler->start(script);
			break;

		case 1:
			_solverCycler.reset(new GLEndCycler());
			_solveButton->setLoop(6);
			_solverCycler->add(*_solveButton);
			_solverCycler->start(script);
			break;

		case 2:
			user().setIsHandsOn(true);
			interface().resetButtons();
			room().setNextRoomNo(script.getData());
			_solverCycler.reset();
		}
	}

	Slider _sliderType;
	GLCel *_sliderCel;
	Common::ScopedPtr<GLScript> _sliderScript;

	Common::ScopedPtr<GLScript> _neonSign;

	bool _solvedPuzzle;
	GLCel *_solveButton;
	Common::ScopedPtr<GLCycler> _solverCycler;
};

#undef self
#define self(name) this, &S2CreditsRoom::name

class S2CreditsRoom : public S2GlobalSubRoom {
public:
	using S2GlobalSubRoom::S2GlobalSubRoom;

	virtual void init(const int roomNo) override {
		auto &hotspot = emplaceHotspot(false, 0, 0, 639, 479);
		hotspot.setMouseUpHandler([=](GLEvent &, GLTarget &) {
			if (roomNo == 4407) {
				finish();
			} else {
				next();
			}
		});
		setScript(self(creditsScript));

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

class S2MapRoom : public S2GlobalSubRoom {
public:
	using S2GlobalSubRoom::S2GlobalSubRoom;

	virtual void init(const int) override {
		for (int flag = kGameFlag85, celNo = 0; flag <= kGameFlag101; ++flag, ++celNo) {
			int celLoopNo;
			int buttonLoopOffset;
			if (_game.getFlags().get(GameFlag(flag))) {
				celLoopNo = 18;
				buttonLoopOffset = 19;
			} else {
				celLoopNo = 0;
				buttonLoopOffset = 1;
			}

			emplaceCel(false, 4130, celLoopNo, celNo, roomBottom, 201).show();
			auto &button = emplaceButton(true, true, 4130, celNo + buttonLoopOffset, 0, roomBottom);
			button.setHighlightedFace(4130, celNo + buttonLoopOffset, 1);
			button.setMouseUpHandler([this, flag, celNo](GLEvent &event, GLTarget &) {
				event.claim();

				if (!_game.getFlags().get(GameFlag(flag))) {
					return;
				}

				const auto &jump = _jumps[celNo];
				_game.getRoomManager().getPanorama().setPanX(jump.panX);
				_game.getRoomManager().setNextRoomNo(jump.roomNo);
				_game.getSoundManager().play(10905, false, 100);
				if (_game.getInventoryManager().hasPrayerStick()) {
					_game.getInterface().changeLife(-2);
				}
				_game.getInterface().resetButtons();
			});
		}
	}

private:
	struct Jump {
		int roomNo;
		int16 panX;
	};

	static constexpr Jump _jumps[] = {
		{ 6390, 123 },
		{ 6420, 1888 },
		{ 6200, 1315 },
		{ 6190, 51 },
		{ 6230, 1504 },
		{ 6120, 892 },
		{ 6310, 1052 },
		{ 6100, 637 },
		{ 6270, 1984 },
		{ 6220, 1255 },
		{ 6250, 1479 },
		{ 6250, 895 },
		{ 6240, 744 },
		{ 6270, 1240 },
		{ 6410, 396 },
		{ 6290, 680 },
		{ 6540, 680 }
	};
};

constexpr S2MapRoom::Jump S2MapRoom::_jumps[];

#undef self
#define self(name) this, &S2InventoryRoom::name

class S2InventoryRoom : public S2GlobalSubRoom {
public:
	using S2GlobalSubRoom::S2GlobalSubRoom;

	virtual void init(const int) override {
		_cel.reset(new GLCel(getPlane(), _game.getInventoryManager().getShowingItemCel(), roomBottom, 255));
		_cel->setCycleSpeed(18);
		_cel->setSelectHandler(self(combine));
		_cel->show();
		_cel->forceUpdate();
		_cycler.reset(new GLCycler());
		_cycler->add(*_cel, true);
	}

	virtual void dispose(const int) override {
		_game.getSoundManager().stop(20019);
		_game.getSoundManager().stop(20020);
		_game.getSoundManager().stop(20021);
	}

	virtual bool handleEvent(GLEvent &event) override {
		// SSCI checked to see if the mouse button was released outside of the
		// cel but still within the bounds of the room area of the screen. This
		// meant the only way to get out of the inventory room in most cases was
		// to click on the eye again, since most inventory cels take up the
		// entire room area. For convenience, we extend the clickable area to
		// include the borders around the edge of the room.
		// SSCI also had an extra check to verify the mouse was not over the
		// cel; this is not necessary since GLCel will unconditionally claim the
		// event, so it will never be received by this room handler if it is
		// over the cel.
		if (event.getType() == kSciEventMouseRelease) {
			const Common::Rect room(0, 0, 640, 384);
			if (room.contains(event.getMousePosition()) &&
				!_game.getCursor().hasInventory()) {

				_game.getInventoryManager().hideItem();
			}
		}
		event.claim();
		return true;
	}

private:
	void combine(GLEvent &event, GLTarget &) {
		if (event.getType() == kSciEventMouseRelease) {
			if (_game.getCursor().hasInventory()) {
				auto item = _game.getInventoryManager().combineItems();
				if (item == S2Inventory::kInv47) {
					_game.getUser().setIsHandsOn(false);
					_cycler.reset();
					setScript([&](GLScript &script, const int state) {
						switch (state) {
						case 0:
							_cel.reset(new GLCel(getPlane(), 3047, 1, 0, roomBottom, 300));
							getPlane().repaint();
							_cycler.reset(new GLEndCycler());
							_cycler->add(*_cel);
							_cycler->start(script);
							break;
						case 1:
							_cel->setLoop(0, true);
							_cycler.reset(new GLCycler());
							_cycler->add(*_cel, true);
							_script.reset();
							_game.getUser().setIsHandsOn(true);
							break;
						}
					});
				} else if (item != S2Inventory::None) {
					_cel->setCelRes(_game.getInventoryManager().getBigCel(item), true);
				}
			} else {
				if (_game.getInventoryManager().getShowingItem() == S2Inventory::kInv28) {
					_cel->setLoop(!_cel->getLoop(), true);
				} else if (_game.getInventoryManager().getShowingItem() == S2Inventory::CompleteTapePlayer) {
					setScript([&](GLScript &script, const int state) {
						switch (state) {
						case 0:
							_game.getSoundManager().play(20020, false, 126, false, &script);
							break;
						case 1:
							_game.getSoundManager().play(20019, false, 126, false, &script);
							_game.getInterface().putText(11072);
							break;
						case 2:
							_game.getSoundManager().play(20021, false, 126);
							_script.reset();
							break;
						}
					});
				}
			}
		}
	}

	Common::ScopedPtr<GLCel> _cel;
	Common::ScopedPtr<GLCycler> _cycler;
};

class S2MusicBoxRoom : public S2GlobalSubRoom {
public:
	using S2GlobalSubRoom::S2GlobalSubRoom;

	virtual void init(const int) override {
		auto &cel = emplaceCel(false, _game.getInventoryManager().getShowingItemCel(), roomBottom, 255);
		cel.setCycleSpeed(18);
		cel.show();
		setScript([&](GLScript &script, const int state) {
			switch (state) {
			case 0:
				_game.getSoundManager().fade(30004, 0, 15, 12, true);
				script.setTicks(30);
				break;
			case 1:
				_game.getSoundManager().play(30005, false, 100, false, &script);
				_cycler.reset(new GLEndCycler());
				_cycler->add(cel);
				_cycler->start(script);
				break;
			case 2: {
				_cycler.reset();
				auto &cel2 = emplaceCel(false, 3019, 1, 0, roomBottom, 300);
				cel2.setCycleSpeed(18);
				cel2.show();
				_cycler.reset(new GLCycler());
				_cycler->add(cel2, true);
				break;
			}
			case 3:
				_game.getSoundManager().play(30004, true, 0);
				_game.getSoundManager().fade(30004, 80, 15, 12);
				_cycler->stop();
				_script.reset();
				break;
			}
		});
	}

	virtual void dispose(const int) override {
		_game.getSoundManager().stop(30005);
		if (_cycler) {
			_cycler->clearCaller();
		}
	}

private:
	Common::ScopedPtr<GLCel> _cel;
	Common::ScopedPtr<GLCycler> _cycler;
};

void S2GlobalRoom::init(const int roomNo) {
	_game.getInterface().putText(0);
	flushEvents();

	switch (roomNo) {
	case 4000:
		setSubRoom<S2MainMenuRoom>(roomNo);
		break;
	case 4010:
		setSubRoom<S2NewGameRoom>(roomNo);
		break;
	case 4020:
		setSubRoom<S2LoadGameRoom>(roomNo);
		break;
	case 4100:
		setSubRoom<S2OptionsRoom>(roomNo);
		break;
	case 2:
	case 1920:
	case 4110:
	case 4111:
	case 5010:
	case 5011:
	case 5120:
	case 6274:
	case 6353:
	case 10121:
	case 10128:
	case 11140:
	case 11352:
	case 13211:
	case 14525:
	case 15021:
	case 15341:
	case 16113:
	case 16931:
	case 18131:
	case 18240:
	case 18423:
	case 20412:
	case 20413:
	case 21321:
	case 21354:
	case 21355:
	case 22511:
	case 24321:
	case 24322:
	case 24323:
	case 24324:
	case 24325:
	case 24326:
	case 24327:
	case 24328:
	case 24329:
	case 24330:
	case 24331:
	case 24332:
	case 24333:
	case 24334:
	case 24335:
		setSubRoom<S2FlashbackRoom>(roomNo);
		break;
	case 4120:
		setSubRoom<S2ConfigurationRoom>(roomNo);
		break;
	case 4130:
		setSubRoom<S2MapRoom>(roomNo);
		break;
	case 4300:
		setSubRoom<S2InventoryRoom>(roomNo);
		break;
	case 4301:
		for (auto i = 0; i < 8; ++i) {
			auto &cel = emplaceCel(false, 4301, i, _game.getRandomNumber(0, 25), roomBottom, 255);
			cel.setCycleSpeed(18);
			cel.show();
			cel.setSelectHandler([&](GLEvent &event, GLTarget &) {
				if (event.getType() == kSciEventMouseRelease) {
					auto celNo = cel.getCel() + 1;
					if (celNo == 26) {
						celNo = 0;
					}
					cel.setCel(celNo, true);
				}
			});
			cel.forceUpdate();
		}
		break;
	case 4302:
		setSubRoom<S2MusicBoxRoom>(roomNo);
		break;
	case 4400:
	case 4401:
	case 4402:
	case 4403:
	case 4404:
	case 4405:
	case 4406:
	case 4407:
		setSubRoom<S2CreditsRoom>(roomNo);
		break;
	default:
		error("Unknown global room %d", roomNo);
	}
}

bool S2GlobalRoom::handleEvent(GLEvent &event) {
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
