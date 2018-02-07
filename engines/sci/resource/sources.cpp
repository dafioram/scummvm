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

#include "common/error.h"
#include "common/stream.h"
#include "sci/resource/manager.h"
#include "sci/resource/sources.h"

namespace Sci {

AudioVolumeResourceSource::AudioVolumeResourceSource(ResourceManager *resMan, const Common::String &name, ResourceSource *map, int volNum)
	: VolumeResourceSource(name, map, volNum, kSourceAudioVolume) {

	_audioCompressionType = 0;

	/*
	 * Check if this audio volume got compressed by our tool. If that is the
	 * case, set _audioCompressionType and read in the offset translation
	 * table for later usage.
	 */

	Common::SeekableReadStream *fileStream = getVolumeFile(resMan, nullptr);
	if (!fileStream)
		return;

	fileStream->seek(0, SEEK_SET);
	const uint32 compressionType = fileStream->readUint32BE();
	switch (compressionType) {
	case MKTAG('M','P','3',' '):
	case MKTAG('O','G','G',' '):
	case MKTAG('F','L','A','C'):
		_audioCompressionType = compressionType;
		const uint32 numEntries = fileStream->readUint32LE();
		if (!numEntries) {
			error("Compressed audio volume %s has no relocation table entries", name.c_str());
		}

		CompressedTableEntry *lastEntry = nullptr;
		for (uint i = 0; i < numEntries; ++i) {
			CompressedTableEntry nextEntry;
			const uint32 sourceOffset = fileStream->readUint32LE();
			nextEntry.offset = fileStream->readUint32LE();
			if (lastEntry != nullptr) {
				lastEntry->size = nextEntry.offset - lastEntry->offset;
			}

			_compressedOffsets.setVal(sourceOffset, nextEntry);
			lastEntry = &_compressedOffsets.getVal(sourceOffset);
		}

		lastEntry->size = fileStream->size() - lastEntry->offset;
	}

	resMan->disposeVolumeFileStream(fileStream, this);
}


void WaveResourceSource::loadResource(ResourceManager *resMan, Resource *res) {
	Common::SeekableReadStream *fileStream = getVolumeFile(resMan, res);
	if (!fileStream)
		return;

	fileStream->seek(res->_fileOffset, SEEK_SET);
	res->loadFromWaveFile(fileStream);
	resMan->disposeVolumeFileStream(fileStream, this);
}

void AudioVolumeResourceSource::loadResource(ResourceManager *resMan, Resource *res) {
	Common::SeekableReadStream *fileStream = getVolumeFile(resMan, res);
	if (!fileStream)
		return;

	fileStream->seek(res->_fileOffset, SEEK_SET);

	// For compressed audio, using loadFromAudioVolumeSCI1 is a hack to bypass
	// the resource type checking in loadFromAudioVolumeSCI11 (since
	// loadFromAudioVolumeSCI1 does nothing more than read raw data)
	if (_audioCompressionType != 0 &&
		(res->getType() == kResourceTypeAudio ||
		 res->getType() == kResourceTypeAudio36)) {
		res->loadFromAudioVolumeSCI1(fileStream);
	} else if (getSciVersion() < SCI_VERSION_1_1)
		res->loadFromAudioVolumeSCI1(fileStream);
	else
		res->loadFromAudioVolumeSCI11(fileStream);

	resMan->disposeVolumeFileStream(fileStream, this);
}

uint32 AudioVolumeResourceSource::getAudioCompressionType() const {
	return _audioCompressionType;
}

ResourceSource::ResourceSource(ResSourceType type, const Common::String &name, int volNum, const Common::FSNode *resFile)
 : _sourceType(type), _name(name), _volumeNumber(volNum), _resourceFile(resFile) {
	_scanned = false;
}

ResourceSource::~ResourceSource() {
}

MacResourceForkResourceSource::MacResourceForkResourceSource(const Common::String &name, int volNum)
 : ResourceSource(kSourceMacResourceFork, name, volNum) {
	_macResMan = new Common::MacResManager();
	assert(_macResMan);
}

MacResourceForkResourceSource::~MacResourceForkResourceSource() {
	delete _macResMan;
}

//-- resMan helper functions --

// Resource source list management


// Resource manager constructors and operations


void PatchResourceSource::loadResource(ResourceManager *resMan, Resource *res) {
	bool result = res->loadFromPatchFile();
	if (!result) {
		// TODO: We used to fallback to the "default" code here if loadFromPatchFile
		// failed, but I am not sure whether that is really appropriate.
		// In fact it looks like a bug to me, so I commented this out for now.
		//ResourceSource::loadResource(res);
	}
}

static Common::Array<uint32> resTypeToMacTags(ResourceType type);

void MacResourceForkResourceSource::loadResource(ResourceManager *resMan, Resource *res) {
	ResourceType type = res->getType();
	Common::SeekableReadStream *stream = 0;

	if (type == kResourceTypeAudio36 || type == kResourceTypeSync36) {
		// Handle audio36/sync36, convert back to audio/sync
		stream = _macResMan->getResource(res->_id.toPatchNameBase36());
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
	bool canBeCompressed = !(g_sci && g_sci->getGameId() == GID_KQ6) && isCompressableResource(resource->_id.getType());
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

Common::SeekableReadStream *ResourceSource::getVolumeFile(ResourceManager *resMan, Resource *res) {
	Common::SeekableReadStream *fileStream = resMan->getVolumeFile(this);

	if (!fileStream) {
		warning("Failed to open %s", getLocationName().c_str());
		resMan->_hasBadResources = true;
		if (res)
			res->unalloc();
	}

	return fileStream;
}

void ResourceSource::loadResource(ResourceManager *resMan, Resource *res) {
	Common::SeekableReadStream *fileStream = getVolumeFile(resMan, res);
	if (!fileStream)
		return;

	fileStream->seek(res->_fileOffset, SEEK_SET);

	ResourceErrorCode error = res->decompress(resMan->getVolVersion(), fileStream);
	if (error) {
		warning("Error %d occurred while reading %s from resource file %s: %s",
				error, res->_id.toString().c_str(), res->getResourceLocation().c_str(),
				getResourceErrorDescription(error));
		res->unalloc();
	}

	resMan->disposeVolumeFileStream(fileStream, this);
}

void DirectoryResourceSource::scanSource(ResourceManager *resMan) {
	resMan->readResourcePatches();

	// We can't use getSciVersion() at this point, thus using _volVersion
	if (resMan->_volVersion >= kResVersionSci11)	// SCI1.1+
		resMan->readResourcePatchesBase36();

	resMan->readWaveAudioPatches();
}

void ExtMapResourceSource::scanSource(ResourceManager *resMan) {
	if (resMan->_mapVersion < kResVersionSci1Late) {
		if (resMan->readResourceMapSCI0(this) != SCI_ERROR_NONE) {
			resMan->_hasBadResources = true;
		}
	} else {
		if (resMan->readResourceMapSCI1(this) != SCI_ERROR_NONE) {
			resMan->_hasBadResources = true;
		}
	}
}

void ExtAudioMapResourceSource::scanSource(ResourceManager *resMan) {
	if (resMan->readAudioMapSCI1(this) != SCI_ERROR_NONE) {
		resMan->_hasBadResources = true;
	}
}

void IntMapResourceSource::scanSource(ResourceManager *resMan) {
	if (resMan->readAudioMapSCI11(this) != SCI_ERROR_NONE) {
		resMan->_hasBadResources = true;
	}
}

#ifdef ENABLE_SCI32

// Chunk resources are resources that hold other resources. They are normally called
// when using the kLoadChunk SCI2.1 kernel function. However, for example, the Lighthouse
// SCI2.1 demo has a chunk but no scripts outside of the chunk.

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

void ChunkResourceSource::scanSource(ResourceManager *resMan) {
	Resource *chunk = resMan->findResource(ResourceId(kResourceTypeChunk, _number), false);

	if (!chunk)
		error("Trying to load non-existent chunk");

	const byte *ptr = chunk->data();
	uint32 firstOffset = 0;

	for (;;) {
		ResourceType type = resMan->convertResType(*ptr);
		uint16 number = READ_LE_UINT16(ptr + 1);
		ResourceId id(type, number);

		ResourceEntry entry;
		entry.offset = READ_LE_UINT32(ptr + 3);
		entry.length = READ_LE_UINT32(ptr + 7);

		_resMap[id] = entry;
		ptr += 11;

		debugC(kDebugLevelResMan, 2, "Found %s in chunk %d", id.toString().c_str(), _number);

		resMan->updateResource(id, this, entry.length, chunk->_source->getLocationName());

		// There's no end marker to the data table, but the first resource
		// begins directly after the entry table. So, when we hit the first
		// resource, we're at the end of the entry table.

		if (!firstOffset)
			firstOffset = entry.offset;

		if ((size_t)(ptr - chunk->data()) >= firstOffset)
			break;
	}
}

void ChunkResourceSource::loadResource(ResourceManager *resMan, Resource *res) {
	Resource *chunk = resMan->findResource(ResourceId(kResourceTypeChunk, _number), false);

	if (!_resMap.contains(res->_id))
		error("Trying to load non-existent resource %s from chunk %d", res->_id.toString().c_str(), _number);

	ResourceEntry entry = _resMap[res->_id];
	if (entry.offset + entry.length > chunk->size()) {
		error("Resource %s is too large to exist within chunk %d (%u + %u > %u)", res->_id.toString().c_str(), _number, entry.offset, entry.length, chunk->size());
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

#endif

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

void MacResourceForkResourceSource::scanSource(ResourceManager *resMan) {
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
}

} // End of namespace Sci
