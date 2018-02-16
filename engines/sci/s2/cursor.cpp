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

#include "sci/s2/cursor.h"

namespace Sci {

static const GLCelRes defaultCel = GLCelRes::makeView(99, 0, S2Cursor::kNormalCel);

S2Cursor::S2Cursor(GfxCursor32 &kernelCursor) :
	GLCursor(kernelCursor, defaultCel),
	_normalCel(defaultCel),
	_prayerStickNormalCel(defaultCel),
	_inventoryCel(defaultCel),
	_inventoryState(0) {
	setHighlightedCelRes(GLCelRes::makeView(99, 0, kHighlightCel));
	setHandsOffCelRes(GLCelRes::makeView(99, 0, kWaitCel));
	setPosition(Common::Point(319, 191));
}

} // End of namespace Sci
