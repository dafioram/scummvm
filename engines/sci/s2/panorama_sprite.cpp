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

#include "sci/s2/panorama_sprite.h"

namespace Sci {

S2PanoramaSprite::S2PanoramaSprite(const uint16 resourceNo, const GLPoint &position, const int16 celNo, const int16 numCels, const bool hasTransparency, const bool isVisible) :
	S2PanoramaImage(resourceNo),
	_celNo(celNo),
	_numCels(numCels),
	_celWidth(getWidth() / numCels),
	_celHeight(getHeight()),
	_scaleInfo(100),
	_hasTransparency(hasTransparency),
	_isVisible(isVisible),
	_cycleSpeed(6),
	_moveSpeed(2),
	_stepSize(30, 10) {
	_position = position;
	_isSprite = true;
	setBounds(Common::Rect(position.x, position.y, position.x + _celWidth, position.y + _celHeight));
	// TODO: Allocated cel wxh pixels for something
}

void S2PanoramaSprite::setBounds(const Common::Rect &bounds) {
	if (_scaleInfo != 100) {
		int deltaX = bounds.width() * _scaleInfo / 200;
		int deltaY = bounds.height() * _scaleInfo / 200;
		_bounds = Common::Rect(bounds.left + deltaX,
							   bounds.top + deltaY,
							   bounds.right - deltaX,
							   bounds.bottom - deltaY);
	} else {
		_bounds = bounds;
	}
}

} // End of namespace Sci
