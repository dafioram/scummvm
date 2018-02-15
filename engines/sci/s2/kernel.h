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

#ifndef SCI_S2_KERNEL_H
#define SCI_S2_KERNEL_H

#include "sci/sound/audio32.h"
#include "sci/engine/features.h"
#include "sci/graphics/frameout.h"
#include "sci/resource/manager.h"
#include "sci/event.h"
#include "sci/sci.h"
#include "sci/time.h"
#include "sci/s2/bitmap_manager.h"

class OSystem;

namespace Sci {
class S2Kernel {
public:
	S2Kernel(OSystem &system, Engine &engine, const GameMetadata &metadata);

	// In SSCI, this was Win32 LoadMessage
	Common::String getMessage(const uint16 resourceNo) const;

	ResourceManager resourceManager;
	GameFeatures features;
	EventManager eventManager;
	TimeManager timeManager;
	Audio32 audioMixer;
	BitmapManager bitmapManager;
	GfxFrameout graphicsManager;
};
} // End of namespace Sci

#endif
