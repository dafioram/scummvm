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
#include "sci/s2/system/glplane.h"

namespace Sci {

S2Interface::S2Interface(S2Game &game) :
	_game(game),
	_isHidden(false),
	_isCaptioningOn(false),
	_healthRemaining(100) {}

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

	// TODO: Inventory
	warning("TODO: %s inventory", __PRETTY_FUNCTION__);
}

void S2Interface::doIt() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Interface::show() {
	if (!_isHidden) {
		return;
	}

	_isHidden = false;
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
	if (_isHidden) {
		return;
	}

	_isHidden = true;
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
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void S2Interface::changeLife(const int amount) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

} // End of namespace Sci
