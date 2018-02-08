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

#include "sci/engine/features.h"
#include "sci/resource/manager.h"
#include "sci/event.h"
#include "sci/s2/debugger.h"
#include "sci/sci.h"
#include "sci/time.h"

class OSystem;

namespace Sci {

class Audio32;
class GfxFrameout;

class S2Engine : public Engine {
public:
	S2Engine(OSystem &system, const GameMetadata &metadata);
	virtual void initializePath(const Common::FSNode &gamePath) override;
	virtual Common::Error run() override;
	virtual GUI::Debugger *getDebugger() override { return &_debugger; }

private:
	OSystem &_system;
	GameMetadata _metadata;
	ResourceManager _resourceManager;
	GameFeatures _features;
	S2Debugger _debugger;
	EventManager _eventManager;
	TimeManager _timeManager;
	Common::ScopedPtr<Audio32> _audioManager;
	Common::ScopedPtr<GfxFrameout> _graphicsManager;
};

} // End of namespace Sci

#endif
