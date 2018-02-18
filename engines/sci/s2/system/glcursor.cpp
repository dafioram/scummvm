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

#include "common/rect.h"
#include "sci/graphics/cursor32.h"
#include "sci/s2/system/glcursor.h"

namespace Sci {

GLCursor::GLCursor(GfxCursor32 &kernelCursor, const GLCelRes &celInfo) :
	_kernelCursor(kernelCursor),
	_normalCel(celInfo),
	_waitCel(celInfo),
	_handsOffCel(celInfo),
	_highlightedCel(celInfo),
	_state(0) {
	updateKernel(celInfo);
}

void GLCursor::setNormalCelRes(const GLCelRes &celInfo) {
	_normalCel = celInfo;
	if (isNormal()) {
		updateKernel(celInfo);
	}
}

void GLCursor::setHighlightedCelRes(const GLCelRes &celInfo) {
	_highlightedCel = celInfo;
	if (isHighlighted()) {
		updateKernel(celInfo);
	}
}

void GLCursor::setHandsOffCelRes(const GLCelRes &celInfo) {
	_handsOffCel = celInfo;
	if (isHandsOff()) {
		updateKernel(celInfo);
	}
}

void GLCursor::setPosition(const Common::Point &position) {
	_position = position;
	_kernelCursor.setPosition(position);
}

void GLCursor::show() {
	_state &= ~kHiddenState;
	setNeedsEvent(true);
	_kernelCursor.show();
}

void GLCursor::hide() {
	_state |= kHiddenState;
	setNeedsEvent(false);
	_kernelCursor.hide();
}

void GLCursor::goHandsOn() {
	if (isHandsOff()) {
		_state &= ~kHandsOffState;
		updateKernel(_normalCel);
	}
}

void GLCursor::goHandsOff() {
	if (!isHandsOff()) {
		_state |= kHandsOffState;
		updateKernel(_handsOffCel);
	}
}

void GLCursor::beginHighlight() {
	if (!isHighlighted()) {
		_state |= kHighlightedState;
		updateKernel(_highlightedCel);
	}
}

void GLCursor::endHighlight() {
	if (isHighlighted()) {
		_state &= ~kHighlightedState;
		updateKernel(_normalCel);
	}
}

void GLCursor::updateKernel(const GLCelRes &celInfo) const {
	_kernelCursor.setView(celInfo.resourceId, celInfo.loopNo, celInfo.celNo);
}

} // End of namespace Sci
