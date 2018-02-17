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

#ifndef SCI_S2_BUTTON_H
#define SCI_S2_BUTTON_H

#include "sci/s2/system/glbutton.h"
#include "sci/s2/system/types.h"

namespace Sci {

class AbsGLPlane;
class S2SoundManager;

class S2Button : public GLButton {
public:
	S2Button(AbsGLPlane &plane, const uint16 viewNo, const int16 loopNo, const int16 celNo, const GLPoint &position, const int16 priority = -9999);

	static void init(S2SoundManager *soundManager) { _soundManager = soundManager; }

	bool getAutoHighlight() const { return _autoHighlight; }
	void setAutoHighlight(const bool set);

	void setMouseUpSoundNo(const uint16 soundNo) { _mouseUpSoundNo = soundNo; }

	void doIt() override;

protected:
	virtual void generalSelect(GLEvent &event) override;

private:
	static S2SoundManager *_soundManager;

	bool _autoHighlight;
	uint16 _mouseUpSoundNo;
};

} // End of namespace Sci

#endif
