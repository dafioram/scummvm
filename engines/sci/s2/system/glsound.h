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

#ifndef SCI_S2_SYSTEM_GLSOUND_H
#define SCI_S2_SYSTEM_GLSOUND_H

#include "sci/s2/system/globject.h"
#include "sci/s2/system/glscript.h"

namespace Sci {

class Resource;

class GLNode : public GLObject {
public:
	enum class Type {
		Header = 0,
		Sound  = 1,
		Wait   = 2,
		PRS    = 3
	};
};

class GLSound : public GLNode {
public:
	enum class State {
		PlayingOnce    = 0,
		PlayingForever = 1,
		Finished       = 4,
		Fading         = 5
	};

	GLSound(const uint16 soundNo, const State state, const int16 volume, GLObject *const caller = nullptr, const int16 pan = 50, const int somePercent = 100) :
		_caller(caller),
		_resource(nullptr),
		_state(state),
		_resourceNo(soundNo),
		_volume(volume),
		_somePercent(somePercent) {}

	uint16 getResourceNo() const { return _resourceNo; }
	State getState() const { return _state; }
	int16 getVolume() const { return _volume; }
	GLObject *getCaller() const { return _caller; }
	void setCaller(GLObject *caller) { _caller = caller; }
	Resource *getResource() const { return _resource; }

private:
	GLObject *_caller;
	Resource *_resource;
	State _state;
	uint16 _resourceNo;
	int16 _volume;
	int16 _somePercent;
};

class GLWaitNode : public GLNode {

};

class GLPRSNode : public GLNode {

};

class GLHeaderNode : public GLNode {

};

class GLSoundTrack : public GLScript {

};

} // End of namespace Sci

#endif
