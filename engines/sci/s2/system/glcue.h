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

#ifndef SCI_S2_SYSTEM_GLCUE_H
#define SCI_S2_SYSTEM_GLCUE_H

#include "sci/s2/system/globject.h"
#include "sci/s2/system/types.h"

namespace Sci {

class S2Game;

class GLCue : public GLObject {
public:
	static void init(GLExtras *extras) { _extras = extras; }

	GLCue(GLObject *const cuee, GLObject *const cuer = nullptr, const int data = 0, void *const dataPointer = nullptr);

	void doIt() override;
	virtual void cue();
	virtual void cue(GLCue &) override { cue(); }
	virtual bool check() { return true; }

protected:
	GLCue() : GLObject() {}
	void init(GLObject *const cuee, GLObject *const cuer, const int data, void *const dataPointer);

protected:
	static GLExtras *_extras;

	GLObject *_cuee;
	GLObject *_cuer;
	int _data;
	void *_dataPointer;
};

} // End of namespace Sci

#endif
