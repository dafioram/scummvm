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

#include "sci/s2/game.h"
#include "sci/s2/kernel.h"
#include "sci/s2/system/glsound_manager.h"

namespace Sci {

GLSoundManager::GLSoundManager(S2Game &game, ResourceManager &resourceManager, Audio32 &mixer) :
	_game(game),
	_resourceManager(resourceManager),
	_mixer(mixer) {}

void GLSoundManager::doIt() {
	for (auto sound = _sounds.begin(); sound != _sounds.end(); ) {
		const auto state = sound->getState();
		if (state == GLSound::State::Finished || state == GLSound::State::Fading) {
			const auto volume = _mixer.getVolume(sound->getResourceNo());
			int16 finishedVolume;
			if (state == GLSound::State::Fading) {
				finishedVolume = sound->getVolume();
			} else {
				finishedVolume = 0;
			}

			if (volume == finishedVolume) {
				if (sound->getCaller()) {
					_game.addCue(sound->getCaller(), this);
				}

				if (sound->getResource()) {
					_resourceManager.unlockResource(sound->getResource());
				}

				sound = _sounds.erase(sound);
			} else {
				++sound;
			}
		}
	}
}

void GLSoundManager::stop() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

} // End of namespace Sci
