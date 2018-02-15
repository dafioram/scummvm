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

#ifndef SCI_S2_SYSTEM_GLMOVIE_H
#define SCI_S2_SYSTEM_GLMOVIE_H

#include "sci/graphics/video32.h"
#include "sci/s2/system/globject.h"
#include "sci/s2/system/types.h"

namespace Sci {

// TODO: Get rid of this silly abstraction, it is not needed
class AbsGLMovie : public GLObject {
public:
	AbsGLMovie(const uint16 movieNo = 0);

	virtual VideoPlayer::EventFlags play() = 0;
	virtual void close() = 0;

protected:
	uint16 _movieNo;
	GLPoint _position;
};

class GLVmdMovie : public AbsGLMovie {
public:
	static void init(VMDPlayer *kernelPlayer) { _kernelPlayer = kernelPlayer; }

	using AbsGLMovie::AbsGLMovie;

	virtual VideoPlayer::EventFlags play() override;
	virtual void close() override;
	void setPosition(const GLPoint &position, const VMDPlayer::PlayFlags playFlags, const bool initKernel);
	void setMovieEvent(const VMDPlayer::EventFlags flags, const int lastFrameNo = 0, const int yieldInterval = -1);
	void showCursor(const bool show);

private:
	static VMDPlayer *_kernelPlayer;

	VMDPlayer::PlayFlags _playFlags;
	VMDPlayer::EventFlags _flags;
	int _lastFrameNo;
	int _yieldInterval;
};

} // End of namespace Sci

#endif
