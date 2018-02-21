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

#ifndef SCI_S2_MESSAGE_BOX_H
#define SCI_S2_MESSAGE_BOX_H

#include "common/ptr.h"
#include "sci/s2/bitmap.h"
#include "sci/s2/button.h"
#include "sci/s2/control.h"
#include "sci/s2/dialog.h"
#include "sci/s2/system/types.h"

namespace Sci {

class S2MessageBox : public S2Dialog {
public:
	enum class Type {
		OK       = 0,
		OKCancel = 1,
		YesNo    = 4
	};

	S2MessageBox(const Common::String &message, Type type, GLPoint position = GLPoint(640, 480), const uint8 backColor = 235);

	static void init(GfxText32 *textManager) { _textManager = textManager; }

	virtual void show() override;
	virtual void hide() override;

private:
	virtual void dialogEvent(GLEvent &event, S2Control *control) override;

	static GfxText32 *_textManager;

	Type _type;
	Common::ScopedPtr<S2Bitmap> _bitmap;
	Common::ScopedPtr<GLScreenItem> _screenItem;
	Common::FixedArray<S2Control *, 2> _controls;
	Common::FixedArray<S2Button *, 2> _buttons;
};

} // End of namespace Sci

#endif
