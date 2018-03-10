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

#ifndef SCI_S2_ROOM_H
#define SCI_S2_ROOM_H

#include "common/ptr.h"
#include "sci/s2/hotspot.h"
#include "sci/s2/flags.h"
#include "sci/s2/game.h"
#include "sci/s2/kernel.h"
#include "sci/s2/panorama_sprite.h"
#include "sci/s2/room_manager.h"
#include "sci/s2/system/glcel.h"
#include "sci/s2/system/glpanorama.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/glscript.h"
#include "sci/s2/transparent_cel.h"

namespace Sci {

class GLEvent;
class S2SubRoom;

class S2Room {
	friend class S2SubRoom;

public:
	static constexpr GLPoint absTop     = {  0,   0 };
	static constexpr GLPoint absBottom  = {  0, 479 };
	static constexpr GLPoint roomTop    = { 64,   0 };
	static constexpr GLPoint roomBottom = { 64, 383 };

	S2Room(S2Kernel &kernel, S2Game &game) :
		_kernel(kernel),
		_game(game) {}

	virtual ~S2Room() { clear(); }

	virtual void init(const int roomNo) = 0;

	virtual void dispose(const int roomNo);

	virtual void doIt();

	virtual bool handleEvent(GLEvent &event);

	void reload(const int roomNo) {
		dispose(roomNo);
		init(roomNo);
	}

	int debugScriptState() const {
		if (!_script) {
			return -2;
		}

		return _script->getState();
	}

	void debugScriptState(const int state) {
		if (!_script) {
			return;
		}

		resetState(state);
	}

protected:
	S2RoomManager &room() { return _game.getRoomManager(); }
	S2SoundManager &sound() { return _game.getSoundManager(); }
	GameFlags &flags() { return _game.getFlags(); }
	S2InventoryManager &inventory() { return _game.getInventoryManager(); }
	S2MovieManager &movie() { return _game.getMovieManager(); }
	S2PhoneManager &phone() { return _game.getPhoneManager(); }
	S2Interface &interface() { return _game.getInterface(); }
	GLUser &user() { return _game.getUser(); }
	S2ScoringManager &score() { return _game.getScoringManager(); }
	S2Cursor &cursor() { return _game.getCursor(); }

	virtual GLPicturePlane &getPlane() const {
		return _game.getRoomManager().getGamePlane();
	}

	template <typename T, typename ...Args>
	void setSubRoom(const int roomNo, Args && ...args) {
		_activeSubRoom.reset(new T(_kernel, _game, *this, args...));
		initSubRoom(roomNo);
	}

	template <typename T, typename ...Args>
	T &emplaceChild(Args && ...args) {
		T *object = new T(getPlane(), args...);
		_children.emplace_back(object);
		return *object;
	}

	void addAsCel(GLCel &cel) {
		_cels.push_back(&cel);
		_game.getRoomManager().addCel(cel);
	}

	void removeChild(S2PanoramaSprite &sprite) {
		auto it = Common::find_if(_sprites.begin(), _sprites.end(), [&](Common::ScopedPtr<S2PanoramaSprite> &ptr) {
			return ptr.get() == &sprite;
		});
		if (it != _sprites.end()) {
			_game.getRoomManager().getPanorama().removeSprite(sprite);
			_sprites.erase(it);
		}
	}

	void removeChild(S2Exit &exit) {
		auto it = Common::find(_exits.begin(), _exits.end(), &exit);
		if (it != _exits.end()) {
			_game.getRoomManager().removeExit(exit);
			_exits.erase(it);
		}

		removeChild(static_cast<GLObject &>(exit));
	}

	void removeChild(S2Hotspot &hotspot) {
		auto it = Common::find(_hotspots.begin(), _hotspots.end(), &hotspot);
		if (it != _hotspots.end()) {
			_game.getRoomManager().removeHotspot(hotspot);
			_hotspots.erase(it);
		}

		removeChild(static_cast<GLObject &>(hotspot));
	}

	void removeChild(GLCel &cel) {
		auto it = Common::find(_cels.begin(), _cels.end(), &cel);
		if (it != _cels.end()) {
			_game.getRoomManager().removeCel(cel);
			_cels.erase(it);
		}

		removeChild(static_cast<GLObject &>(cel));
	}

	void removeChild(GLObject &object) {
		auto it = Common::find_if(_children.begin(), _children.end(), [&](Common::ScopedPtr<GLObject> &ptr) {
			return ptr.get() == &object;
		});
		if (it != _children.end()) {
			_children.erase(it);
		}
	}

	template <typename ...Args>
	S2Button &emplaceButton(const bool autoHighlight, const bool enable, Args && ...args) {
		auto &button = emplaceChild<S2Button>(args...);
		if (autoHighlight) {
			button.setAutoHighlight(true);
		}
		if (enable) {
			button.enable();
		}
		return button;
	}

	template <typename ...Args>
	GLCel &emplaceCel(const bool autoHighlight, Args && ...args) {
		auto &cel = emplaceChild<GLCel>(args...);
		if (autoHighlight) {
			_cels.push_back(&cel);
			_game.getRoomManager().addCel(cel);
		}
		return cel;
	}

	template <typename ...Args>
	S2TransparentCel &emplaceTransparentCel(const bool autoHighlight, Args && ...args) {
		auto &cel = emplaceChild<S2TransparentCel>(args...);
		if (autoHighlight) {
			_cels.push_back(&cel);
			_game.getRoomManager().addCel(cel);
		}
		return cel;
	}

	template <typename ...Args>
	S2Hotspot &emplaceHotspot(const bool autoHighlight, Args && ...args) {
		auto &hotspot = emplaceChild<S2Hotspot>(args...);
		if (autoHighlight) {
			_hotspots.push_back(&hotspot);
			_game.getRoomManager().addHotspot(hotspot);
		}
		return hotspot;
	}

	template <typename ...Args>
	S2Exit &emplaceExit(const bool autoHighlight, Args && ...args) {
		auto &exit = emplaceChild<S2Exit>(args...);
		if (autoHighlight) {
			_exits.push_back(&exit);
			_game.getRoomManager().addExit(exit);
		}
		return exit;
	}

	template <typename ...Args>
	S2PanoramaSprite &emplaceSprite(const bool willUpdate, Args && ...args) {
		S2PanoramaSprite *sprite = new S2PanoramaSprite(args...);
		_sprites.emplace_back(sprite);
		_game.getRoomManager().getPanorama().addSprite(*sprite, willUpdate);
		return *sprite;
	}

	template <typename ...Args>
	void addPanoramaExit(Args && ...args) {
		_game.getRoomManager().getPanorama().addExit(args...);
	}

	void reshowCel(GLCel &cel) {
		getPlane().getCast().remove(cel);
		cel.show();
	}

	template <typename ... Args>
	void setScript(Args && ...args) {
		_script.reset(new GLScript(args...));
	}

	void resetState(const int newState) {
		auto handler(_script->getChangeState());
		_script.reset(new GLScript(handler, newState));
	}

	void flushEvents() const {
		_kernel.eventManager.flushEvents();
	}

	void clear() {
		_cycler.reset();

		if (!_keepPanoramaExits) {
			_game.getRoomManager().getPanorama().removeAllExits();
		}

		if (!_keepPanoramaSprites) {
			for (auto &&sprite : _sprites) {
				_game.getRoomManager().getPanorama().removeSprite(*sprite);
			}
			_sprites.clear();
		}

		for (auto &&hotspot : _hotspots) {
			if (hotspot) {
				_game.getRoomManager().removeHotspot(*hotspot);
			}
		}
		for (auto &&cel : _cels) {
			if (cel) {
				_game.getRoomManager().removeCel(*cel);
			}
		}
		for (auto &&exit : _exits) {
			if (exit) {
				_game.getRoomManager().removeExit(*exit);
			}
		}
		_exits.clear();
		_hotspots.clear();
		_cels.clear();
		_children.clear();
		_script.reset();
	}

	void enter(const int roomNo, const uint16 enterSound, const uint16 exitSound, const bool addExit) {
		_enterSoundNo = enterSound;
		_exitSoundNo = exitSound;
		setScript(this, &S2Room::enterScript, 0, roomNo);
		if (addExit) {
			// cannot use getCurrentBaseRoomNo because the base room for rooms
			// 50000-50999 is actually 30000 for whatever reason
			auto exitNo = room().getCurrentRoomNo() / 1000 * 1000 + 999;
			emplaceExit(true, exitNo, S2Cursor::kBackCel);
		}
	}

	virtual void enterScript(GLScript &script, const int state) {
		switch (state) {
		case 0:
			user().setIsHandsOn(false);
			if (_enterDelay) {
				script.setSeconds(1);
			} else {
				script.cue();
			}
			break;

		case 1:
			_cel.reset(new GLCel(getPlane(), script.getData(), 0, 0, roomBottom));
			_cel->show();
			_cycler.reset(new GLEndCycler());
			_cycler->add(*_cel);
			_cycler->start(script);
			sound().play(_enterSoundNo, false, 100);
			break;

		case 2:
			getPlane().getCast().remove(*_cel);
			_cycler.reset();
			user().setIsHandsOn(true);
			break;
		}
	}

	void enterFrom(const int previousRoomNo, const int16 panX, const uint16 soundNo) {
		if (room().getPreviousRoomNo() == previousRoomNo) {
			if (panX >= 0) {
				room().getPanorama().setPanX(panX);
			}
			if (soundNo) {
				sound().play(soundNo);
			}
		}
	}

	void exitBorder(const int roomNo, const bool top = true, const bool left = true, const bool right = true) {
		if (left) {
			emplaceExit(true, roomNo, 64, 0, 144, 383, S2Cursor::kBackCel);
		}
		if (right) {
			emplaceExit(true, roomNo, 495, 0, 575, 383, S2Cursor::kBackCel);
		}
		if (top) {
			emplaceExit(true, roomNo, 145, 0, 494, 80, S2Cursor::kBackCel);
		}
	}

	S2Kernel &_kernel;
	S2Game &_game;

	Common::ScopedPtr<S2SubRoom> _activeSubRoom;
	Common::ScopedPtr<GLScript> _script;
	Common::Array<Common::ScopedPtr<GLObject>> _children;
	Common::Array<Common::ScopedPtr<S2PanoramaSprite>> _sprites;
	Common::Array<GLCel *> _cels;
	Common::Array<S2Hotspot *> _hotspots;
	Common::Array<S2Exit *> _exits;

	bool _enterDelay = false;
	uint16 _enterSoundNo;
	uint16 _exitSoundNo;

	// Subclasses must clear these appropriately at the end of their lifecycles.
	// They are not auto-disposed by this class when the room changes since they
	// may need to persist across rooms in an implementation-specific manner
	// (usually these persist to the 999 room, but sometimes they persist to
	// other rooms too).
	Common::ScopedPtr<GLCel> _cel;
	Common::ScopedPtr<GLCycler> _cycler;

	bool _keepPanoramaExits = false;
	bool _keepPanoramaSprites = false;

private:
	void initSubRoom(const int roomNo);
};

class S2SubRoom : public S2Room {
public:
	S2SubRoom(S2Kernel &kernel, S2Game &game, S2Room &parent) :
		S2Room(kernel, game),
		_parent(parent) {}

	virtual void init(const int roomNo) override {}
	virtual void dispose(const int roomNo) override {}
	virtual void doIt() override {}
	virtual bool handleEvent(GLEvent &) override { return false; }

protected:
	virtual GLPicturePlane &getPlane() const override { return _parent.getPlane(); }

	S2Room &_parent;
};

} // End of namespace Sci

#endif
