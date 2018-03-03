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

#ifndef SCI_S2_DEBUGGER_H
#define SCI_S2_DEBUGGER_H

#include "sci/debugger.h"

namespace Sci {

class S2Game;
class S2Kernel;

class S2Debugger : public Debugger {
public:
	S2Debugger(S2Kernel &kernel, S2Game &game);

protected:
	virtual bool cmdHelp(int argc, const char **argv) override;

private:
	bool cmdBitmapInfo(int argc, const char **argv);
	bool cmdRoom(int argc, const char **argv);
	bool cmdFlag(int argc, const char **argv);
	bool cmdGive(int argc, const char **argv);

	S2Kernel &_kernel;
	S2Game &_game;
};

} // End of namespace Sci

#endif
