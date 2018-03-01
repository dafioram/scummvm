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

	virtual void dispose(const int roomNo) {
		_activeSubRoom.reset();
		clear();
	}

	virtual void doIt();

	virtual bool handleEvent(GLEvent &event);

	void reload(const int roomNo) {
		dispose(roomNo);
		init(roomNo);
	}

protected:
	virtual GLPicturePlane &getPlane() const {
		return _game.getRoomManager().getGamePlane();
	}

	template <typename T, typename ...Args>
	void setSubRoom(Args && ...args) {
		_activeSubRoom.reset(new T(_kernel, _game, *this, args...));
	}

	template <typename T, typename ...Args>
	T &emplaceChild(Args && ...args) {
		T *object = new T(getPlane(), args...);
		_children.emplace_back(object);
		return *object;
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
	S2PanoramaSprite &emplaceSprite(const bool shouldUpdate, Args && ...args) {
		S2PanoramaSprite *sprite = new S2PanoramaSprite(args...);
		_children.emplace_back(sprite);
		_game.getRoomManager().getPanorama().addSprite(*sprite);
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
		if (!_keepPanoramaExits) {
			_game.getRoomManager().getPanorama().removeAllExits();
		}
		_game.getRoomManager().getPanorama().removeAllSprites();

		for (auto &&sprite : _sprites) {
			if (sprite) {
				_game.getRoomManager().getPanorama().removeSprite(*sprite);
			}
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
		_sprites.clear();
		_exits.clear();
		_hotspots.clear();
		_cels.clear();
		_children.clear();
		_script.reset();
	}

	S2RoomManager &room() { return _game.getRoomManager(); }
	S2SoundManager &sound() { return _game.getSoundManager(); }
	GameFlags &flags() { return _game.getFlags(); }
	S2InventoryManager &inventory() { return _game.getInventoryManager(); }
	S2MovieManager &movie() { return _game.getMovieManager(); }
	S2PhoneManager &phone() { return _game.getPhoneManager(); }
	S2Interface &interface() { return _game.getInterface(); }
	GLUser &user() { return _game.getUser(); }
	S2ScoringManager &score() { return _game.getScoringManager(); }

	void enterFrom(const int previousRoomNo, const int16 panX, const uint16 soundNo) {
		if (_game.getRoomManager().getPreviousRoomNo() == previousRoomNo) {
			if (panX >= 0) {
				_game.getRoomManager().getPanorama().setPanX(panX);
			}
			if (soundNo) {
				_game.getSoundManager().play(soundNo);
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
	Common::Array<S2PanoramaSprite *> _sprites;
	Common::Array<GLCel *> _cels;
	Common::Array<S2Hotspot *> _hotspots;
	Common::Array<S2Exit *> _exits;

	bool _keepPanoramaExits = false;
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
