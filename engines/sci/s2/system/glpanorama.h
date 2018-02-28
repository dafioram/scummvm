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

#ifndef SCI_S2_SYSTEM_GLPANORAMA_H
#define SCI_S2_SYSTEM_GLPANORAMA_H

#include "common/array.h"
#include "common/list.h"
#include "common/ptr.h"
#include "common/rect.h"
#include "sci/s2/panorama_image.h"
#include "sci/s2/panorama_sprite.h"
#include "sci/s2/system/globject.h"
#include "sci/s2/system/glpanorama_exit.h"
#include "sci/s2/system/glvr_plane.h"
#include "sci/s2/system/glset.h"

namespace Sci {

class GLEvent;
class GLVRPlane;

class GLPanorama : public GLObject {
public:
	GLPanorama(const Common::Rect &drawRect);

	static void init(S2Game *game) { _game = game; }

	uint16 getResourceNo() const { return _resourceNo; }

	int16 &panX() { return _panX; }
	int16 &panY() { return _panY; }
	void setPanX(const int16 x) { _panX = x; }

	void drawPic(const uint16 panoramaNo, const bool shouldUpdate = true);

	// SSCI did not have a `force` parameter, it just meddled with an internal
	// flag of the panorama object first when it wanted to force an update
	void updatePanorama(const bool force = false);

	GLVRPlane &getPlane() { return _plane; }

	virtual void doIt() override;
	virtual bool handleEvent(GLEvent &event) override;

	template <typename ...Args>
	void addExit(Args && ... args) { _exits.emplace_back(args...); }
	void removeAllExits() { _exits.clear(); }

	void attachSound(const uint16 soundNo, const int16 panX, const int16 volume = 80);
	void detachSound(const uint16 soundNo);

private:
	static constexpr int _shiftY = 22;
	static constexpr double _aspectRatio = 1.3;
	static S2Game *_game;

	void buildWarpTable();
	void updateSprites();
	void checkMouse();
	void stretchPanorama();
	void panAudio();

	bool checkSprites(GLEvent &event);
	bool checkExits(GLEvent &event);

	GLPoint getUnwarpedPoint(const GLPoint &point) const {
		return GLPoint(
			(point.x + _panX) % _image.getHeight(),
			(_xToYInitial[point.y] + (_panY + point.y) * _xToYDelta[point.x]) >> _shiftY
		);
	}

	uint16 _resourceNo;
	S2PanoramaImage _screen;
	GLVRPlane _plane;
	S2PanoramaImage _image;
	int16 _panX;
	int16 _panY;
	bool _isDirty;
	bool _isFrozen;
	Common::FixedArray<uint, 2048> _panTable;
	Common::FixedArray<uint, 2048> _percentTable;
	int16 _width, _height;
	Common::Rect _deadZone;
	Common::FixedArray<uint, 640> _xToYDelta;
	Common::FixedArray<uint, 640> _xToYInitial;
	Common::List<GLSound> _sounds;
	Common::List<S2PanoramaSprite> _sprites;
	Common::Array<GLPanoramaExit> _exits;
};

} // End of namespace Sci

#endif
