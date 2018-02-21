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

#ifndef SCI_S2_DIALOG_H
#define SCI_S2_DIALOG_H

#include "sci/s2/system/globject.h"
#include "sci/s2/system/glplane.h"

namespace Sci {

class GfxFrameout;
class GLEvent;
class GLUser;
class S2Control;

class S2Dialog : public GLColoredPlane, public GLObject {
public:
	enum class Result {
		None   = 0,
		OK     = 1,
		Cancel = 2,
		Yes    = 3,
		No     = 4
	};

	S2Dialog(const Common::Rect &rect, const uint8 color);
	virtual ~S2Dialog();

	static void init(GfxFrameout *graphicsManager) { _graphicsManager = graphicsManager; }
	static void init(GLUser *user) { _user = user; }

	Result createS2Dialog();

	bool handleEvent(GLEvent &event) override;
	void doIt() override {}

protected:
	void addControl(S2Control &control);
	virtual void show();
	virtual void hide();

	Result _result;

private:
	virtual void dialogEvent(GLEvent &event, S2Control *control) = 0;

	static GfxFrameout *_graphicsManager;
	static GLUser *_user;

	GLSetAsArray<S2Control> _controls;
	bool _isModeless;
};

} // End of namespace Sci

#endif
