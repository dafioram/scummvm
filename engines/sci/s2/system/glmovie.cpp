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

#include "sci/graphics/video32.h"
#include "sci/s2/system/glmovie.h"

namespace Sci {

VMDPlayer *GLVmdMovie::_kernelPlayer = nullptr;

AbsGLMovie::AbsGLMovie(const uint16 movieNo) : _movieNo(movieNo) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

VideoPlayer::EventFlags GLVmdMovie::play() {
	if (_kernelPlayer->getStatus() == VMDPlayer::kVMDNotOpen) {
		_kernelPlayer->open(Common::String::format("%u.vmd", _movieNo), VMDPlayer::kOpenFlagNone);
	}

	return _kernelPlayer->kernelPlayUntilEvent(_flags, _lastFrameNo, _yieldInterval);
}

void GLVmdMovie::close() {
	_kernelPlayer->close();
}

void GLVmdMovie::setPosition(const GLPoint &position, const VMDPlayer::PlayFlags playFlags, const bool initKernel) {
	_position = position;
	_playFlags = playFlags;
	if (initKernel) {
		_kernelPlayer->init(position.x, position.y, playFlags, 0, 0, 0);
	}
}

void GLVmdMovie::setMovieEvent(const VMDPlayer::EventFlags flags, const int lastFrameNo, const int yieldInterval) {
	_flags = flags;
	_lastFrameNo = lastFrameNo;
	_yieldInterval = yieldInterval;
}

void GLVmdMovie::showCursor(const bool show) {
	_kernelPlayer->setShowCursor(show);
}

} // End of namespace Sci
