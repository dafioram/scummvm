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

#ifndef SCI_S2_ENGINE_H
#define SCI_S2_ENGINE_H

#include "common/error.h"
#include "common/ptr.h"
#include "engines/engine.h"
#include "sci/sci.h"

class OSystem;
namespace Common { class FSNode; }
namespace GUI { class Debugger; }

namespace Sci {

class S2Debugger;
class S2Kernel;
class S2Game;

class S2Engine : public Engine {
public:
	S2Engine(OSystem &system, const GameMetadata &metadata);
	virtual void initializePath(const Common::FSNode &gamePath) override;
	virtual Common::Error run() override;
	virtual GUI::Debugger *getDebugger() override;
	virtual bool hasFeature(const EngineFeature f) const override;
	virtual bool canSaveGameStateCurrently() override;
	virtual bool canLoadGameStateCurrently() override;

private:
	OSystem &_system;
	GameMetadata _metadata;
	Common::ScopedPtr<S2Debugger> _debugger;
	Common::ScopedPtr<S2Kernel> _kernel;
	Common::ScopedPtr<S2Game> _game;
};

} // End of namespace Sci

#endif
