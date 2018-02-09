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
#include "sci/resource/source.h"

namespace Sci {

ResourceSource::ResourceSource(ResSourceType type, const Common::String &name, int volNum, const Common::FSNode *resFile)
 : _sourceType(type), _name(name), _volumeNumber(volNum), _resourceFile(resFile) {
	_scanned = false;
}

Common::SeekableReadStream *ResourceSource::getVolumeFile(const ResourceManager *resMan, const Resource *res) const {
	Common::SeekableReadStream *fileStream = resMan->getVolumeFile(this);

	if (!fileStream) {
		warning("Failed to open %s", getLocationName().c_str());
		// TODO: trigger resman resource failure
		if (res) {
			// TODO: No const_cast
			const_cast<Resource *>(res)->unalloc();
		}
	}

	return fileStream;
}

void ResourceSource::loadResource(const ResourceManager *resMan, Resource *res) const {
	Common::SeekableReadStream *fileStream = getVolumeFile(resMan, res);
	if (!fileStream)
		return;

	fileStream->seek(res->_fileOffset, SEEK_SET);

	ResourceErrorCode error = decompress(resMan, res, fileStream);
	if (error) {
		warning("Error %d occurred while reading %s from resource file %s: %s",
				error, res->name().c_str(), res->getResourceLocation().c_str(),
				getResourceErrorDescription(error));
		res->unalloc();
	}

	resMan->disposeVolumeFileStream(fileStream, this);
}

ResourceErrorCode ResourceSource::decompress(const ResourceManager *resMan, Resource *res, Common::SeekableReadStream *file) const {
	ResourceErrorCode errorNum;

	ResourceManager::ResourceHeader header;
	errorNum = resMan->readResourceHeader(file, header);
	if (errorNum != SCI_ERROR_NONE)
		return errorNum;

	if (header.uncompressedSize > kMaxResourceSize) {
		return SCI_ERROR_RESOURCE_TOO_BIG;
	}

	assert(res->_id == ResourceId(header.type, header.resourceNo));

	// getting a decompressor
	Common::ScopedPtr<Decompressor> dec;
	switch (header.compression) {
	case kCompNone:
		dec.reset(new Decompressor);
		break;
	case kCompHuffman:
		dec.reset(new DecompressorHuffman);
		break;
	case kCompLZW:
	case kCompLZW1:
	case kCompLZW1View:
	case kCompLZW1Pic:
		dec.reset(new DecompressorLZW(header.compression));
		break;
	case kCompDCL:
		dec.reset(new DecompressorDCL);
		break;
#ifdef ENABLE_SCI32
	case kCompSTACpack:
		dec.reset(new DecompressorLZS);
		break;
#endif
	default:
		error("Resource %s: Compression method %d not supported", res->name().c_str(), header.compression);
	}

	res->_size = header.uncompressedSize;
	byte *ptr = new byte[res->_size];
	res->_data = ptr;
	res->_status = kResStatusAllocated;
	if (!ptr) {
		errorNum = SCI_ERROR_RESOURCE_TOO_BIG;
	} else if (dec->unpack(file, ptr, header.compressedSize, header.uncompressedSize) != SCI_ERROR_NONE) {
		errorNum = SCI_ERROR_DECOMPRESSION_ERROR;
	} else {
		errorNum = SCI_ERROR_NONE;
	}

	if (errorNum != SCI_ERROR_NONE) {
		res->unalloc();
	} else {
		// At least Lighthouse puts sound effects in RESSCI.00n/RESSCI.PAT
		// instead of using a RESOURCE.SFX
		if (res->getId().getType() == kResourceTypeAudio) {
			const uint8 headerSize = res->getUint8At(1);
			if (headerSize < 11) {
				error("Unexpected audio header size for %s: should be >= 11, but got %d", res->name().c_str(), headerSize);
			}
			const uint32 audioSize = res->getUint32LEAt(9);
			const uint32 calculatedTotalSize = audioSize + headerSize + kResourceHeaderSize;
			if (calculatedTotalSize != res->_size) {
				warning("Unexpected audio file size: the size of %s in %s is %d, but the volume says it should be %d", res->name().c_str(), getLocationName().c_str(), calculatedTotalSize, res->_size);
			}
			res->_size = MIN(res->_size - kResourceHeaderSize, headerSize + audioSize);
		}
	}

	return errorNum;
}

} // End of namespace Sci
