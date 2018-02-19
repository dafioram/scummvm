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

#include "common/savefile.h"
#include "common/translation.h"
#include "sci/s2/button.h"
#include "sci/s2/control.h"
#include "sci/s2/dialog.h"
#include "sci/s2/engine.h"
#include "sci/s2/game.h"
#include "sci/s2/kernel.h"
#include "sci/s2/message_box.h"
#include "sci/s2/savegame.h"
#include "sci/s2/system/glcue.h"
#include "sci/s2/system/glcycler.h"
#include "sci/s2/system/glquit_handler.h"

namespace Sci {
S2Game::S2Game(S2Engine &engine, S2Kernel &kernel) :
	_engine(engine),
	_kernel(kernel),
	_rng(""),
	_user(*this),
	_cursor(kernel.graphicsManager._cursor),
	_soundManager(*this, kernel.audioMixer),
	_interface(kernel, *this),
	_movieManager(kernel, *this),
	_inventoryManager(*this),
	_roomManager(kernel, *this),
	_flags(),
	_saveGameSlotNo(-1),
	_volume(Audio32::kMaxVolume) {
	GLCue::init(&_extras);
	GLCycler::init(&_extras);
	GLTarget::init(&_user);
	S2Button::init(&_soundManager);
	S2Control::init(&_user);
	S2Dialog::init(&_user);
	S2InventoryObject::init(this);
	_phoneManager.init();
}

void S2Game::run() {
	init();

	_interface.disableButtons();
	_roomManager.loadRoom(1000);
	_roomManager.initRoom(1000);

	play();

	if (!_roomManager.getIsSaved() && _roomManager.inInteractiveRoom()) {
		S2MessageBox message(_("Your game '%s' has not yet been saved. "
							   "Would you like to save this game before exiting?"), S2MessageBox::Type::YesNo);
		if (message.createS2Dialog() == S2MessageBox::Result::Yes) {
			save();
		}
	}

	// TODO: A save-before-quit confirmation dialogue was shown here; add this
	// if it feels like a good idea later

	// Conditional checks that were here are moved into their respective
	// functions
	_movieManager.stopRobot(false);
	_roomManager.unloadGlobalRoom();
	_roomManager.disposeRoom(_roomManager.getCurrentRoomNo());
	_roomManager.unloadRoom();
}

void S2Game::quit() {
	_engine.quitGame();
}

bool S2Game::hasSaveGames() const {
	return !_engine.listSaves().empty();
}

Common::Array<S2SaveGameMetadata> S2Game::getSaveGameList() const {
	Common::Array<S2SaveGameMetadata> list;
	for (const auto &gameFilename : _engine.listSaves()) {
		Common::ScopedPtr<Common::InSaveFile> stream(_engine.getSaveFileManager()->openForLoading(gameFilename));
		const auto version = stream->readByte();
		const auto name = stream->readPascalString();
		const auto ticksElapsed = stream->readUint32LE();
		const auto timestamp = stream->readUint64LE();
		const S2SaveGameMetadata metadata = { name, timestamp, ticksElapsed, version };
		list.push_back(metadata);
	}
	return list;
}

bool S2Game::canSaveNow() const {
	// TODO: The room list conditions come from the game's save-before-quit
	// confirmation code. The hands on test exists since the user would not
	// normally be able to save the game when it is non-interactable. In the
	// case a user quit the game via the OS, it looks like it would intercept
	// the WM_SYSCOMMAND/SC_CLOSE message and hold the window open until it got
	// back to the main game loop and exited through the shouldQuit flag. There
	// may be some additional conditions which need to be checked, like the
	// state of the game's UI, and that cannot be done until the game is
	// actually running
	if (!_user.getIsHandsOn()) {
		return false;
	}
	const int currentRoomNo = _roomManager.getCurrentRoomNo();
	if (currentRoomNo >= 2000) {
		switch (currentRoomNo) {
		case 26721:
		case 26850:
		case 26900:
		case 26901:
			break;
		default:
			return true;
		}
	}
	return false;
}

bool S2Game::canLoadNow() const {
	// TODO: Figure out these conditions
	return true;
}

Common::String S2Game::getMessage(const uint16 resourceNo) const {
	auto *resource = _kernel.resourceManager.findResource(ResourceId(kResourceTypeText, resourceNo), false);
	if (resource) {
		return Common::String(reinterpret_cast<const char *>(resource->data()), resource->size());
	}
	return "";
}

void S2Game::play() {
	GLQuitHandler quitHandler;

	_extras.push_front(&_soundManager);
	_user.getPrimaDonnas().push_back(&quitHandler);

	_user.setIsHandsOn(true);

	while (!_engine.shouldQuit()) {
		doIt();
	}

	_user.getPrimaDonnas().remove(&quitHandler);
	_extras.remove(&_soundManager);
}

void S2Game::doIt() {
	_planes.doIt();
	// TODO: This list (and presumably most doit lists) might be mutated in the
	// middle of the doit loop, so iterators cannot be used since they may be
	// invalidated. This happens at least when a text caption is removed. This
	// also means that some entries might be skipped for a loop. This was also a
	// problem in SSCI, so it is unclear if sets should remove elements by
	// nulling them out and then adding an extra packing step, or what.
	for (auto i = 0; i < _extras.size(); ++i) {
		_extras[i]->doIt();
	}
	_kernel.graphicsManager.kernelFrameOut(true);
	_user.doIt();
}

void S2Game::init() {
	_cursor.show();
	_interface.init();
	_inventoryManager.init();

	// SSCI had multiple heap allocations + instantiations here in this order:
	// * S2Cursor
	// * S2SndMgr (+ call to always preload audio data into memory)
	// * S2RoomMgr
	// * S2Interface (+ call to S2Interface::Init)
	// * MovieMgr (+ slow computer configuration)
	// * S2InvMgr
	// * S2ResourceMgr
	// * ScoringManager (+ call to ScoringManager::Init)
	// * FlagManager (+ call to FlagManager::Init)
	// * S2PhoneMgr (+ call to S2PhoneMgr::Init)
	// * Plus a bunch of initialisation code for save game catalogues
	// * Plus a few global initialisations
	//
	// Most of these have been eliminated simply by making these objects part of
	// S2Game or S2Kernel so they get initialised at the same time as the engine
	// itself. There are a couple of exceptions, though:
	//
	// * S2Cursor initialisation, which was in this init function, has been
	//   mostly moved into its constructor since it was only ever instantiated
	//   once, at game startup
	// * S2SndMgr (actually GLSndManager)'s constructor added itself to the list
	//   of global extra objects and removed itself in the destructor; this is
	//   now handled in `S2Engine::play`
	// * S2ResourceMgr was used for loading panorama bitmaps; this functionality
	//   has been moved into the standard ResourceManager
	// * No slow computer configuration is used
	// * Save game management is done by ScummVM itself
	// * Globals not in the S2Engine class initializer list were unused, so have
	//   been omitted

	_kernel.audioMixer.setAttenuatedMixing(false);
}

void S2Game::save() {
	_engine.saveGameState(_saveGameSlotNo, _saveGameName);
}

} // End of namespace Sci
