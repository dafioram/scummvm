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

#ifndef SCI_S2_SYSTEM_GLTIMER_H
#define SCI_S2_SYSTEM_GLTIMER_H

#include "sci/s2/system/glcue.h"

namespace Sci {

class TimeManager;

// TODO: Get rid of this class, it was never used concretely and was split into
// GLScript
class GLTimer : public GLCue {
public:
	enum class Type {
		None    = 0,
		Cycles  = 1,
		Ticks   = 2,
		Seconds = 3
	};

	GLTimer(GLObject *const cuee, GLObject *const cuer = nullptr, const int data = 0, void *const dataPointer = nullptr);

	static void init(TimeManager *timeManager) { _timeManager = timeManager; }

	// For some reason these methods were on GLScript in SSCI
	bool check() override;
	void doIt() override;
	void setCycles(const uint32 numCycles);
	void setTicks(const uint32 numTicks);
	void setSeconds(const uint32 numSeconds);

protected:
	GLTimer() :
		GLCue(),
		_type(Type::None) {}

	static TimeManager *_timeManager;

	Type _type;
	uint32 _cyclesUntilCue;
	uint32 _tickToCueAt;
	uint32 _secondToCueAt;
};

} // End of namespace Sci

#endif
