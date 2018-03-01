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
#include "sci/s2/system/glsound.h"

namespace Sci {

GLSoundManager::GLSoundManager(S2Game &game, Audio32 &mixer) :
	_game(game),
	_mixer(mixer) {}

uint16 GLSoundManager::play(const uint16 soundNo, const bool loop, const int16 volume, const bool paused, GLObject *const caller, const reg_t soundNode) {
	if (isPlaying(soundNo, soundNode)) {
		return 0;
	}

	const ResourceId resourceId(kResourceTypeAudio, soundNo);
	const uint16 length = _mixer.restart(resourceId, !paused, loop, volume, soundNode, false);

	GLSound sound(soundNo, GLSound::State::Finished, volume, caller);
	_sounds.push_front(sound);

	// SSCI did not return length, but we do so that we can wait for samples
	// more easily
	return length;
}

void GLSoundManager::pan(const uint16 soundNo, const int16 pan, const reg_t soundNode) {
	_mixer.setPan(ResourceId(kResourceTypeAudio, soundNo), soundNode, pan);
}

void GLSoundManager::fade(const uint16 soundNo, const int16 targetVolume, const int16 speed, const int16 steps, const bool stopAfterFade, GLObject *const caller, const reg_t soundNode) {
	_mixer.fadeChannel(ResourceId(kResourceTypeAudio, soundNo), soundNode, targetVolume, speed, steps, stopAfterFade);
	if (caller) {
		GLSound sound(soundNo, GLSound::State::Fading, targetVolume, caller);
		_sounds.push_front(sound);
	}
}

void GLSoundManager::stop() {
	_mixer.stop(kAllChannels);
}

void GLSoundManager::stop(const int soundNo, const reg_t soundNode) {
	if (soundNo == kAllChannels) {
		_mixer.stop(kAllChannels);
	} else {
		for (auto &sound : _sounds) {
			if (sound.getResourceNo() == soundNo && sound.getCaller()) {
				sound.setCaller(nullptr);
				break;
			}
		}
	}

	_mixer.stop(ResourceId(kResourceTypeAudio, soundNo), soundNode);
}

void GLSoundManager::doIt() {
	for (auto sound = _sounds.begin(); sound != _sounds.end(); ) {
		const auto state = sound->getState();
		if (state == GLSound::State::Finished || state == GLSound::State::Fading) {
			const auto resourceId = ResourceId(kResourceTypeAudio, sound->getResourceNo());
			const auto volume = _mixer.getVolume(_mixer.findChannelById(resourceId));
			int16 finishedVolume;
			if (state == GLSound::State::Fading) {
				finishedVolume = sound->getVolume();
			} else {
				finishedVolume = -1;
			}

			if (volume == finishedVolume) {
				if (sound->getCaller()) {
					new GLCue(sound->getCaller(), this);
				}

				sound = _sounds.erase(sound);
				continue;
			}
		}
		++sound;
	}
}

bool GLSoundManager::isPlaying(const uint16 soundNo, const reg_t soundNode) const {
	return getPosition(soundNo, soundNode) != -1;
}

int GLSoundManager::getPosition(const uint16 soundNo, const reg_t soundNode) const {
	return _mixer.getPosition(ResourceId(kResourceTypeAudio, soundNo), soundNode);
}

GLSoundTrack &GLSoundManager::createSoundTrack() {
	_tracks.emplace_front(_tracks.size());
	_tracks.front().init();
	return _tracks.front();
}

void GLSoundManager::deleteSoundTrack(const int trackId) {
	auto it = Common::find_if(_tracks.begin(), _tracks.end(), [=](const GLSoundTrack &track) {
		return track.getTrackId() == trackId;
	});
	if (it != _tracks.end()) {
		_tracks.erase(it);
	}
}

} // End of namespace Sci
