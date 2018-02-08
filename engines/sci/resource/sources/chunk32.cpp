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

#include "sci/resource/sources/chunk32.h"
#include "sci/resource/manager.h"

namespace Sci {

// Chunk resources are resources that hold other resources. They are never used
// except in one Lighthouse demo.

// A chunk resource is pretty straightforward in terms of layout
// It begins with 11-byte entries in the header:
// =========
// b resType
// w nEntry
// dw offset
// dw length

ChunkResourceSource::ChunkResourceSource(const Common::String &name, uint16 number)
	: ResourceSource(kSourceChunk, name) {
	_number = number;
}

bool ChunkResourceSource::scanSource(ResourceManager *resMan) {
	const Resource *chunk = resMan->findResource(ResourceId(kResourceTypeChunk, _number), false);

	if (!chunk) {
		warning("Trying to load non-existent chunk %d", _number);
		return false;
	}

	SciSpan<const byte> ptr = *chunk;
	uint32 firstOffset = 0;

	for (;;) {
		ResourceType type = resMan->convertResType(*ptr);
		uint16 number = ptr.getUint16LEAt(1);
		ResourceId id(type, number);

		ResourceEntry entry;
		entry.offset = ptr.getUint32LEAt(3);
		entry.length = ptr.getUint32LEAt(7);

		_resMap[id] = entry;
		ptr += 11;

		debugC(kDebugLevelResMan, 2, "Found %s in chunk %d", id.toString().c_str(), _number);

		resMan->updateResource(id, this, entry.length, chunk->name());

		// There's no end marker to the data table, but the first resource
		// begins directly after the entry table. So, when we hit the first
		// resource, we're at the end of the entry table.

		if (!firstOffset)
			firstOffset = entry.offset;

		if (uint32(ptr - *chunk) >= firstOffset)
			break;
	}

	return true;
}

void ChunkResourceSource::loadResource(const ResourceManager *resMan, Resource *res) const {
	const Resource *chunk = resMan->findResource(ResourceId(kResourceTypeChunk, _number), false);

	if (!_resMap.contains(res->getId()))
		error("Trying to load non-existent resource %s from chunk %d", res->name().c_str(), _number);

	ResourceEntry entry = _resMap[res->getId()];
	if (entry.offset + entry.length > chunk->size()) {
		error("Resource %s is too large to exist within chunk %d (%u + %u > %u)", res->name().c_str(), _number, entry.offset, entry.length, chunk->size());
	}
	byte *ptr = new byte[entry.length];
	res->_data = ptr;
	res->_size = entry.length;
	res->_header = 0;
	res->_headerSize = 0;
	res->_status = kResStatusAllocated;

	// Copy the resource data over
	memcpy(ptr, chunk->data() + entry.offset, entry.length);
}

} // End of namespace Sci
