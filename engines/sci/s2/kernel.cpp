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

#include "common/str.h"
#include "common/system.h"
#include "sci/s2/debugger.h"
#include "sci/s2/dialog.h"
#include "sci/s2/kernel.h"
#include "sci/s2/message_box.h"
#include "sci/s2/system/glevent.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/glscreen_item.h"
#include "sci/s2/system/gltimer.h"

namespace Sci {

S2Kernel::S2Kernel(OSystem &system, Engine &engine, const GameMetadata &metadata) :
	resourceManager(metadata),
	features(&resourceManager),
	eventManager(false),
	timeManager(system, engine, eventManager),
	audioMixer(&resourceManager, &features, &timeManager),
	graphicsManager(&resourceManager, &features, &timeManager, &eventManager, &audioMixer, &bitmapManager) {
	GLEvent::init(&eventManager);
	AbsGLPlane::init(&graphicsManager);
	GLTimer::init(&timeManager);
	GLScreenItem::init(&graphicsManager);
	S2MessageBox::init(&graphicsManager._text);
	S2Dialog::init(&graphicsManager);
	S2Bitmap::init(&graphicsManager._bitmap);
}

Common::String S2Kernel::getMessage(const uint16 resourceNo) const {
	auto *resource = resourceManager.findResource(ResourceId(kResourceTypeText, resourceNo), false);
	if (resource) {
		return Common::String(reinterpret_cast<const char *>(resource->data()), resource->size());
	}
	return "";
}

} // End of namespace Sci
