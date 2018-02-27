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
	Debugger(&kernel.resourceManager, &kernel.graphicsManager, &kernel.audioMixer, nullptr),
	_kernel(kernel),
	_game(game) {

	registerCmd("go", WRAP_METHOD(S2Debugger, cmdExit));
	registerCmd("bitmap_info", WRAP_METHOD(S2Debugger, cmdBitmapInfo));
	registerCmd("room", WRAP_METHOD(S2Debugger, cmdRoom));

	kernel.eventManager.attachDebugger(this);
	kernel.graphicsManager.attachDebugger(this);
}

bool S2Debugger::cmdHelp(int argc, const char **argv) {
	debugPrintf("Game:\n");
	debugPrintf("room - Get or set the current room\n");
	debugPrintf("Resources:\n");
	printResourcesHelp();
	debugPrintf("\nGraphics:\n");
	debugPrintf("bitmap_info - Get information about an in-memory bitmap\n");
	printGraphicsHelp();
	debugPrintf("\n");
	debugPrintf("Music/SFX:\n");
	printAudioHelp();
	return true;
}

bool S2Debugger::cmdBitmapInfo(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Display information about an in-memory bitmap.\n");
		debugPrintf("Usage: %s <address>\n", argv[0]);
		debugPrintf("Check the \"addresses\" command on how to use addresses\n");
		return true;
	}

	reg_t id = NULL_REG;

	if (parse_reg_t(nullptr, argv[1], &id, false)) {
		debugPrintf("Invalid address passed.\n");
		debugPrintf("Check the \"addresses\" command on how to use addresses\n");
		return true;
	}

	if (id.isNull()) {
		debugPrintf("Invalid address.\n");
		return true;
	}

	SciBitmap *bitmap = _kernel.bitmapManager.lookupBitmap(id);
	if (!bitmap) {
		debugPrintf("Not a valid bitmap.\n");
		return true;
	}

	debugPrintf("%s\n", bitmap->toString().c_str());
	return true;
}

bool S2Debugger::cmdRoom(int argc, const char **argv) {
	if (argc > 1) {
		int roomNo;
		if (!parseInteger(argv[1], roomNo)) {
			debugPrintf("Invalid room number '%s'\n", argv[1]);
		} else {
			_game.getRoomManager().setNextRoomNo(roomNo);
			debugPrintf("Room changed from %d to %d\n", _game.getRoomManager().getCurrentRoomNo(), roomNo);
		}
	} else {
		debugPrintf("Current room is %d\n", _game.getRoomManager().getCurrentRoomNo());
	}
	return true;
}

} // End of namespace Sci
