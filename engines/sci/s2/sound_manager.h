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

#ifndef SCI_S2_SOUND_MANAGER_H
#define SCI_S2_SOUND_MANAGER_H

#include "sci/engine/vm_types.h"
#include "sci/s2/system/glsound_manager.h"

namespace Sci {

class S2SoundManager : public GLSoundManager {
public:
	using GLSoundManager::GLSoundManager;

	int getRoomNo() const { return _roomNo; }

	void stopAllSounds();
	void createAmbient(const int roomNo);
	void deleteAmbient(const int roomNo);

private:
	int _roomNo;
};

} // End of namespace Sci

#endif
