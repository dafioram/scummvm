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

#ifndef SCI_S2_SYSTEM_GLROBOT_H
#define SCI_S2_SYSTEM_GLROBOT_H

#include "sci/graphics/frameout.h"
#include "sci/s2/system/globject.h"
#include "sci/s2/system/types.h"

namespace Sci {

class AbsGLPlane;
class EventManager;
class RobotDecoder;

class GLRobot : public GLObject {
public:
	GLRobot();
	GLRobot(const uint16 robotNo, AbsGLPlane &plane, const int16 priority = -1, const GLPoint &position = GLPoint(0, 0));
	~GLRobot();

	static void init(GfxFrameout *graphicsManager) {
		_graphicsManager = graphicsManager;
		_robotPlayer = &graphicsManager->_video.getRobotPlayer();
	}

	bool getIsPlaying() const;

	void setCaller(GLObject &caller) { _caller = &caller; }
	void setClient(GLScreenItem &client) { _client = &client; }

	void start(const bool isModal = false, const bool hideClient = true, const bool keepLastFrame = true);
	void pause();
	void resume();
	void terminate(const bool shouldCue = false);

	void doIt() override;

private:
	enum State {
		kModal           = 1,
		kStarted         = 2,
		kFinished        = 4,
		kDisposeAfterUse = 8,
		kHideClient      = 0x10,
		kKeepLastFrame   = 0x20,
		kPaused          = 0x40
	};

	void init();
	void done();
	void cleanUp(const bool shouldCue);

	static GfxFrameout *_graphicsManager;
	static RobotDecoder *_robotPlayer;

	uint16 _robotNo;
	AbsGLPlane *_plane;
	int16 _priority;
	GLPoint _position;
	GLObject *_caller;
	GLScreenItem *_client;
	bool _isDirty;
	int _state;
	int _signal;
};

} // End of namespace Sci

#endif
