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

#include "sci/graphics/celobj32.h"
#include "sci/graphics/text32.h"
#include "sci/s2/bitmap.h"
#include "sci/s2/message_box.h"
#include "sci/s2/system/glevent.h"

namespace Sci {

GfxText32 *S2MessageBox::_textManager = nullptr;


S2MessageBox::S2MessageBox(const Common::String &message, Type type, GLPoint position, const uint8 backColor) :
	S2Dialog(Common::Rect(), backColor),
	_type(type),
	_buttons() {

	// TODO: In SSCI this was 249, which caused backgrounds to be green
	// at least on the main menu when failing to open the web browser; maybe try
	// to make it a remapped black instead so it is a semitransparent background
	constexpr auto backgroundColor = 0;

	const int16 borderSize = CelObjView(997, 0, 0)._height;
	const int16 cornerSize = CelObjView(997, 0, 4)._width;
	int16 minWidth = CelObjView(998, 0, 0)._width;
	if (type != Type::OK) {
		minWidth += CelObjView(998, 0, 0)._width + 5;
	}

	if (position.x == 640) {
		position.x /= 2;
		position.y /= 2;
	}

	_textManager->setFont(503);
	Common::Rect textBox = _textManager->getTextSize(message, 320, false);
	if (textBox.width() > textBox.height() * 2) {
		int16 maxWidth = MAX<int16>(sqrt(textBox.width() * textBox.height() * 3 / 2), minWidth);
		if (maxWidth < 200) {
			maxWidth = 200;
		}
		textBox = _textManager->getTextSize(message, maxWidth, false);
	}

	int16 textWidth = textBox.width();
	int16 textHeight = textBox.height();

	int16 height = textHeight + CelObjView(998, 0, 0)._height + borderSize * 2 + 15;
	int16 width = MAX(minWidth, textWidth) + borderSize * 2 + 10;

	int16 widthRemainder = (width - borderSize * 2) % cornerSize;
	if (widthRemainder) {
		int16 extra = cornerSize - widthRemainder;
		width += extra;
		if (minWidth - textWidth > 0) {
			extra += minWidth - textWidth;
		}
		widthRemainder = extra / 2;
	}

	int16 heightRemainder = (height - borderSize * 2) % cornerSize;
	if (heightRemainder) {
		const int16 extra = cornerSize - heightRemainder;
		height += extra;
		heightRemainder = extra / 2;
	}

	position.x -= width / 2;
	position.y -= height / 2;

	Common::Rect dialogBox(width, height);
	dialogBox.moveTo(position);
	dialogBox.clip(Common::Rect(0, 0, 640, 480));
	setRect(dialogBox);

	textBox.moveTo(borderSize + widthRemainder + 5, borderSize + heightRemainder + 5);

	_bitmap.reset(new S2Bitmap(width, height, 255, backgroundColor));
	_bitmap->drawView(997, 0, 0, 0, 0);
	_bitmap->drawView(997, 0, 1, width - borderSize, 0);
	_bitmap->drawView(997, 0, 2, 0, height - borderSize);
	_bitmap->drawView(997, 0, 3, width - borderSize, height - borderSize);
	for (uint x = borderSize; x < width - borderSize; x += cornerSize) {
		_bitmap->drawView(997, 0, 4, x, 0);
		_bitmap->drawView(997, 0, 5, x, height - borderSize);
	}
	for (uint y = borderSize; y < height - borderSize; y += borderSize) {
		_bitmap->drawView(997, 0, 6, 0, y);
		_bitmap->drawView(997, 0, 7, width - borderSize, y);
	}
	_bitmap->drawText(message, textBox, 202, backgroundColor, 255, 503, kTextAlignLeft, 255);

	int16 buttonY = height - (borderSize + 5 + CelObjView(998, 0, 0)._height);
	int16 buttonHalfWidth = CelObjView(998, 0, 0)._width / 2;
	if (type == Type::OK) {
		_buttons[0] = new S2Button(*this, 998, 0, 0, GLPoint(width / 2 - buttonHalfWidth, buttonY));
		_buttons[0]->setMouseUpSoundNo(10908);
	} else {
		const int16 startLoopNo = (type == Type::OKCancel ? 0 : 2);
		_buttons[0] = new S2Button(*this, 998, startLoopNo, 0, GLPoint(width / 3 - buttonHalfWidth, buttonY));
		_buttons[1] = new S2Button(*this, 998, startLoopNo + 1, 0, GLPoint(width / 3 * 2 - buttonHalfWidth, buttonY));
	}

	for (uint i = 0; i < _buttons.size(); ++i) {
		auto &button = _buttons[i];
		if (button) {
			auto &control = _controls[i];
			button->enable(false);
			control = new S2Control();
			control->addCel(*button);
			addControl(*control);
		}
	}

	_screenItem.reset(new GLScreenItem(*this, *_bitmap, GLPoint(0, 0), 1));
}

void S2MessageBox::show() {
	_screenItem->show();
	S2Dialog::show();
}

void S2MessageBox::hide() {
	_screenItem->hide();
	S2Dialog::hide();
}

void S2MessageBox::dialogEvent(GLEvent &event, S2Control *control) {
	if (event.getType() == kSciEventQuit) {
		_result = (_type == Type::YesNo) ? Result::No : Result::Cancel;
	} else if (event.getType() == kSciEventMouseRelease) {
		const uint buttonIndex = (control == _controls[1]) ? 1 : 0;
		const S2Button &button = *_buttons[buttonIndex];
		if (button.checkIsOnMe(event.getMousePosition())) {
			if (buttonIndex == 0) {
				_result = (_type == Type::YesNo) ? Result::Yes : Result::OK;
			} else {
				_result = (_type == Type::YesNo) ? Result::No : Result::Cancel;
			}
		}
	// SSCI did not allow keyboard interaction with the dialogue; we add this
	// for convenience
	} else if (event.getType() == kSciEventKeyDown) {
		if (event.getMessage() == kSciKeyEsc) {
			switch (_type) {
			case Type::YesNo:
				_result = Result::No;
				break;
			case Type::OKCancel:
				_result = Result::Cancel;
				break;
			case Type::OK:
				_result = Result::OK;
				break;
			}
		} else if (event.getMessage() == kSciKeyEnter) {
			switch (_type) {
			case Type::YesNo:
				_result = Result::Yes;
				break;
			case Type::OKCancel:
			case Type::OK:
				_result = Result::OK;
				break;
			}
		}
	}
}

} // End of namespace Sci
