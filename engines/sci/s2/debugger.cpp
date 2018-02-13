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

#include "sci/sound/audio32.h"
#include "sci/graphics/frameout.h"
#include "sci/resource/manager.h"
#include "sci/s2/debugger.h"
#include "sci/s2/game.h"
#include "sci/s2/kernel.h"

namespace Sci {

S2Debugger::S2Debugger(S2Kernel &kernel, S2Game &game) :
	Debugger(&kernel.resourceManager, &kernel.graphicsManager, &kernel.audioMixer, nullptr) {

	registerCmd("go", WRAP_METHOD(S2Debugger, cmdExit));

	kernel.graphicsManager.attachDebugger(this);
}

bool S2Debugger::cmdHelp(int argc, const char **argv) {
	debugPrintf("Resources:\n");
	printResourcesHelp();
	debugPrintf("\n");
	debugPrintf("Music/SFX:\n");
	printAudioHelp();
	return true;
}

} // End of namespace Sci
