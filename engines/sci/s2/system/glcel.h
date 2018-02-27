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

#ifndef SCI_S2_SYSTEM_GLCEL_H
#define SCI_S2_SYSTEM_GLCEL_H

#include "sci/graphics/screen_item32.h"
#include "sci/s2/system/glfeature.h"
#include "sci/s2/system/glscreen_item.h"
#include "sci/s2/system/types.h"

namespace Sci {

class AbsGLPlane;
class GLCycler;

class GLCel : public GLScreenItem, public GLFeature {
public:
	GLCel(AbsGLPlane &plane, const uint16 viewNo, const int16 loopNo, const int16 celNo, const GLPoint &position, const int16 priority = -9999, const ScaleInfo &scaleInfo = ScaleInfo());
	GLCel(AbsGLPlane &plane, const GLCelRes &celInfo, const GLPoint &position, const int16 priority = -9999, const ScaleInfo &scaleInfo = ScaleInfo());

	virtual void show() override;
	virtual void hide() override;

	int getCycleSpeed() const { return _cycleSpeed; }
	void setCycleSpeed(const int numTicks) { _cycleSpeed = numTicks; }

	int getMoveSpeed() const { return _moveSpeed; }
	void setMoveSpeed(const int numTicks) { _moveSpeed = numTicks; }

	void setCycler(GLCycler *cycler) { _cycler = cycler; }

protected:
	virtual void update() override;
	void setBounds();

private:
	GLCycler *_cycler;
	int _cycleSpeed;
	int _moveSpeed;
	GLPoint _stepSize;
};

} // End of namespace Sci

#endif
