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

#ifndef SCI_S2_SYSTEM_GLCYCLER_H
#define SCI_S2_SYSTEM_GLCYCLER_H

#include "sci/s2/system/globject.h"
#include "sci/s2/system/glset.h"

namespace Sci {

class GLCel;

class GLCycler : public GLObject {
public:
	GLCycler() : GLObject() {
		warning("TODO: %s", __PRETTY_FUNCTION__);
	}
	GLCycler(GLCel &cel) : GLObject() {
		warning("TODO: %s", __PRETTY_FUNCTION__);
		add(cel);
	}

	int add(GLCel &cel, const bool start = false);
	void start(GLObject * = nullptr);
	void stop();

private:
	GLObject *_caller = nullptr;
	bool _isCycling = false;
	bool _isComplete = false;
	int _targetCel;
	GLSetAsArray<GLCel> _cels;
	Common::Array<int> _timings;
	int _direction = 1;
	int _numCyclesCompleted = 0;
};

class GLPingPongCycler : public GLCycler {};

} // End of namespace Sci

#endif
