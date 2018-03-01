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

#include "sci/resource/manager.h"
#include "sci/resource/sources/patch.h"
#include "sci/sci.h"

namespace Sci {

bool PatchResourceSource::loadPatch(Common::SeekableReadStream *file, Resource *res) {
	// We assume that the resource type matches `type`
	// We also assume that the current file position is right at the actual data (behind resourceid/headersize byte)

	byte *data = new byte[res->size()];
	res->_data = data;

	byte *header;
	if (res->_headerSize > 0) {
		header = new byte[res->_headerSize];
		res->_header = header;
	}

	if (!data || (res->_headerSize > 0 && !res->_header)) {
		warning("Can't allocate %u bytes needed for loading %s", res->_size + res->_headerSize, res->name().c_str());
		res->unalloc();
		return false;
	}

	uint32 bytesRead;
	if (res->_headerSize > 0) {
		bytesRead = file->read(header, res->_headerSize);
		if (bytesRead != res->_headerSize) {
			warning("Read %d bytes from %s but expected %d", bytesRead, res->name().c_str(), res->_headerSize);
			res->unalloc();
			return false;
		}
	}

	bytesRead = file->read(data, res->_size);
	if (bytesRead != res->_size) {
		warning("Read %d bytes from %s but expected %u", bytesRead, res->name().c_str(), res->_size);
		res->unalloc();
		return false;
	}

	res->_status = kResStatusAllocated;
	return true;
}

void PatchResourceSource::loadResource(const ResourceManager *resMan, Resource *res) const {
	Common::File file;
	if (!file.open(getLocationName())) {
		warning("Failed to open patch file %s", getLocationName().c_str());
		res->unalloc();
		return;
	}
	file.seek(0, SEEK_SET);
	loadPatch(&file, res);
}

bool PatchResourceSource::processPatch(ResourceManager *resMan, ResourceType resourceType, uint16 resourceNr, uint32 tuple) const {
	Common::ScopedPtr<Common::SeekableReadStream> fileStream;
	Resource *newrsc = nullptr;
	ResourceId resId = ResourceId(resourceType, resourceNr, tuple);
	ResourceType checkForType = resourceType;

	if (isBlacklistedPatch(resMan, resId)) {
		debug("Skipping blacklisted patch file %s", getLocationName().c_str());
		return false;
	}

	// base36 encoded patches (i.e. audio36 and sync36) have the same type as their non-base36 encoded counterparts
	if (checkForType == kResourceTypeAudio36)
		checkForType = kResourceTypeAudio;
	else if (checkForType == kResourceTypeSync36)
		checkForType = kResourceTypeSync;

	if (_resourceFile) {
		fileStream.reset(_resourceFile->createReadStream());
	} else {
		Common::File *file = new Common::File();
		if (!file->open(getLocationName())) {
			warning("ResourceManager::processPatch(): failed to open %s", getLocationName().c_str());
			delete file;
			return false;
		}
		fileStream.reset(file);
	}

	int fsize = fileStream->size();
	if (fsize < 3) {
		debug("Patching %s failed - file too small", getLocationName().c_str());
		return false;
	}

	byte patchType;
#ifdef ENABLE_SCI32S2
	if (fileStream->readUint16BE() == MKTAG16('B','M')) {
		fileStream->seek(-2, SEEK_CUR);
		patchType = kResourceTypePano;
	} else {
		fileStream->seek(-2, SEEK_CUR);
#endif
		if (fileStream->readUint32BE() == MKTAG('R','I','F','F')) {
			fileStream->seek(-4, SEEK_CUR);
			patchType = kResourceTypeAudio;
		} else {
			fileStream->seek(-4, SEEK_CUR);
			patchType = resMan->convertResType(fileStream->readByte());
		}
#ifdef ENABLE_SCI32S2
	}
#endif

	enum {
		kExtraHeaderSize    = 2, ///< extra header used in gfx resources
		kViewHeaderSize     = 22 ///< extra header used in view resources
	};

	int32 patchDataOffset = kResourceHeaderSize;
	const ResVersion volVersion = resMan->getVolVersion();
	if (volVersion < kResVersionSci11) {
		patchDataOffset += fileStream->readByte();
	} else {
		switch (patchType) {
		case kResourceTypeView:
			fileStream->seek(3, SEEK_SET);
			patchDataOffset += fileStream->readByte() + kViewHeaderSize + kExtraHeaderSize;
			break;
		case kResourceTypePic:
			if (volVersion < kResVersionSci2) {
				fileStream->seek(3, SEEK_SET);
				patchDataOffset += fileStream->readByte() + kViewHeaderSize + kExtraHeaderSize;
			} else {
				patchDataOffset += kExtraHeaderSize;
			}
			break;
		case kResourceTypePalette:
			fileStream->seek(3, SEEK_SET);
			patchDataOffset += fileStream->readByte() + kExtraHeaderSize;
			break;
		case kResourceTypeAudio:
		case kResourceTypeAudio36:
#ifdef ENABLE_SCI32
		case kResourceTypeWave:
		case kResourceTypeVMD:
		case kResourceTypeDuck:
		case kResourceTypeClut:
		case kResourceTypeTGA:
		case kResourceTypeZZZ:
		case kResourceTypeEtc:
#ifdef ENABLE_SCI32S2
		case kResourceTypePano:
#endif
#endif
			patchDataOffset = 0;
			break;
		default:
			fileStream->seek(1, SEEK_SET);
			patchDataOffset += fileStream->readByte();
			break;
		}
	}

	if (patchType != checkForType) {
		debug("Patching %s failed - resource type mismatch", getLocationName().c_str());
		return false;
	}

	if (patchDataOffset >= fsize) {
		debug("Patching %s failed - patch starting at offset %d can't be in file of size %d",
		      getLocationName().c_str(), patchDataOffset, fsize);
		return false;
	}

	// FIXME: This is weird and non-standard, Resource becomes the owner of this
	// PatchResourceSource instead of ResourceManager, even though every other
	// type of resource source (including Wave patches!) becomes owned by
	// ResourceManager
	newrsc = resMan->updateResource(resId, this, 0, fsize - patchDataOffset, getLocationName());
	newrsc->_headerSize = patchDataOffset;

	debugC(1, kDebugLevelResMan, "Patching %s - OK", getLocationName().c_str());
	return true;
}

bool PatchResourceSource::isBlacklistedPatch(const ResourceManager *resMan, const ResourceId &resId) {
	const GameMetadata &game = resMan->getGameMetadata();
	switch (game.id) {
	case GID_SHIVERS:
		// The SFX resource map patch in the Shivers interactive demo has
		// broken offsets for some sounds; ignore it so that the correct map
		// from RESSCI.000 will be used instead.
		return game.isDemo &&
			resId.getType() == kResourceTypeMap &&
			resId.getNumber() == kSfxModule;
	case GID_PHANTASMAGORIA:
		// The GOG release of Phantasmagoria 1 merges all resources into a
		// single-disc bundle, but they also include the 65535.MAP from the
		// original game's CD 1, which does not contain the entries for sound
		// effects from later CDs. So, just ignore this map patch since the
		// correct maps will be found in the RESSCI.000 file. This also helps
		// eliminate user error when copying files from the original CDs, since
		// each CD had a different 65535.MAP patch file.
		return resId.getType() == kResourceTypeMap && resId.getNumber() == kSfxModule;
	default:
		return false;
	}
}

} // End of namespace Sci
