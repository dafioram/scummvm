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

#include "common/serializer.h"
#include "sci/s2/movie_captions.h"
#include "sci/s2/system/glmovie.h"
#include "sci/s2/system/glmovie_player.h"
#include "sci/s2/system/globject.h"
#include "sci/s2/system/glrobot.h"
#include "sci/s2/system/types.h"

namespace Sci {

class S2Game;
class S2Kernel;

class S2MovieManager : public GLObject, public Common::Serializable {
public:
	S2MovieManager(S2Kernel &kernel, S2Game &game);

	virtual void saveLoadWithSerializer(Common::Serializer &) override;

	void initRobot(const uint16 robotNo, AbsGLPlane &plane, const int16 priority = -1, const GLPoint &position = GLPoint(0, 0));
	void playRobot(const bool isModal = false, const bool hideClient = true, const bool keepLastFrame = true);
	void pauseRobot();
	void resumeRobot();
	void stopRobot(const bool shouldCue);
	void setRobotCaller(GLObject &caller);
	void setRobotClient(GLCel &cel);

	// SSCI accepted a captioner as a parameter, but whenever this was null
	// it would just be determined by the play call internally, so it is omitted
	void play(const uint16 movieNo, const GLPoint &position = GLPoint(64, 0), const bool forceDoublePixels = false, const bool keepRoom = false);

	void play(const uint16 tapeId, const bool blockHypnotism);

	bool getUseHalfScreen() const { return _useHalfScreen; }
	void toggleUseHalfScreen() { _useHalfScreen = !_useHalfScreen; }

	int getFrameNo() const { return _frameNo; }
	void setFrameNo(const int frameNo) { _frameNo = frameNo; }

private:
	S2Kernel &_kernel;
	S2Game &_game;

	GLMoviePlayer _player;
	GLVmdMovie _movie;
	GLRobot _robot;
	bool _robotIsInitialized;

	bool _preventSkip;
	bool _useHalfScreen;
	int _frameNo;

	bool _shouldHypnotise;
	bool _someHypnotise;
};

} // End of namespace Sci

#endif
