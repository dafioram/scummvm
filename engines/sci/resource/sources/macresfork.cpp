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

#include "common/scummsys.h"
#include "sci/resource/manager.h"
#include "sci/resource/resource.h"
#include "sci/resource/sources/macresfork.h"

namespace Sci {

struct MacResTag {
	uint32 tag;
	ResourceType type;
};

static const MacResTag macResTagMap[] = {
	{ MKTAG('V','5','6',' '), kResourceTypeView },
	{ MKTAG('P','5','6',' '), kResourceTypePic },
	{ MKTAG('S','C','R',' '), kResourceTypeScript },
	{ MKTAG('T','E','X',' '), kResourceTypeText },
	{ MKTAG('S','N','D',' '), kResourceTypeSound },
	{ MKTAG('V','O','C',' '), kResourceTypeVocab },
	{ MKTAG('F','O','N',' '), kResourceTypeFont },
	{ MKTAG('C','U','R','S'), kResourceTypeCursor },
	{ MKTAG('c','r','s','r'), kResourceTypeCursor },
	{ MKTAG('P','a','t',' '), kResourceTypePatch },
	{ MKTAG('P','A','L',' '), kResourceTypePalette },
	{ MKTAG('s','n','d',' '), kResourceTypeAudio },
	{ MKTAG('M','S','G',' '), kResourceTypeMessage },
	{ MKTAG('H','E','P',' '), kResourceTypeHeap },
	{ MKTAG('I','B','I','N'), kResourceTypeMacIconBarPictN },
	{ MKTAG('I','B','I','S'), kResourceTypeMacIconBarPictS },
	{ MKTAG('P','I','C','T'), kResourceTypeMacPict },
	{ MKTAG('S','Y','N',' '), kResourceTypeSync },
	{ MKTAG('S','Y','N','C'), kResourceTypeSync }
};

static Common::Array<uint32> resTypeToMacTags(ResourceType type) {
	Common::Array<uint32> tags;

	for (uint32 i = 0; i < ARRAYSIZE(macResTagMap); i++)
		if (macResTagMap[i].type == type)
			tags.push_back(macResTagMap[i].tag);

	return tags;
}

MacResourceForkResourceSource::MacResourceForkResourceSource(const Common::String &name, int volNum)
 : ResourceSource(kSourceMacResourceFork, name, volNum) {
	_macResMan = new Common::MacResManager();
	assert(_macResMan);
}

MacResourceForkResourceSource::~MacResourceForkResourceSource() {
	delete _macResMan;
}

bool MacResourceForkResourceSource::scanSource(ResourceManager *resMan) {
	if (!_macResMan->open(getLocationName().c_str()))
		error("%s is not a valid Mac resource fork", getLocationName().c_str());

	Common::MacResTagArray tagArray = _macResMan->getResTagArray();

	for (uint32 i = 0; i < tagArray.size(); i++) {
		ResourceType type = kResourceTypeInvalid;

		// Map the Mac tags to our ResourceType
		for (uint32 j = 0; j < ARRAYSIZE(macResTagMap); j++)
			if (tagArray[i] == macResTagMap[j].tag) {
				type = macResTagMap[j].type;
				break;
			}

		if (type == kResourceTypeInvalid)
			continue;

		Common::MacResIDArray idArray = _macResMan->getResIDArray(tagArray[i]);

		for (uint32 j = 0; j < idArray.size(); j++) {
			ResourceId resId;

			// Check to see if we've got a base36 encoded resource name
			if (type == kResourceTypeAudio) {
				Common::String resourceName = _macResMan->getResName(tagArray[i], idArray[j]);

				// If we have a file name on an audio resource, we've got an audio36
				// resource. Parse the file name to get the id.
				if (!resourceName.empty() && resourceName[0] == '@')
					resId = convertPatchNameBase36(kResourceTypeAudio36, resourceName);
				else
					resId = ResourceId(type, idArray[j]);
			} else if (type == kResourceTypeSync) {
				Common::String resourceName = _macResMan->getResName(tagArray[i], idArray[j]);

				// Same as with audio36 above
				if (!resourceName.empty() && resourceName[0] == '#')
					resId = convertPatchNameBase36(kResourceTypeSync36, resourceName);
				else
					resId = ResourceId(type, idArray[j]);
			} else {
				// Otherwise, we're just going with the id that was given
				resId = ResourceId(type, idArray[j]);
			}

			// Overwrite Resource instance. Resource forks may contain patches.
			// The size will be filled in later by decompressResource()
			resMan->updateResource(resId, this, 0, getLocationName());
		}
	}

	return true;
}

void MacResourceForkResourceSource::loadResource(const ResourceManager *resMan, Resource *res) const {
	ResourceType type = res->getType();
	Common::SeekableReadStream *stream = 0;

	if (type == kResourceTypeAudio36 || type == kResourceTypeSync36) {
		// Handle audio36/sync36, convert back to audio/sync
		stream = _macResMan->getResource(res->getId().toPatchNameBase36());
	} else {
		// Plain resource handling
		Common::Array<uint32> tagArray = resTypeToMacTags(type);

		for (uint32 i = 0; i < tagArray.size() && !stream; i++)
			stream = _macResMan->getResource(tagArray[i], res->getNumber());
	}

	if (stream)
		decompressResource(stream, res);
}

bool MacResourceForkResourceSource::isCompressableResource(ResourceType type) const {
	// Any types that were not originally an SCI format are not compressed, it seems.
	// (Audio/36 being Mac snd resources here)
	return type != kResourceTypeMacPict && type != kResourceTypeAudio &&
			type != kResourceTypeMacIconBarPictN && type != kResourceTypeMacIconBarPictS &&
			type != kResourceTypeAudio36 && type != kResourceTypeSync &&
			type != kResourceTypeSync36 && type != kResourceTypeCursor;
}

#define OUTPUT_LITERAL() \
	assert(ptr + literalLength <= bufferEnd); \
	while (literalLength--) \
		*ptr++ = stream->readByte();

#define OUTPUT_COPY() \
	assert(ptr + copyLength <= bufferEnd); \
	while (copyLength--) { \
		byte value = ptr[-offset]; \
		*ptr++ = value; \
	}

void MacResourceForkResourceSource::decompressResource(Common::SeekableReadStream *stream, Resource *resource) const {
	// KQ6 Mac is the only game not compressed. It's not worth writing a
	// heuristic just for that game. Also, skip over any resource that cannot
	// be compressed.
	bool canBeCompressed = !(g_sci && g_sci->getGameId() == GID_KQ6) && isCompressableResource(resource->getId().getType());
	uint32 uncompressedSize = 0;

#ifdef ENABLE_SCI32_MAC
	// GK2 Mac is crazy. In its Patches resource fork, picture 2315 is not
	// compressed and it is hardcoded in the executable to say that it's
	// not compressed. Why didn't they just add four zeroes to the end of
	// the resource? (Checked with PPC disasm)
	if (g_sci && g_sci->getGameId() == GID_GK2 && resource->_id.getType() == kResourceTypePic && resource->_id.getNumber() == 2315)
		canBeCompressed = false;
#endif

	// Get the uncompressed size from the end of the resource
	if (canBeCompressed && stream->size() > 4) {
		stream->seek(-4, SEEK_END);
		uncompressedSize = stream->readUint32BE();
		stream->seek(0);
	}

	if (uncompressedSize == 0) {
		// Not compressed
		resource->_size = stream->size();

		// Cut out the 'non-compressed marker' (four zeroes) at the end
		if (canBeCompressed)
			resource->_size -= 4;

		byte *ptr = new byte[resource->size()];
		resource->_data = ptr;
		stream->read(ptr, resource->size());
	} else {
		// Decompress
		resource->_size = uncompressedSize;
		byte *ptr = new byte[uncompressedSize];
		resource->_data = ptr;

		const byte *const bufferEnd = resource->data() + uncompressedSize;

		while (stream->pos() < stream->size()) {
			byte code = stream->readByte();

			int literalLength = 0, offset = 0, copyLength = 0;
			byte extraByte1 = 0, extraByte2 = 0;

			if (code == 0xFF) {
				// End of stream marker
				break;
			}

			switch (code & 0xC0) {
			case 0x80:
				// Copy chunk expanded
				extraByte1 = stream->readByte();
				extraByte2 = stream->readByte();

				literalLength = extraByte2 & 3;

				OUTPUT_LITERAL()

				offset = ((code & 0x3f) | ((extraByte1 & 0xe0) << 1) | ((extraByte2 & 0xfc) << 7)) + 1;
				copyLength = (extraByte1 & 0x1f) + 3;

				OUTPUT_COPY()
				break;
			case 0xC0:
				// Literal chunk
				if (code >= 0xD0) {
					// These codes cannot be used
					if (code == 0xD0 || code > 0xD3)
						error("Bad Mac compression code %02x", code);

					literalLength = code & 3;
				} else
					literalLength = (code & 0xf) * 4 + 4;

				OUTPUT_LITERAL()
				break;
			default:
				// Copy chunk
				extraByte1 = stream->readByte();

				literalLength = (extraByte1 >> 3) & 0x3;

				OUTPUT_LITERAL()

				offset = (code + ((extraByte1 & 0xE0) << 2)) + 1;
				copyLength = (extraByte1 & 0x7) + 3;

				OUTPUT_COPY()
				break;
			}
		}
	}

	resource->_status = kResStatusAllocated;
	delete stream;
}

#undef OUTPUT_LITERAL
#undef OUTPUT_COPY

} // End of namespace Sci
