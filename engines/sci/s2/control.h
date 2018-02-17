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

#ifndef SCI_S2_CONTROL_H
#define SCI_S2_CONTROL_H

#include "common/array.h"
#include "common/ptr.h"
#include "common/rect.h"
#include "sci/s2/system/globject.h"
#include "sci/s2/system/glset.h"

namespace Sci {

class GLCel;
class GLEvent;
class GLUser;
class S2TextButton;

class S2Control : public GLObject {
public:
	S2Control();
	virtual ~S2Control();

	static void init(GLUser *user) { _user = user; }

	void show();
	void hide();

	bool handleEvent(GLEvent &event) override;
	void doIt() override {}

	// Ownership is transferred
	void addCel(GLCel &cel);
	// Ownership is transferred
	void addButton(S2TextButton &button);

protected:
	virtual void controlEvent(GLEvent &event, GLCel &cel) {}
	virtual void controlEvent(GLEvent &event, S2TextButton &button) {}
	virtual void setRect(const Common::Rect &rect);

private:
	static GLUser *_user;

	Common::Rect _boundingBox;
	int16 _priority;
	bool _isVisible;
	Common::Array<Common::ScopedPtr<GLCel>> _cels;
	GLSetAsArray<S2TextButton> _buttons;
};

} // End of namespace Sci

#endif
