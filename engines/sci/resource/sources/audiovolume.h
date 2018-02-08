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

#ifndef SCI_RESOURCE_SOURCES_AUDIOVOLUME_H
#define SCI_RESOURCE_SOURCES_AUDIOVOLUME_H

#include "sci/resource/resource.h"
#include "sci/resource/sources/volume.h"

namespace Sci {

class AudioVolumeResourceSource final : public VolumeResourceSource {
public:
	AudioVolumeResourceSource(ResourceManager *resMan, const Common::String &name, ResourceSource *map, int volNum);

	virtual void loadResource(const ResourceManager *resMan, Resource *res) const override;

	virtual uint32 getAudioCompressionType() const override;

	bool relocateMapOffset(uint32 &offset, uint32 &size) const {
		if (_audioCompressionType == 0) {
			return true;
		}

		if (!_compressedOffsets.contains(offset)) {
			return false;
		}

		const CompressedTableEntry &entry = _compressedOffsets.getVal(offset);
		offset = entry.offset;
		size = entry.size;
		return true;
	}

private:
	struct CompressedTableEntry {
		uint32 offset;
		uint32 size;
	};

	uint32 _audioCompressionType;

	Common::HashMap<uint32, CompressedTableEntry> _compressedOffsets;

	bool loadFromAudioVolumeSCI1(const ResourceManager *resMan, Resource *resource, Common::SeekableReadStream *file) const;
	bool loadFromAudioVolumeSCI11(const ResourceManager *resMan, Resource *resource, Common::SeekableReadStream *file) const;
};

} // End of namespace Sci

#endif
