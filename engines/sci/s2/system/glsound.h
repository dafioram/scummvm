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

#include "sci/sound/audio32.h"
#include "sci/s2/system/globject.h"
#include "sci/s2/system/glscript.h"

namespace Sci {

class Resource;
class ResourceManager;
class S2Game;

class GLNode : public GLObject {
public:
	enum class Type {
		Header = 0,
		Sound  = 1,
		Wait   = 2,
		PRS    = 3
	};

	GLNode(Type type) : _type(type) {}

	Type getType() const { return _type; }

private:
	Type _type;
};

class GLSound : public GLNode {
public:
	enum class State {
		PlayingOnce    = 0,
		PlayingForever = 1,
		Finished       = 4,
		Fading         = 5
	};

	GLSound(const uint16 soundNo, const State state, const int16 volume, GLObject *const caller = nullptr, const int16 pan = 50, const int randomness = 100) :
		GLNode(GLNode::Type::Sound),
		_caller(caller),
		_state(state),
		_resourceNo(soundNo),
		_volume(volume),
		_pan(pan),
		_randomness(randomness) {}

	uint16 getResourceNo() const { return _resourceNo; }
	State getState() const { return _state; }
	int16 getVolume() const { return _volume; }
	int16 getPan() const { return _pan; }
	GLObject *getCaller() const { return _caller; }
	void setCaller(GLObject *caller) { _caller = caller; }
	int16 getRandomness() const { return _randomness; }

private:
	GLObject *_caller;
	State _state;
	uint16 _resourceNo;
	int16 _volume;
	int16 _pan;
	int16 _randomness;
};

class GLWaitNode : public GLNode {
public:
	GLWaitNode(const int minSeconds, const int maxSeconds) :
		GLNode(GLNode::Type::Wait),
		_minSeconds(minSeconds),
		_maxSeconds(maxSeconds) {}

	int getMinimum() const { return _minSeconds; }
	int getMaximum() const { return _maxSeconds; }

private:
	int _minSeconds;
	int _maxSeconds;
};

class GLPRSNode : public GLNode {
public:
	GLPRSNode() : GLNode(GLNode::Type::PRS) {}

	template <typename ... Args>
	void addSound(Args && ...args) {
		_nodes.emplace_back(new GLSound(std::forward<Args>(args)...));
	}

private:
	Common::List<Common::ScopedPtr<GLNode>> _nodes;
};

class GLHeaderNode : public GLNode {
public:
	GLHeaderNode() : GLNode(GLNode::Type::Header) {}
};

class GLSoundTrack : public GLScript {
public:
	GLSoundTrack(const int trackId) :
		GLScript(),
		_trackId(trackId) {}

	void init() {
		GLScript::init(this, &GLSoundTrack::changeState);
	}

	static void init(S2Game *game) { _game = game; }
	static void init(Audio32 *mixer) { _mixer = mixer; }

	int getTrackId() const { return _trackId; }

	void addWaitNode(const int minTicks, const int maxTicks = 0);
	void addSoundNode(const uint16 soundNo, const int16 volume = Audio32::kMaxVolume, const bool loop = true, const int16 pan = 50, const int somePercent = 100);
	void addPRSNode(const uint16 soundNo, const int somePercent = 100, const int16 volume = Audio32::kMaxVolume, const int16 pan = 50);

	void play();

private:
	using Nodes = Common::List<Common::ScopedPtr<GLNode>>;

	static S2Game *_game;
	static Audio32 *_mixer;

	void changeState(GLScript &script, const int state);

	bool _isPaused;
	bool _isStopped;
	int _trackId;
	uint16 _currentSoundNo;

	Nodes::iterator _currentNode;
	Nodes _nodes;
};

} // End of namespace Sci

#endif
