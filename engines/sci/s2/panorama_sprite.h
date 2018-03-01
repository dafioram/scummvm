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

#ifndef SCI_S2_PANORAMA_SPRITE_H
#define SCI_S2_PANORAMA_SPRITE_H

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
#include "sci/s2/panorama_image.h"
#include "sci/s2/system/glcycler.h"
#include "sci/s2/system/types.h"

namespace Sci {

class S2PanoramaSprite;
using S2PanoramaCycler = AbsGLCycler<S2PanoramaSprite>;
class S2PanoramaMover;

class S2PanoramaSprite : public S2PanoramaImage {
public:
	using EventHandler = std::function<void()>;

	S2PanoramaSprite(const uint16 resourceNo, const GLPoint &position, const int16 celNo = 0, const int16 numCels = 1, const bool transparent = false, const bool visible = true);

	const EventHandler &getMouseDownHandler() const { return _mouseDownHandler; }
	void setMouseDownHandler(const EventHandler &handler) { _mouseDownHandler = handler; }

	const Common::Rect &getRect() { return _bounds; }
	void setBounds(const Common::Rect &bounds);

	int getCycleSpeed() const { return _cycleSpeed; }

	void setCycler(S2PanoramaCycler *cycler) { _cycler = cycler; }

	int16 getCel() const { return _celNo; }
	void setCel(const int16 celNo, const bool = false) { _celNo = celNo; }
	int16 getLastCel() const { return _numCels - 1; }

	int16 getCelWidth() const { return _celWidth; }
	int16 getCelHeight() const { return _celHeight; }

	bool getHasTransparency() const { return _hasTransparency; }
	bool getIsVisible() const { return _isVisible; }

	Common::Array<byte> &getSavedPixels() { return _savedPixels; }

private:
	Common::Rect _bounds;
	EventHandler _mouseDownHandler;

	Common::Array<byte> _savedPixels;
	int16 _celNo;
	int16 _numCels;
	int16 _celWidth;
	int16 _celHeight;
	int16 _scaleInfo;
	int _cycleSpeed;
	int _moveSpeed;
	GLPoint _stepSize;
	bool _isVisible;
	bool _hasTransparency;
	S2PanoramaCycler *_cycler;
	S2PanoramaMover *_mover;
};

} // End of namespace Sci

#endif
