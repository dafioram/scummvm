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

#include "audio/mididrv.h"
#include "audio/mixer.h"
#include "common/config-manager.h"
#include "sci/engine/features.h"
#include "sci/sound/sound.h"
#include "sci/sound/drivers/adlib.h"
#include "sci/sound/drivers/genmidi.h"
#include "sci/sound/drivers/mt32.h"

namespace Sci {

SoundManager::SoundManager(ResourceManager &resMan, SegManager &segMan, GameFeatures &features, GuestAdditions &guestAdditions) :
	_resMan(resMan),
	_segMan(&segMan),
	_guestAdditions(guestAdditions),
	_driverEnabledState(true),
	_soundVersion(features.detectDoSoundType()),
	_numServerSuspensions(0),
	_samplePlayer(*this, *g_system->getMixer()) {
	_preferSampledSounds = _soundVersion >= SCI_VERSION_2 ||
		g_sci->getGameId() == GID_GK1DEMO ||
		ConfMan.getBool("prefer_digitalsfx");
}

void SoundManager::systemSuspend(const bool pause) {
	Common::StackLock lock(_mutex);

	enableSoundServer(!pause);
	if (pause) {
		_driverEnabledState = _driver->isEnabled();
		_driver->enable(false);
	} else {
		_driver->enable(_driverEnabledState);
	}
}

void SoundManager::initDriver(MusicType musicType, Common::Platform platform) {
	// HACK: The Fun Seeker's Guide demo doesn't have patch 3 and the
	// version of the Adlib driver (adl.drv) that it includes is
	// unsupported. That demo doesn't have any sound anyway, so this
	// shouldn't be fatal.
	// The GK2 demo has a similar issue, it has no audio drivers at all but
	// plays no MIDI so it is not fatal for it to have no driver.
	if (g_sci->getGameId() == GID_FUNSEEKER ||
		(g_sci->getGameId() == GID_GK2 && g_sci->isDemo())) {
		return;
	}

	switch (musicType) {
	case MT_ADLIB:
		// FIXME: There's no Amiga sound option, so we hook it up to AdLib
		if (platform == Common::kPlatformAmiga || platform == Common::kPlatformMacintosh) {
			error("TODO AmigaMac");
//			_driver.reset(makeAmigaMacDriver(resMan, _soundVersion));
		} else {
			_driver.reset(makeAdLibDriver(_resMan, _soundVersion));
		}
		break;
	case MT_PCJR:
		error("TODO PCJr");
//		_driver.reset(makePcJrDriver(resMan, _soundVersion));
		break;
	case MT_PCSPK:
		error("TODO PCSpeaker");
//		_driver.reset(makePcSpeakerDriver(resMan, _soundVersion));
		break;
	case MT_CMS:
		error("TODO CMS");
//		_driver.reset(makeCmsDriver(resMan, _soundVersion));
		break;
	case MT_TOWNS:
		error("TODO FM-Towns");
//		_driver.reset(makeFmTownsDriver(resMan, _soundVersion));
		break;
	case MT_MT32:
		_driver.reset(makeMt32Driver(_resMan, _soundVersion));
		break;
	case MT_GM:
		if (ConfMan.getBool("native_fb01")) {
			error("TODO FB-01");
//			_driver.reset(makeFb01Driver(resMan, _soundVersion));
		} else {
			_driver.reset(makeGeneralMidiDriver(_resMan, _soundVersion, false));
		}
		break;
	default:
		error("Unknown music type %d", musicType);
	}

	if (!_driver) {
		error("Failed to initialize sound driver");
	}
}

#pragma mark -
#pragma mark MIDI server

void SoundManager::enableSoundServer(const bool enable) {
	Common::StackLock lock(_mutex);

	// In SSCI1early- this function used a boolean instead of a counter, but
	// games could not access this function at all, so we can just always use
	// the counter mode

	if (!enable) {
		++_numServerSuspensions;
	} else if (_numServerSuspensions) {
		--_numServerSuspensions;
	}
}

#pragma mark -
#pragma mark Effects

uint8 SoundManager::getMasterVolume() const {
	Common::StackLock lock(_mutex);

	if (ConfMan.getBool("mute")) {
		// When a game is muted, the master volume is set to zero so that
		// mute applies to external MIDI devices, but this should not be
		// communicated to the game as it will cause the UI to be drawn with
		// the wrong (zero) volume for music
		return (ConfMan.getInt("music_volume") + 1) * kMaxMasterVolume / Audio::Mixer::kMaxMixerVolume;
	}

	return _driver->getMasterVolume();
}

void SoundManager::setMasterVolume(uint8 volume) {
	Common::StackLock lock(_mutex);
	if (volume > kMaxMasterVolume) {
		volume = kMaxMasterVolume;
	}
	_driver->setMasterVolume(volume);
}

bool SoundManager::isSoundEnabled() const {
	Common::StackLock lock(_mutex);
	return _driver->isEnabled();
}

void SoundManager::setSoundOn(const bool enable) {
	Common::StackLock lock(_mutex);
	_driver->enable(enable);
}

#pragma mark -
#pragma mark Digital sample playback


SoundManager::SamplePlayer::SamplePlayer(SoundManager &manager, Audio::Mixer &mixer) :
	_manager(manager),
	_mixer(mixer),
	_playing(false) {}

SoundManager::SamplePlayer::~SamplePlayer() {
	_mixer.stopHandle(_handle);
}

void SoundManager::SamplePlayer::load(SciSpan<const byte> data, const uint8 volume, const bool loop) {
	if (volume == 0 || _manager.getMasterVolume() == 0 || !_manager.isSoundEnabled()) {
		return;
	}

	// TODO: Move sample marker somewhere else
	enum { kSampleMarker = 0xfe };
	while (*data++ == kSampleMarker);

	_mixer.stopHandle(_handle);
	_loop = loop;
	_playing = false;
	_pos = 8;
	_sampleRate = data.getUint16LEAt(0);
	_size = data.getUint16LEAt(2);
	_loopStart = data.getUint16LEAt(4);
	_loopEnd = data.getUint16LEAt(6);
	_data = data.subspan(0, _size);
	_mixer.playStream(Audio::Mixer::kSFXSoundType, &_handle, this, -1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO);
}

SoundManager::SamplePlayer::Status SoundManager::SamplePlayer::advance(const bool loop) {
	_loop = loop;
	_playing = true;

	if (!_loop && _pos == _size) {
		_mixer.stopHandle(_handle);
		_playing = false;
		return kFinished;
	}

	return kPlaying;
}

void SoundManager::SamplePlayer::unload() {
	_mixer.stopHandle(_handle);
	_playing = false;
}

int SoundManager::SamplePlayer::readBuffer(int16 *buffer, const int numSamples) {
	if (!_playing) {
		return 0;
	}

	int samplesRead = 0;
	for (samplesRead = 0; samplesRead < numSamples; ++samplesRead) {
		if (_loop && _pos == _loopEnd) {
			_pos = _loopStart;
		}
		if (_pos == _size) {
			break;
		}

		*buffer++ = (_data[_pos++] << 8) ^ 0x8000;
	}
	return samplesRead;
}

#pragma mark -
#pragma mark Debugging

void SoundManager::debugPrintDriverState(Console &con) const {
	Common::StackLock lock(_mutex);
	_driver->debugPrintState(con);
}

} // end of namespace Sci
