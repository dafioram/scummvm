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
#include "common/system.h"
#include "sci/event.h"
#include "sci/graphics/frameout.h"
#include "sci/s2/engine.h"
#include "sci/sound/audio32.h"
#include "sci/resource/manager.h"

namespace Sci {

S2Engine::S2Engine(OSystem &system, const GameMetadata &metadata) :
	Engine(&system),
	_system(system),
	_metadata(metadata),
	_resourceManager(metadata),
	_features(&_resourceManager, nullptr, nullptr),
	_eventManager(false, &_debugger, nullptr, nullptr),
	_timeManager(system, *this, _eventManager),
	_debugger() {
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
	_audioManager.reset(new Audio32(&_resourceManager, nullptr, &_features, &_timeManager));
	_graphicsManager.reset(new GfxFrameout(&_resourceManager, &_features, &_debugger, &_timeManager, &_eventManager, _audioManager.get(), nullptr));

	while (!shouldQuit()) {
		_timeManager.sleep(16);
	}

	return Common::kNoError;
}

} // End of namespace Sci
