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

#include "common/debug.h"
#include "common/str.h"
#include "common/system.h"
#include "sci/event.h"
#include "sci/graphics/frameout.h"
#include "sci/s2/engine.h"
#include "sci/s2/system/glquit_handler.h"
#include "sci/s2/system/types.h"
#include "sci/sound/audio32.h"
#include "sci/resource/manager.h"

namespace Sci {

S2Engine::S2Engine(OSystem &system, const GameMetadata &metadata) :
	Engine(&system),
	// System
	_system(system),
	// Kernel
	_resourceManager(metadata),
	_features(&_resourceManager, nullptr, nullptr),
	_debugger(&_resourceManager),
	_eventManager(false, &_debugger, nullptr, nullptr),
	_timeManager(system, *this, _eventManager),
	// Game
	_flags(),
	_volume(127) {
	g_SciBE = false;
	g_Sci11BE = false;
	g_Sci32BE = false;
}

void S2Engine::initializePath(const Common::FSNode &gamePath) {
	Engine::initializePath(gamePath);
	SearchMan.addSubDirectoryMatching(gamePath, "data");
}

Common::Error S2Engine::run() {
	_resourceManager.run();
	_audioMixer.reset(new Audio32(&_resourceManager, nullptr, &_features, &_timeManager));
	_graphicsManager.reset(new GfxFrameout(&_resourceManager, &_features, &_debugger, &_timeManager, &_eventManager, _audioMixer.get(), nullptr));
	_debugger.run(_graphicsManager.get(), _audioMixer.get());

	init();
	play();

	// TODO: A save-before-quit confirmation dialogue was shown here; add this
	// if it feels like a good idea later

	return Common::kNoError;
}

Common::String S2Engine::getMessage(const uint16 resourceNo) const {
	const Resource *resource = _resourceManager.findResource(ResourceId(kResourceTypeText, resourceNo), false);
	if (resource) {
		return Common::String(reinterpret_cast<const char *>(resource->data()), resource->size());
	}
	return "";
}

void S2Engine::play() {
	GLQuitHandler quitHandler;

	_extras.push_front(&_soundManager);
	_user._primaDonnas.push_back(&quitHandler);

	_user.setIsHandsOn(true);

	while (!shouldQuit()) {
		doIt();
	}

	_user._primaDonnas.remove(&quitHandler);
	_extras.remove(&_soundManager);
}

void S2Engine::doIt() {
	_planes.doIt();
	for (auto &extra : _extras) {
		extra->doIt();
	}
	_graphicsManager->kernelFrameOut(true);
	_user.doIt();
}

void S2Engine::init() {
	_cursor.show();

	// SSCI had multiple heap allocations + instantiations here in this order:
	// * S2Cursor
	// * S2SndMgr (+ call to always preload audio data into memory)
	// * S2RoomMgr
	// * S2Interface (+ call to S2Interface::Init)
	// * MovieMgr (+ slow computer configuration)
	// * S2InvMgr
	// * S2ResourceMgr
	// * ScoringManager (+ call to ScoringManager::Init)
	// * FlagManager (+ call to FlagManager::Init)
	// * S2PhoneMgr (+ call to S2PhoneMgr::Init)
	// * Plus a bunch of initialisation code for save game catalogues
	// * Plus a few global initialisations
	//
	// Most of these have been eliminated simply by making these objects part of
	// S2Engine so they get initialised at the same time as the engine itself.
	// There are a couple of exceptions, though:
	//
	// * S2Cursor initialisation has been mostly moved into S2Cursor itself
	//   since it is only ever instantiated once, at engine startup
	// * S2SndMgr (actually GLSndManager)'s constructor added itself to the list
	//   of global extra objects and removed itself in the destructor; this is
	//   now handled in `S2Engine::play`
	// * S2ResourceMgr was used for loading panorama bitmaps; this functionality
	//   has been moved into the standard ResourceManager
	// * No slow computer configuration is used
	// * Save game management is done by ScummVM itself
	// * Globals not in the S2Engine class initializer list were unused, so have
	//   been omitted

	_audioMixer->setAttenuatedMixing(false);

	_interface.disableButtons();
	_roomManager.loadRoom(1000);
	_roomManager.initRoom(1000);
}

} // End of namespace Sci
