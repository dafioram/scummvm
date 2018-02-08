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

#include "common/file.h"
#include "common/ptr.h"
#include "common/str.h"
#include "common/stream.h"
#include "sci/resource/sources/audiovolume.h"
#include "sci/resource/sources/extmap.h"
#include "sci/resource/sources/intmap.h"
#include "sci/resource/manager.h"

namespace Sci {

bool ExtMapResourceSource::scanSource(ResourceManager *resMan) {
	if (resMan->getMapVersion() < kResVersionSci1Late) {
		return (readResourceMapSCI0(resMan) == SCI_ERROR_NONE);
	} else {
		return (readResourceMapSCI1(resMan) == SCI_ERROR_NONE);
	}
}

ResourceErrorCode ExtMapResourceSource::readResourceMapSCI0(ResourceManager *resMan) const {
	Common::ScopedPtr<Common::SeekableReadStream> fileStream;
	ResourceType type = kResourceTypeInvalid;	// to silence a false positive in MSVC
	uint16 number, id;
	uint32 offset;

	if (_resourceFile) {
		fileStream.reset(_resourceFile->createReadStream());
		if (!fileStream)
			return SCI_ERROR_RESMAP_NOT_FOUND;
	} else {
		Common::File *file = new Common::File();
		if (!file->open(getLocationName())) {
			delete file;
			return SCI_ERROR_RESMAP_NOT_FOUND;
		}
		fileStream.reset(file);
	}

	fileStream->seek(0, SEEK_SET);

	ResVersion mapVersion = resMan->getMapVersion();

	byte bMask = (mapVersion >= kResVersionSci1Middle) ? 0xF0 : 0xFC;
	byte bShift = (mapVersion >= kResVersionSci1Middle) ? 28 : 26;

	do {
		// King's Quest 5 FM-Towns uses a 7 byte version of the SCI1 Middle map,
		// splitting the type from the id.
		if (resMan->getMapVersion() == kResVersionKQ5FMT)
			type = resMan->convertResType(fileStream->readByte());

		id = fileStream->readUint16LE();
		offset = fileStream->readUint32LE();

		if (fileStream->eos() || fileStream->err()) {
			warning("Error while reading %s", getLocationName().c_str());
			return SCI_ERROR_RESMAP_NOT_FOUND;
		}

		if (offset == 0xFFFFFFFF)
			break;

		if (resMan->getMapVersion() == kResVersionKQ5FMT) {
			number = id;
		} else {
			type = resMan->convertResType(id >> 11);
			number = id & 0x7FF;
		}

		ResourceId resId = ResourceId(type, number);
		// adding a new resource
		if (!resMan->testResource(resId)) {
			const ResourceSource *source = resMan->findVolumeForMap(this, offset >> bShift);
			if (!source) {
				warning("Could not get volume for resource %d, VolumeID %d", id, offset >> bShift);
				if (resMan->getMapVersion() != resMan->getVolVersion()) {
					warning("Retrying with the detected volume version instead");
					warning("Map version was: %d, retrying with: %d", resMan->getMapVersion(), resMan->getVolVersion());
					mapVersion = resMan->getVolVersion();
					bMask = (mapVersion == kResVersionSci1Middle) ? 0xF0 : 0xFC;
					bShift = (mapVersion == kResVersionSci1Middle) ? 28 : 26;
					source = resMan->findVolumeForMap(this, offset >> bShift);
					if (!source) {
						warning("Still couldn't find the volume");
						return SCI_ERROR_NO_RESOURCE_FILES_FOUND;
					}
				} else {
					return SCI_ERROR_NO_RESOURCE_FILES_FOUND;
				}
			}

			resMan->addResource(resId, source, offset & ((((byte)~bMask) << 24) | 0xFFFFFF), 0, getLocationName());
		}
	} while (!fileStream->eos());

	return SCI_ERROR_NONE;
}

ResourceErrorCode ExtMapResourceSource::readResourceMapSCI1(ResourceManager *resMan) const {
	Common::ScopedPtr<Common::SeekableReadStream> fileStream;

	if (_resourceFile) {
		fileStream.reset(_resourceFile->createReadStream());
		if (!fileStream)
			return SCI_ERROR_RESMAP_NOT_FOUND;
	} else {
		Common::File *file = new Common::File();
		if (!file->open(getLocationName())) {
			delete file;
			return SCI_ERROR_RESMAP_NOT_FOUND;
		}
		fileStream.reset(file);
	}

	ResourceIndex resMap[32] = {};
	byte type = 0, prevtype = 0;
	byte nEntrySize = resMan->getMapVersion() == kResVersionSci11 ? 5 : 6;
	ResourceId resId;

	// Read resource type and offsets to resource offsets block from .MAP file
	// The last entry has type=0xFF (0x1F) and offset equals to map file length
	do {
		type = fileStream->readByte() & 0x1F;
		resMap[type].offset = fileStream->readUint16LE();
		if (fileStream->eos()) {
			warning("Premature end of file %s", getLocationName().c_str());
			return SCI_ERROR_RESMAP_NOT_FOUND;
		}

		resMap[prevtype].size = (resMap[type].offset
		                          - resMap[prevtype].offset) / nEntrySize;
		prevtype = type;
	} while (type != 0x1F); // the last entry is FF

	const ResVersion mapVersion = resMan->getMapVersion();

	// reading each type's offsets
	uint32 fileOffset = 0;
	for (type = 0; type < 32; type++) {
		if (resMap[type].offset == 0) // this resource does not exist in map
			continue;
		fileStream->seek(resMap[type].offset);
		for (int i = 0; i < resMap[type].size; i++) {
			uint16 number = fileStream->readUint16LE();
			int volumeNo = 0;
			if (mapVersion == kResVersionSci11) {
				// offset stored in 3 bytes
				fileOffset = fileStream->readUint16LE();
				fileOffset |= fileStream->readByte() << 16;
				fileOffset <<= 1;
			} else {
				// offset/volume stored in 4 bytes
				fileOffset = fileStream->readUint32LE();
				if (resMan->getMapVersion() < kResVersionSci11) {
					volumeNo = fileOffset >> 28; // most significant 4 bits
					fileOffset &= 0x0FFFFFFF;     // least significant 28 bits
				} else {
					// in SCI32 it's a plain offset
				}
			}
			if (fileStream->eos() || fileStream->err()) {
				warning("Error while reading %s", getLocationName().c_str());
				return SCI_ERROR_RESMAP_NOT_FOUND;
			}
			resId = ResourceId(resMan->convertResType(type), number);
			// NOTE: We add the map's volume number here to the specified volume number
			// for SCI2.1 and SCI3 maps that are not resmap.000. The resmap.* files' numbers
			// need to be used in concurrence with the volume specified in the map to get
			// the actual resource file.
			int mapVolumeNr = volumeNo + _volumeNumber;
			const ResourceSource *source = resMan->findVolumeForMap(this, mapVolumeNr);

			if (!source) {
				warning("Could not get volume for resource %d, VolumeID %d", number, mapVolumeNr);
				return SCI_ERROR_NO_RESOURCE_FILES_FOUND;
			}

			const Resource *resource = resMan->testResource(resId);
			if (!resource) {
				resMan->addResource(resId, source, fileOffset, 0, getLocationName());
			} else {
				// If the resource is already present in a volume, change it to
				// the new content (but only in a volume, so as not to overwrite
				// external patches - refer to bug #3366295).
				// This is needed at least for the German version of Pharkas.
				// That version contains several duplicate resources INSIDE the
				// resource data files like fonts, views, scripts, etc. Thus,
				// if we use the first entries in the resource file, half of the
				// game will be English and umlauts will also be missing :P
				if (resource->getSourceType() == kSourceVolume) {
					resMan->updateResource(resId, source, fileOffset, 0, getLocationName());
				}
			}

#ifdef ENABLE_SCI32
			// Different CDs may have different audio maps on each disc. The
			// ResourceManager does not know how to deal with this; it expects
			// each resource ID to be unique across an entire game. To work
			// around this problem, all audio maps from this disc must be
			// processed immediately, since they will be replaced by the audio
			// map from the next disc on the next call to readResourceMapSCI1
			if (_shouldScanAudioMaps && resId.getType() == kResourceTypeMap) {
				resMan->scanMultiDiscAudioMap(source, mapVolumeNr, resId);
			}
#endif
		}
	}

	return SCI_ERROR_NONE;
}

} // End of namespace Sci
