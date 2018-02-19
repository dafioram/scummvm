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
#include "sci/s2/system/glpanorama.h"

namespace Sci {

GLPanorama::GLPanorama(const Common::Rect &drawRect) :
	GLObject(),
	_plane(drawRect) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void GLPanorama::drawPic(const uint16 panoramaNo, const bool shouldUpdate) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void GLPanorama::updatePanorama(const bool force) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void GLPanorama::doIt() {
	// TODO
}

bool GLPanorama::handleEvent(GLEvent &event) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
	return false;
}

} // End of namespace Sci
