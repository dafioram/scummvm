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

#include "common/error.h"
#include "common/system.h"
#include "sci/s2/debugger.h"
#include "sci/s2/engine.h"
#include "sci/s2/game.h"
#include "sci/s2/kernel.h"

namespace Sci {

S2Engine::S2Engine(OSystem &system, const GameMetadata &metadata) :
	Engine(&system),
	_system(system),
	_metadata(metadata) {
	g_SciBE = false;
	g_Sci11BE = false;
	g_Sci32BE = false;
}

void S2Engine::initializePath(const Common::FSNode &gamePath) {
	Engine::initializePath(gamePath);
	SearchMan.addSubDirectoryMatching(gamePath, "data");
}

Common::Error S2Engine::run() {
	_kernel.reset(new S2Kernel(_system, *this, _metadata));
	_game.reset(new S2Game(*this, *_kernel));
	_debugger.reset(new S2Debugger(*_kernel, *_game));
	_game->run();
	return Common::kNoError;
}

GUI::Debugger *S2Engine::getDebugger() {
	return _debugger.get();
}

} // End of namespace Sci
