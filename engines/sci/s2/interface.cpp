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

#include "sci/s2/bitmap.h"
#include "sci/s2/button.h"
#include "sci/s2/game.h"
#include "sci/s2/interface.h"
#include "sci/s2/message_box.h"
#include "sci/s2/system/glplane.h"

namespace Sci {

S2Interface::S2Interface(S2Game &game) :
	_game(game),
	_isVisible(false),
	_inputEnabled(false),
	_healthRemaining(100),
	_isCaptioningOn(false),
	_hasCaptioningFinished(false) {}

S2Button *S2Interface::makeButton(const int16 loopNo, const GLButton::EventHandler &handler, const bool shouldEnable) const {
	S2Button *button = new S2Button(*_main, 999, loopNo, 0, GLPoint(0, 479), 4);
	button->setDepressedFace(999, loopNo);
	button->setHighlightedFace(999, loopNo);
	button->setDisabledFace(999, loopNo);
	button->setAutoHighlight(true);
	button->setSelectHandler(handler);
	if (shouldEnable) {
		button->enable();
	}
	return button;
}

void S2Interface::init() {
	_background.reset(new GLColoredPlane(Common::Rect(640, 480), 235, 1));
	_game.getPlanes().add(*_background);
	_main.reset(new GLTransparentPlane(Common::Rect(640, 480), 3));
	_game.getPlanes().add(*_main);
	_captions.reset(new GLTransparentPlane(Common::Rect(640, 480), 250));
	_game.getPlanes().add(*_captions);
	_toolbar.reset(new GLScreenItem(*_main, 999, 0, 0, GLPoint(0, 479), 3));
	_toolbar->show();
	_health.reset(new GLScreenItem(*_main, 999, 1, 0, GLPoint(0, 479), 1));
	_health->show();
	_healthMask.reset(new GLScreenItem(*_main, 999, 2, 0, GLPoint(500, 479), 2));
	_healthMask->show();

	_internet.reset(makeButton(3, nullptr));
	_internet->disable(); // :(

	// TODO: handlers
	_flashback.reset(makeButton(4, nullptr));
	_options.reset(makeButton(5, nullptr));
	_map.reset(makeButton(6, nullptr));
	_eye.reset(makeButton(7, nullptr, false));
	_eye->setDisabledFace(999, 7, 0);
	_eye->setEnabledFace(999, 7, 1);
	_eye->setHighlightedFace(999, 7, 2);
	_eye->enable();

	_captionRect = { 2, 2, 512 - 2, 46 - 2 };
	_captionText.reset(new S2Bitmap(512, 46, 255, 255));
	_captionUi.reset(new GLScreenItem(*_captions, *_captionText, GLPoint(64, 383 - 46), 255));
	_captionUi->show();

	// TODO: Inventory table
	for (int slotNo = 0; slotNo < _inventory.size(); ++slotNo) {
		S2InventoryObject *object = new S2InventoryObject(*_main, 3000, 0, 0, Inventory::None, GLPoint(80 + 40 * slotNo, 392), 4, slotNo);
		_inventory[slotNo].reset(object);
		object->show();
		object->enable();
	}

	_isVisible = true;
	_game.getExtras().push_front(this);
	_main->getCast().addEventHandler(*this);
}

void S2Interface::doIt() {
	// There was internet chat stuff in here, omitted since there is no server
	// for a chat

	if (_hasCaptioningFinished && _captionScript && _captionScript->getState() >= 3) {
		_captionScript.reset();
	}
}

bool S2Interface::handleEvent(GLEvent &event) {
	if (event.getType() != kSciEventKeyDown) {
		return event.isClaimed();
	}

	switch (event.getMessage()) {
	case kSciKeyEnter:
		// There was chat stuff here, removed
		break;
	case kSciKeyTab:
		if (!_game.getRoomManager().getCurrentGlobalRoomNo()) {
			// internet was also disabled here, but it is always disabled
			_flashback->disable();
			_options->disable();
			_game.getRoomManager().loadGlobalRoom(4130);
		} else if (_game.getRoomManager().getCurrentGlobalRoomNo() == 4130){
			_game.getRoomManager().unloadGlobalRoom();
			resetButtons();
		}
		event.claim();
		break;
	case kSciKeyF2:
		// this changed the panorama resolution; we always output the highest
		// resolution
		break;
	case kSciKeyF3:
		_isCaptioningOn = !_isCaptioningOn;
		event.claim();
		break;
	case kSciKeyF4:
		_game.getRoomManager().toggleAutoHighlight();
		event.claim();
		break;
	case kSciKeyF5:
		_game.getMovieManager().toggleUseHalfScreen();
		event.claim();
		break;
	case kSciKeyF6:
		// internet-related functionality
		event.claim();
		break;
	case kSciKeyCtrlS:
		_game.save();
		event.claim();
		break;
	case kSciKeyCtrlV: {
		Common::String version;
		Common::File versionFile;
		// SSCI read a version from the configuration file created by setup; we
		// have no such file so use the standalone version file instead
		if (versionFile.open("VERSION.TXT")) {
			version = versionFile.readLine();
		} else if (versionFile.open("VERSION")) {
			version = versionFile.readLine();
		}
		if (!version.empty()) {
			S2MessageBox message(versionFile.readLine(), S2MessageBox::Type::OK);
			message.createS2Dialog();
		}
		event.claim();
	}
	}

	return event.isClaimed();
}

void S2Interface::show() {
	if (_isVisible) {
		return;
	}

	_isVisible = true;
	_toolbar->show();
	_health->show();
	_healthMask->show();
	_eye->show();
	_internet->show();
	_flashback->show();
	_options->show();
	_map->show();
	// TODO: Inventory
}

void S2Interface::hide() {
	if (!_isVisible) {
		return;
	}

	_isVisible = false;
	_toolbar->hide();
	_health->hide();
	_healthMask->hide();
	_eye->hide();
	_internet->hide();
	_flashback->hide();
	_options->hide();
	_map->hide();
}

void S2Interface::putText(const uint16 messageNo) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Interface::disableButtons() {
	_flashback->disable();
	_options->disable();
	_map->disable();
}

void S2Interface::resetButtons() {
	if (_game.getRoomManager().inInteractiveRoom()) {
		_options->enable();
		_map->enable();
		_flashback->enable();
	}
}

void S2Interface::changeLife(const int amount) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

} // End of namespace Sci
