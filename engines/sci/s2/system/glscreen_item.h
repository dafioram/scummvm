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

#ifndef SCI_S2_SYSTEM_GLSCREEN_ITEM_H
#define SCI_S2_SYSTEM_GLSCREEN_ITEM_H

#include "common/ptr.h"
#include "sci/graphics/screen_item32.h"
#include "sci/s2/system/globject.h"
#include "sci/s2/system/types.h"

namespace Sci {

class AbsGLPlane;
class GfxFrameout;
class S2Bitmap;

class GLScreenItem : public virtual GLObject {
public:
	GLScreenItem(AbsGLPlane &plane, const GLCelRes &celInfo, const GLPoint &position, const int16 priority = -9999, const ScaleInfo &scaleInfo = ScaleInfo());
	GLScreenItem(AbsGLPlane &plane, const uint16 viewNo, const int16 loopNo, const int16 celNo, const GLPoint &position, const int16 priority = -9999, const ScaleInfo &scaleInfo = ScaleInfo());
	GLScreenItem(AbsGLPlane &plane, S2Bitmap &bitmap, const GLPoint &position, const int16 priority = -9999, const ScaleInfo &scaleInfo = ScaleInfo());
	virtual ~GLScreenItem();

	static void init(GfxFrameout *graphicsManager) { _graphicsManager = graphicsManager; }

	AbsGLPlane &getPlane() const { assert(_plane); return *_plane; }
	bool getIsVisible() const { return _isVisible; }
	int16 getPriority() const { return _screenItem->_priority; }

	const GLPoint &getPosition() const { return _screenItem->_position; }
	void setPosition(const GLPoint &position, const bool shouldUpdate = false);

	void setCelRes(const GLCelRes &celInfo, const bool shouldUpdate = false);
	int16 getLoop() const { return _celInfo.loopNo; }
	void setLoop(const int16 loopNo, const bool shouldUpdate = false);
	int16 getCel() const { return _celInfo.celNo; }
	void setCel(const int16 celNo, const bool shouldUpdate = false);
	int16 getLastCel() const;

	virtual void doIt() override {}

	virtual void show();
	virtual void hide();

	void forceUpdate();

protected:
	bool hasVLC() const { return _celInfo.type == kCelTypeView; }
	void load(const GLCelRes &celInfo, const bool shouldUpdate = false);
	virtual void update();
	bool getNowSeenRect(Common::Rect &result) const;

private:
	static GfxFrameout *_graphicsManager;

	GLCelRes _celInfo;
	GLPoint _position;
	AbsGLPlane *_plane;
	Common::ScopedPtr<ScreenItem> _screenItem;
	bool _isDirty;
	bool _isVisible;
};

} // End of namespace Sci

#endif
