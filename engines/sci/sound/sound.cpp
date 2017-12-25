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
#include "common/platform.h"
#include "common/system.h"
#include "sci/engine/features.h"
#include "sci/sound/sound.h"

namespace Sci {

SoundManager::SoundManager(ResourceManager &resMan, GameFeatures &features) :
	_resMan(resMan),
	_mixer(*g_system->getMixer()),
	_soundVersion(features.detectDoSoundType()) {

	uint32 deviceFlags;
#ifdef ENABLE_SCI32
	if (features.generalMidiOnly()) {
		deviceFlags = MDT_MIDI;
	} else {
#endif
		deviceFlags = MDT_PCSPK | MDT_PCJR | MDT_ADLIB | MDT_MIDI | MDT_CMS;
#ifdef ENABLE_SCI32
	}
#endif

	// Default to MIDI for Windows versions of SCI1.1 games, as their
	// soundtrack is written for GM.
	if (features.useAltWinGMSound())
		deviceFlags |= MDT_PREFER_GM;

	const Common::Platform platform = g_sci->getPlatform();

	if (platform == Common::kPlatformFMTowns) {
		if (getSciVersion() > SCI_VERSION_1_EARLY)
			deviceFlags = MDT_TOWNS;
		else
			deviceFlags |= MDT_TOWNS;
	}

	const uint32 dev = MidiDriver::detectDevice(deviceFlags);
	const MusicType musicType = MidiDriver::getMusicType(dev);

	if (features.useAltWinGMSound() && musicType != MT_GM) {
		warning("A Windows CD version with an alternate MIDI soundtrack has been chosen, "
				"but no MIDI music device has been selected. Reverting to the DOS soundtrack");
		features.forceDOSTracks();
#ifdef ENABLE_SCI32
	} else if (features.generalMidiOnly() && musicType != MT_GM) {
		warning("This game only supports General MIDI, but a non-GM device has "
				"been selected. Some music may be wrong or missing");
#endif
	}

	switch (musicType) {
	case MT_ADLIB:
		// FIXME: There's no Amiga sound option, so we hook it up to AdLib
		if (platform == Common::kPlatformAmiga || platform == Common::kPlatformMacintosh)
			error("TODO AmigaMac");
//			_driver.reset(MidiPlayer_AmigaMac_create(_soundVersion));
		else
			error("TODO AdLib");
//			_driver.reset(MidiPlayer_AdLib_create(_soundVersion));
		break;
	case MT_PCJR:
		error("TODO PCJr");
//		_driver.reset(MidiPlayer_PCJr_create(_soundVersion));
		break;
	case MT_PCSPK:
		error("TODO PCSpeaker");
//		_driver.reset(MidiPlayer_PCSpeaker_create(_soundVersion));
		break;
	case MT_CMS:
		error("TODO CMS");
//		_driver.reset(MidiPlayer_CMS_create(_soundVersion));
		break;
	case MT_TOWNS:
		error("TODO FM-Towns");
//		_driver.reset(MidiPlayer_FMTowns_create(_soundVersion));
		break;
	default:
		if (ConfMan.getBool("native_fb01")) {
			error("TODO FB-01");
//			_driver.reset(MidiPlayer_Fb01_create(_soundVersion));
		} else {
			error("TODO General MIDI");
//			_driver.reset(MidiPlayer_Midi_create(_soundVersion));
		}
	}

	if (!_driver) {
		if (g_sci->getGameId() == GID_FUNSEEKER ||
			(g_sci->getGameId() == GID_GK2 && g_sci->isDemo())) {
			// HACK: The Fun Seeker's Guide demo doesn't have patch 3 and the version
			// of the Adlib driver (adl.drv) that it includes is unsupported. That demo
			// doesn't have any sound anyway, so this shouldn't be fatal.
		} else {
			error("Failed to initialize sound driver");
		}
	}

	// TODO: 1. should be using sound version 2. probably wrong
	if (getSciVersion() >= SCI_VERSION_1_1) {
		dynamic_cast<Sci1SoundDriver &>(*_driver).setReverbMode(0);
	}
}

void SoundManager::saveLoadWithSerializer(Common::Serializer &s) {

}

} // end of namespace Sci
