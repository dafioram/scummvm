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

#ifndef SCI_RESOURCE_SOURCES_EXTAUDIOMAP_H
#define SCI_RESOURCE_SOURCES_EXTAUDIOMAP_H

#include "sci/resource/resource.h"
#include "sci/resource/source.h"

namespace Sci {

class ExtAudioMapResourceSource final : public ResourceSource {
public:
	ExtAudioMapResourceSource(const Common::String &name, int volumeNo) :
		ResourceSource(kSourceExtAudioMap, name, volumeNo) {
	}

	virtual bool scanSource(ResourceManager *resMan) override;

	/**
	 * Adds or removes all audio resources from a SCI1 audio map file.
	 * @param map The map
	 * @param unload Unload the map instead of loading it
	 */
	ResourceErrorCode readAudioMapSCI1(ResourceManager *resMan, bool unload) const;
};

} // End of namespace Sci

#endif