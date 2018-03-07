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
class TimeManager;

template <class CelT>
class AbsGLCycler : public GLObject {
public:
	AbsGLCycler() : GLObject() {}
	AbsGLCycler(CelT &cel) : GLObject() {
		add(cel);
	}
	virtual ~AbsGLCycler();

	static void init(TimeManager *timeManager) { _timeManager = timeManager; }
	static void init(GLExtras *extras) { _extras = extras; }

	int getDirection() const { return _direction; }

	int add(CelT &cel, const bool shouldStart = false);
	void start();
	void start(GLObject &caller);
	void stop();
	void clearCaller() { _caller = nullptr; }

	bool cycleForward() { return _direction == 1; }
	void cycleForward(const bool forward);

	virtual void doIt() override;

protected:
	void incrementCycle() { ++_numCyclesCompleted; }
	virtual int16 nextCel(CelT &client);

private:
	static TimeManager *_timeManager;
	static GLExtras *_extras;

	void done();
	void release();

	GLObject *_caller = nullptr;
	bool _isCycling = false;
	bool _isFinished = false;
	GLSetAsArray<CelT> _cels;
	Common::Array<int> _timings;
	int _direction = 1;
	int _numCyclesCompleted = 0;
};

template <class CelT>
class AbsGLEndCycler : public AbsGLCycler<CelT> {
public:
	using AbsGLCycler<CelT>::AbsGLCycler;
	virtual int16 nextCel(CelT &client) override;
};

template <class CelT>
class AbsGLEndBackCycler : public AbsGLCycler<CelT> {
public:
	using AbsGLCycler<CelT>::AbsGLCycler;
	virtual int16 nextCel(CelT &client) override;
};

template <class CelT>
class AbsGLEndForwardCycler : public AbsGLCycler<CelT> {
public:
	using AbsGLCycler<CelT>::AbsGLCycler;
	virtual int16 nextCel(CelT &client) override;
};

template <class CelT>
class AbsGLEndForwardBackwardCycler : public AbsGLCycler<CelT> {
public:
	using AbsGLCycler<CelT>::AbsGLCycler;
	virtual int16 nextCel(CelT &client) override;
};

template <class CelT>
class AbsGLPingPongCycler : public AbsGLCycler<CelT> {
public:
	using AbsGLCycler<CelT>::AbsGLCycler;

private:
	virtual int16 nextCel(CelT &client) override;
};

template <class CelT>
class AbsGLStartResetCycler : public AbsGLCycler<CelT> {
public:
	using AbsGLCycler<CelT>::AbsGLCycler;

private:
	virtual int16 nextCel(CelT &client) override;
};

template <class CelT>
class AbsGLEndResetCycler : public AbsGLCycler<CelT> {
public:
	using AbsGLCycler<CelT>::AbsGLCycler;

private:
	virtual int16 nextCel(CelT &client) override;
};

// SSCI did not accept the target cel in the constructor and put the target cel
// property on GLCycler even though it is specific to this cycler only
template <class CelT>
class AbsGLCycleToCycler : public AbsGLCycler<CelT> {
public:
	AbsGLCycleToCycler(const int16 targetCel) :
		AbsGLCycler<CelT>(),
		_targetCel(targetCel) {}

private:
	virtual int16 nextCel(CelT &client) override;
	int16 _targetCel;
};

using GLCycler = AbsGLCycler<GLCel>;
using GLEndCycler = AbsGLEndCycler<GLCel>;
using GLEndBackCycler = AbsGLEndBackCycler<GLCel>;
using GLEndForwardCycler = AbsGLEndForwardCycler<GLCel>;
using GLPingPongCycler = AbsGLPingPongCycler<GLCel>;
using GLEndForwardBackwardCycler = AbsGLEndForwardBackwardCycler<GLCel>;

} // End of namespace Sci

#endif
