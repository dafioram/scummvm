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

	void setPanX(const int16 x) { _panX = x; }

	void drawPic(const uint16 panoramaNo, const bool shouldUpdate = true);

	// SSCI did not have a `force` parameter, it just meddled with an internal
	// flag of the panorama object first when it wanted to force an update
	void updatePanorama(const bool force = false);

	GLVRPlane &getPlane() { return _plane; }

	virtual void doIt() override;
	virtual bool handleEvent(GLEvent &event) override;

	void addExit(GLPanoramaExit &exit) { _exits.push_back(&exit); }
	void removeExit(GLPanoramaExit &exit) { _exits.remove(&exit); }
	void removeAllExits() { _exits.clear(); }

private:
	static S2Game *_game;

	void buildWarpTable();
	void updateSprites();
	void checkMouse();
	void stretchPanorama();
	void panAudio();

	bool checkSprites(GLEvent &event);
	bool checkExits(GLEvent &event);

	uint16 _resourceNo;
	S2PanoramaImage _screen;
	GLVRPlane _plane;
	S2PanoramaImage _image;
	int16 _panX;
	int16 _panY;
	int _shiftY;
	double _aspectRatio;
	bool _isDirty;
	bool _isUpdating;
	bool _isFrozen;
	Common::FixedArray<int, 2048> _panTable;
	Common::FixedArray<int, 2048> _percentTable;
	int16 _width, _height;
	Common::Rect _deadZone;
	Common::FixedArray<int, 640> _xToYDelta;
	Common::FixedArray<int, 640> _xToYInitial;
	Common::List<GLSound> _sounds;
	// In SSCI this was not a set for some reason, even though there is no way
	// two of the same exit should exist in one panorama
	GLSetAsArray<GLPanoramaExit> _exits;
};

} // End of namespace Sci

#endif
