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

#ifndef SCI_S2_SYSTEM_GLSOUND_MANAGER_H
#define SCI_S2_SYSTEM_GLSOUND_MANAGER_H

#include "common/list.h"
#include "sci/sound/audio32.h"
#include "sci/s2/system/globject.h"
#include "sci/s2/system/glsound.h"

namespace Sci {

class ResourceManager;
class S2Game;

class GLSoundManager : public GLObject {
public:
	GLSoundManager(S2Game &game, Audio32 &mixer);

	uint16 play(const uint16 soundNo, const bool loop = false, const int16 volume = Audio32::kMaxVolume, const bool paused = false, GLObject *const caller = nullptr, const reg_t soundNode = NULL_REG);
	void pan(const uint16 soundNo, const int16 pan, const reg_t soundNode = NULL_REG);
	void fade(const uint16 soundNo, const int16 volume, const int16 speed, const int16 steps, const bool stopOnFade = false, GLObject *const caller = nullptr, const reg_t soundNode = NULL_REG);
	void stop();
	void stop(const int soundNo, const reg_t soundNode = NULL_REG);
	void doIt();

	// Merged CreateSndTrack and AttachSndTrack
	GLSoundTrack &createSoundTrack();
	void deleteSoundTrack(const int trackId);

protected:
	S2Game &_game;

private:
	Audio32 &_mixer;
	Common::List<GLSound> _sounds;
	Common::List<GLSoundTrack> _tracks;
};

} // End of namespace Sci

#endif