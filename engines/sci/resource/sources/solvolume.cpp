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

#include "sci/resource/sources/solvolume.h"
#include "sci/resource/manager.h"

namespace Sci {

bool SolVolumeResourceSource::scanSource(ResourceManager *resMan) {
	Common::File volume;
	if (!volume.open(_name)) {
		warning("Could not open SOL volume %s", _name.c_str());
		return false;
	}

	for (;;) {
		char signature[3], filename[13];
		const uint32 entryPos = volume.pos();
		volume.read(signature, sizeof(signature));
		if (strncmp(signature, "S20", sizeof(signature)) != 0) {
			error("Invalid S2 file entry signature at %d", entryPos);
		}
		volume.skip(2); // unused
		volume.read(filename, sizeof(filename));
		if (strncmp(filename, "THE_END", sizeof(filename)) == 0) {
			break;
		}
		volume.skip(2); // padding
		const uint32 size = volume.readUint32LE();
		const uint32 offset = volume.readUint32LE();
		char *end = nullptr;
		const uint16 resourceNo = strtol(filename, &end, 10);

		ResourceId id;
		if (resourceNo && *end == '.') {
			id = ResourceId(getResourceTypeFromSuffix(end + 1), resourceNo);
		}

		if (id.getType() != kResourceTypeInvalid) {
			resMan->addResource(id, this, offset, size, _name);
		} else {
			warning("Invalid S2 file entry name %13s at %d", filename, entryPos);
			return false;
		}
	}

	return true;
}

void SolVolumeResourceSource::loadResource(const ResourceManager *resMan, Resource *res) const {
	loadRawData(resMan, res);
}

} // End of namespace Sci
