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

#ifndef SCI_S2_INVENTORY_OBJECT_H
#define SCI_S2_INVENTORY_OBJECT_H

#include "sci/s2/system/glcel.h"
#include "sci/s2/inventory.h"

namespace Sci {

class AbsGLPlane;
class GLEvent;
class S2Game;

class S2InventoryObject : public GLCel {
public:
	S2InventoryObject(AbsGLPlane &plane, const uint16 viewNo, const int16 loopNo, const int16 celNo, Inventory id, const GLPoint &position = GLPoint(0, 0), const int16 priority = -9999, const int slotNo = -1);

	static void init(S2Game *game) { _game = game; }

	bool getIsEnabled() const { return _isEnabled; }
	void enable();
	void disable();
	virtual bool handleEvent(GLEvent &event) override;

private:
	static S2Game *_game;

	Inventory _id;
	bool _isEnabled;
	int _slotNo;
};

} // End of namespace Sci

#endif
