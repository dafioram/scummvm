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

#ifndef SCI_S2_MOVIE_MANAGER_H
#define SCI_S2_MOVIE_MANAGER_H

#undef abort
#undef FILE
#undef time
#undef mktime
#undef localtime
#undef gmtime
#undef getdate
#undef asctime
#undef difftime
#undef ctime
#undef clock
#include <functional>
#include "sci/s2/system/glmovie.h"
#include "sci/s2/system/glmovie_player.h"
#include "sci/s2/system/globject.h"
#include "sci/s2/system/types.h"

namespace Sci {

class S2Game;
class S2Kernel;

class S2MovieManager : public GLObject {
public:
	using Captioner = std::function<void(S2MovieManager &)>;

	S2MovieManager(S2Kernel &kernel, S2Game &game);

	void pauseRobot();
	void stopRobot(const bool);
	void play(const uint16 movieNo, Captioner captioner = nullptr, const GLPoint &position = GLPoint(64, 0), const bool forceDoublePixels = false, const bool keepRoom = false);

private:
	S2Kernel &_kernel;
	S2Game &_game;

	GLMoviePlayer _player;
	GLVmdMovie _movie;

	bool _preventSkip;
	bool _useHalfScreen;
	int _frameNo;
};

} // End of namespace Sci

#endif
