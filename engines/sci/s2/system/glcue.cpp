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

#include "sci/s2/system/glcue.h"
#include "sci/s2/game.h"

namespace Sci {

GLExtras *GLCue::_extras = nullptr;

GLCue::GLCue(GLObject *const cuee, GLObject *const cuer, const int data, void *const dataPointer) :
	GLObject() {
	init(cuee, cuer, data, dataPointer);
}

void GLCue::init(GLObject *const cuee, GLObject *const cuer, const int data, void *const dataPointer) {
	_cuee = cuee;
	_cuer = cuer;
	_data = data;
	_dataPointer = dataPointer;
	_extras->push_back(this);
}

void GLCue::doIt() {
	if (check()) {
		cue();
	}
}

void GLCue::cue() {
	_cuee->cue(*this);
	_extras->remove(this);
	delete this;
}

} // End of namespace Sci
