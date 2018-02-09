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
#include "common/stream.h"
#include "sci/resource/manager.h"
#include "sci/resource/sources/audiovolume.h"
#include "sci/resource/sources/patch.h"
#include "sci/resource/sources/wave.h"

namespace Sci {

AudioVolumeResourceSource::AudioVolumeResourceSource(ResourceManager *resMan, const Common::String &name, ResourceSource *map, int volNum) :
	VolumeResourceSource(name, map, volNum, kSourceAudioVolume),
	_audioCompressionType(0) {

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


void AudioVolumeResourceSource::loadResource(const ResourceManager *resMan, Resource *res) const {
	Common::SeekableReadStream *fileStream = getVolumeFile(resMan, res);
	if (!fileStream)
		return;

	fileStream->seek(res->_fileOffset, SEEK_SET);

	// For compressed audio, using loadFromAudioVolumeSCI1 is a hack to bypass
	// the resource type checking in loadFromAudioVolumeSCI11 (since
	// loadFromAudioVolumeSCI1 does nothing more than read raw data)
	const ResourceType type = res->getId().getType();
	if (_audioCompressionType != 0 &&
		(type == kResourceTypeAudio || type == kResourceTypeAudio36)) {

		loadFromAudioVolumeSCI1(resMan, res, fileStream);
	} else if (getSciVersion() < SCI_VERSION_1_1) {
		loadFromAudioVolumeSCI1(resMan, res, fileStream);
	} else {
		loadFromAudioVolumeSCI11(resMan, res, fileStream);
	}

	resMan->disposeVolumeFileStream(fileStream, this);
}

uint32 AudioVolumeResourceSource::getAudioCompressionType() const {
	return _audioCompressionType;
}

bool AudioVolumeResourceSource::loadFromAudioVolumeSCI1(const ResourceManager *resMan, Resource *resource, Common::SeekableReadStream *file) const {
	byte *ptr = new byte[resource->_size];
	resource->_data = ptr;

	if (!ptr) {
		error("Can't allocate %u bytes needed for loading %s", resource->_size, resource->name().c_str());
	}

	uint32 bytesRead = file->read(ptr, resource->_size);
	if (bytesRead != resource->_size)
		warning("Read %d bytes from %s but expected %u", bytesRead, resource->name().c_str(), resource->_size);

	resource->_status = kResStatusAllocated;
	return true;
}

bool AudioVolumeResourceSource::loadFromAudioVolumeSCI11(const ResourceManager *resMan, Resource *resource, Common::SeekableReadStream *file) const {
	// Check for WAVE files here
	uint32 riffTag = file->readUint32BE();
	if (riffTag == MKTAG('R','I','F','F')) {
		resource->_size = file->readUint32LE() + 8;
		file->seek(-8, SEEK_CUR);
		return loadFromStream(file, resource);
	}
	file->seek(-4, SEEK_CUR);

	// Rave-resources (King's Quest 6) don't have any header at all
	const ResourceType resourceType = resource->getId().getType();
	if (resourceType != kResourceTypeRave) {
		ResourceType type = resMan->convertResType(file->readByte());

		if (((resourceType == kResourceTypeAudio || resourceType == kResourceTypeAudio36) && (type != kResourceTypeAudio))
			|| ((resourceType == kResourceTypeSync || resourceType == kResourceTypeSync36) && (type != kResourceTypeSync))) {
			warning("Resource type mismatch loading %s", resource->name().c_str());
			resource->unalloc();
			return false;
		}

		const uint8 headerSize = file->readByte();

		if (type == kResourceTypeAudio) {
			if (headerSize != 7 && headerSize != 11 && headerSize != 12) {
				warning("Unsupported audio header size %d in %s", headerSize, resource->name().c_str());
				resource->unalloc();
				return false;
			}

			if (headerSize != 7) { // Size is defined already from the map
				// Load sample size
				file->seek(7, SEEK_CUR);
				resource->_size = file->readUint32LE() + headerSize + kResourceHeaderSize;
				if (file->err() || file->eos()) {
					warning("Error while reading size of %s", resource->name().c_str());
					resource->unalloc();
					return false;
				}
				// Adjust offset to point at the beginning of the audio file
				// again
				file->seek(-11, SEEK_CUR);
			}

			// SOL audio files are designed to require the resource header
			file->seek(-2, SEEK_CUR);
		}
	}
	return PatchResourceSource::loadPatch(file, resource);
}

} // End of namespace Sci
