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

#ifndef SCI_RESOURCE_SOURCES_EXTMAP_H
#define SCI_RESOURCE_SOURCES_EXTMAP_H

#include "sci/resource/resource.h"
#include "sci/resource/source.h"

namespace Common {
class FSNode;
class String;
}

namespace Sci {

class ResourceManager;

class ExtMapResourceSource final : public ResourceSource {
public:
	ExtMapResourceSource(const Common::String &name, int volNum, const bool shouldScanAudioMaps, const Common::FSNode *resFile = nullptr) :
		ResourceSource(kSourceExtMap, name, volNum, resFile),
		_shouldScanAudioMaps(shouldScanAudioMaps) {}

	virtual bool scanSource(ResourceManager *resMan) override;

private:
	struct ResourceIndex {
		uint16 offset;
		uint16 size;
	};

	/**
	 * Adds all resources from a SCI0 resource.map file.
	 * @param map The map
	 */
	ResourceErrorCode readResourceMapSCI0(ResourceManager *resMan) const;

	/**
	 * Adds all resources from a SCI1+ resource.map file.
	 * @param map The map
	 */
	ResourceErrorCode readResourceMapSCI1(ResourceManager *resMan) const;

	bool _shouldScanAudioMaps;
};

} // End of namespace Sci

#endif
