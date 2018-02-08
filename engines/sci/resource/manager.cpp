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

#include "common/translation.h"
#include "sci/engine/script.h"
#include "sci/parser/vocabulary.h"
#include "sci/resource/manager.h"
#include "sci/resource/patcher.h"
#include "sci/resource/resource.h"
#include "sci/resource/source.h"
#include "sci/resource/sources/audiovolume.h"
#include "sci/resource/sources/directory.h"
#include "sci/resource/sources/extaudiomap.h"
#include "sci/resource/sources/extmap.h"
#include "sci/resource/sources/intmap.h"
#include "sci/resource/sources/macresfork.h"
#include "sci/resource/sources/patch.h"
#include "sci/resource/sources/volume.h"
#include "sci/resource/sources/wave.h"
#ifdef ENABLE_SCI32
#include "sci/resource/sources/chunk32.h"
#endif

namespace Sci {

//////////////////////////////////////////////////////////////////////

static SciVersion s_sciVersion = SCI_VERSION_NONE;	// FIXME: Move this inside a suitable class, e.g. SciEngine

SciVersion getSciVersion() {
	assert(s_sciVersion != SCI_VERSION_NONE);
	return s_sciVersion;
}

SciVersion getSciVersionForDetection() {
	assert(!g_sci);
	return s_sciVersion;
}

//////////////////////////////////////////////////////////////////////

static const ResourceType s_resTypeMapSci0[] = {
	kResourceTypeView, kResourceTypePic, kResourceTypeScript, kResourceTypeText,          // 0x00-0x03
	kResourceTypeSound, kResourceTypeMemory, kResourceTypeVocab, kResourceTypeFont,       // 0x04-0x07
	kResourceTypeCursor, kResourceTypePatch, kResourceTypeBitmap, kResourceTypePalette,   // 0x08-0x0B
	kResourceTypeCdAudio, kResourceTypeAudio, kResourceTypeSync, kResourceTypeMessage,    // 0x0C-0x0F
	kResourceTypeMap, kResourceTypeHeap, kResourceTypeAudio36, kResourceTypeSync36,       // 0x10-0x13
	kResourceTypeTranslation, kResourceTypeRave                                           // 0x14
};

// TODO: 12 should be "Wave", but SCI seems to just store it in Audio resources
static const ResourceType s_resTypeMapSci21[] = {
	kResourceTypeView, kResourceTypePic, kResourceTypeScript, kResourceTypeAnimation,     // 0x00-0x03
	kResourceTypeSound, kResourceTypeEtc, kResourceTypeVocab, kResourceTypeFont,          // 0x04-0x07
	kResourceTypeCursor, kResourceTypePatch, kResourceTypeBitmap, kResourceTypePalette,   // 0x08-0x0B
	kResourceTypeAudio, kResourceTypeAudio, kResourceTypeSync, kResourceTypeMessage,      // 0x0C-0x0F
	kResourceTypeMap, kResourceTypeHeap, kResourceTypeChunk, kResourceTypeAudio36,        // 0x10-0x13
	kResourceTypeSync36, kResourceTypeTranslation, kResourceTypeRobot, kResourceTypeVMD,  // 0x14-0x17
	kResourceTypeDuck, kResourceTypeClut, kResourceTypeTGA, kResourceTypeZZZ              // 0x18-0x1B
};

ResourceId convertPatchNameBase36(ResourceType type, const Common::String &filename) {
	// The base36 encoded resource contains the following:
	// uint16 resourceId, byte noun, byte verb, byte cond, byte seq

	// Skip patch type character
	uint16 resourceNr = strtol(Common::String(filename.c_str() + 1, 3).c_str(), 0, 36); // 3 characters
	uint16 noun = strtol(Common::String(filename.c_str() + 4, 2).c_str(), 0, 36);       // 2 characters
	uint16 verb = strtol(Common::String(filename.c_str() + 6, 2).c_str(), 0, 36);       // 2 characters
	// Skip '.'
	uint16 cond = strtol(Common::String(filename.c_str() + 9, 2).c_str(), 0, 36);       // 2 characters
	uint16 seq = strtol(Common::String(filename.c_str() + 11, 1).c_str(), 0, 36);       // 1 character

	return ResourceId(type, resourceNr, noun, verb, cond, seq);
}

ResourceManager::ResourceManager(const bool detectionMode) :
	_detectionMode(detectionMode), _patcher(nullptr) {}

void ResourceManager::init() {
	_maxMemoryLRU = 256 * 1024; // 256KiB
	_memoryLocked = 0;
	_memoryLRU = 0;
	_LRU.clear();
	_resMap.clear();
	_audioMapSCI1 = nullptr;
#ifdef ENABLE_SCI32
	_currentDiscNo = 1;
#endif

	// FIXME: put this in an Init() function, so that we can error out if detection fails completely

	_mapVersion = detectMapVersion();
	_volVersion = detectVolVersion();

	if ((_volVersion == kResVersionUnknown) && (_mapVersion != kResVersionUnknown)) {
		warning("Volume version not detected, but map version has been detected. Setting volume version to map version");
		_volVersion = _mapVersion;
	}

	if ((_mapVersion == kResVersionUnknown) && (_volVersion != kResVersionUnknown)) {
		warning("Map version not detected, but volume version has been detected. Setting map version to volume version");
		_mapVersion = _volVersion;
	}

	debugC(1, kDebugLevelResMan, "resMan: Detected resource map version %d: %s", _mapVersion, versionDescription(_mapVersion));
	debugC(1, kDebugLevelResMan, "resMan: Detected volume version %d: %s", _volVersion, versionDescription(_volVersion));

	if ((_mapVersion == kResVersionUnknown) && (_volVersion == kResVersionUnknown)) {
		warning("Volume and map version not detected, assuming that this is not a SCI game");
		_viewType = kViewUnknown;
		return;
	}

	scanNewSources();

	addAudioSources();
#ifdef ENABLE_SCI32
	addScriptChunkSource();
#endif
	scanNewSources();

	detectSciVersion();

	debugC(1, kDebugLevelResMan, "resMan: Detected %s", getSciVersionDesc(getSciVersion()));

	// Resources in SCI32 games are significantly larger than SCI16
	// games and can cause immediate exhaustion of the LRU resource
	// cache, leading to constant decompression of picture resources
	// and making the renderer very slow.
	if (getSciVersion() >= SCI_VERSION_2) {
		_maxMemoryLRU = 4096 * 1024; // 4MiB
	}

	switch (_viewType) {
	case kViewEga:
		debugC(1, kDebugLevelResMan, "resMan: Detected EGA graphic resources");
		break;
	case kViewAmiga:
		debugC(1, kDebugLevelResMan, "resMan: Detected Amiga ECS graphic resources");
		break;
	case kViewAmiga64:
		debugC(1, kDebugLevelResMan, "resMan: Detected Amiga AGA graphic resources");
		break;
	case kViewVga:
		debugC(1, kDebugLevelResMan, "resMan: Detected VGA graphic resources");
		break;
	case kViewVga11:
		debugC(1, kDebugLevelResMan, "resMan: Detected SCI1.1 VGA graphic resources");
		break;
	default:
		// Throw a warning, but do not error out here, because this is called from the
		// fallback detector, and the user could be pointing to a folder with a non-SCI
		// game, but with SCI-like file names (e.g. Pinball Creep)
		warning("resMan: Couldn't determine view type");
		break;
	}

	if (!_detectionMode && getSciVersion() < SCI_VERSION_2) {
		addNewGMPatch();
	}
}

ResourceManager::~ResourceManager() {
	for (ResourceMap::iterator it = _resMap.begin(); it != _resMap.end(); ++it) {
		delete it->_value;
	}

	Common::for_each(_sources.begin(), _sources.end(), Common::DefaultDeleter<ResourceSource>());

	Common::for_each(_volumeFiles.begin(), _volumeFiles.end(), Common::DefaultDeleter<Common::File>());
}

void ResourceManager::removeFromLRU(const Resource *res) const {
	if (res->_status != kResStatusEnqueued) {
		warning("resMan: trying to remove resource that isn't enqueued");
		return;
	}
	_LRU.remove(res);
	_memoryLRU -= res->size();
	res->_status = kResStatusAllocated;
}

void ResourceManager::addToLRU(const Resource *res) const {
	if (res->_status != kResStatusAllocated) {
		warning("resMan: trying to enqueue resource with state %d", res->_status);
		return;
	}
	_LRU.push_front(res);
	_memoryLRU += res->size();
#if SCI_VERBOSE_RESMAN
	debug("Adding %s (%d bytes) to lru control: %d bytes total",
	      res->_id.toString().c_str(), res->size,
	      _memoryLRU);
#endif
	res->_status = kResStatusEnqueued;
}

void ResourceManager::printLRU() const {
	uint mem = 0;
	uint entries = 0;

	for (LRUList::const_iterator it = _LRU.begin(); it != _LRU.end(); ++it) {
		const Resource *res = *it;
		debug("\t%s: %u bytes", res->_id.toString().c_str(), res->size());
		mem += res->size();
		++entries;
	}

	debug("Total: %d entries, %d bytes (mgr says %d)", entries, mem, _memoryLRU);
}

void ResourceManager::freeOldResources() const {
	while (_maxMemoryLRU < _memoryLRU) {
		assert(!_LRU.empty());
		const Resource *goner = _LRU.back();
		removeFromLRU(goner);
		const_cast<Resource *>(goner)->unalloc();
#ifdef SCI_VERBOSE_RESMAN
		debug("resMan-debug: LRU: Freeing %s (%d bytes)", goner->_id.toString().c_str(), goner->size);
#endif
	}
}

Common::List<ResourceId> ResourceManager::listResources(ResourceType type, int mapNumber) {
	Common::List<ResourceId> resources;

	ResourceMap::iterator itr = _resMap.begin();
	while (itr != _resMap.end()) {
		const ResourceId id = itr->_value->getId();
		if ((id.getType() == type) && ((mapNumber == -1) || (id.getNumber() == mapNumber)))
			resources.push_back(itr->_value->_id);
		++itr;
	}

	return resources;
}

ResourceErrorCode ResourceManager::readResourceHeader(Common::SeekableReadStream *file, ResourceHeader &outInfo) const {
	// SCI0 volume format:  {wResId wPacked+4 wUnpacked wCompression} = 8 bytes
	// SCI1 volume format:  {bResType wResNumber wPacked+4 wUnpacked wCompression} = 9 bytes
	// SCI1.1 volume format:  {bResType wResNumber wPacked wUnpacked wCompression} = 9 bytes
	// SCI32 volume format:  {bResType wResNumber dwPacked dwUnpacked wCompression} = 13 bytes
	uint16 compression;

	if (file->size() == 0)
		return SCI_ERROR_EMPTY_RESOURCE;

	switch (_volVersion) {
	case kResVersionSci0Sci1Early:
	case kResVersionSci1Middle: {
		const uint16 resourceId = file->readUint16LE();
		outInfo.type = convertResType(resourceId >> 11);
		outInfo.resourceNo = resourceId & 0x7FF;
		outInfo.compressedSize = file->readUint16LE() - 4;
		outInfo.uncompressedSize = file->readUint16LE();
		compression = file->readUint16LE();
		break;
	}
	case kResVersionSci1Late:
		outInfo.type = convertResType(file->readByte());
		outInfo.resourceNo = file->readUint16LE();
		outInfo.compressedSize = file->readUint16LE() - 4;
		outInfo.uncompressedSize = file->readUint16LE();
		compression = file->readUint16LE();
		break;
	case kResVersionSci11:
		outInfo.type = convertResType(file->readByte());
		outInfo.resourceNo = file->readUint16LE();
		outInfo.compressedSize = file->readUint16LE();
		outInfo.uncompressedSize = file->readUint16LE();
		compression = file->readUint16LE();
		break;
#ifdef ENABLE_SCI32
	case kResVersionSci2:
	case kResVersionSci3:
		outInfo.type = convertResType(file->readByte());
		outInfo.resourceNo = file->readUint16LE();
		outInfo.compressedSize = file->readUint32LE();
		outInfo.uncompressedSize = file->readUint32LE();
		compression = file->readUint16LE();

		// The same comment applies here as in detectVolVersion regarding SCI3.
		// We ignore the compression field for SCI3 games, but must presume it
		// exists in the file.
		if (_volVersion == kResVersionSci3) {
			compression = outInfo.compressedSize != outInfo.uncompressedSize ? 32 : 0;
		}

		break;
#endif
	default:
		return SCI_ERROR_RESMAP_INVALID_ENTRY;
	}

	if (file->eos() || file->err()) {
		return SCI_ERROR_IO_ERROR;
	}

	switch (compression) {
	case 0:
		outInfo.compression = kCompNone;
		break;
	case 1:
		outInfo.compression = (getSciVersion() <= SCI_VERSION_01) ? kCompLZW : kCompHuffman;
		break;
	case 2:
		outInfo.compression = (getSciVersion() <= SCI_VERSION_01) ? kCompHuffman : kCompLZW1;
		break;
	case 3:
		outInfo.compression = kCompLZW1View;
		break;
	case 4:
		outInfo.compression = kCompLZW1Pic;
		break;
	case 18:
	case 19:
	case 20:
		outInfo.compression = kCompDCL;
		break;
#ifdef ENABLE_SCI32
	case 32:
		outInfo.compression = kCompSTACpack;
		break;
#endif
	default:
		outInfo.compression = kCompUnknown;
		return SCI_ERROR_UNKNOWN_COMPRESSION;
	}

	return SCI_ERROR_NONE;
}

const Resource *ResourceManager::findResource(ResourceId id, bool lock) const {
	Resource *retval = _resMap.getVal(id);

	if (!retval)
		return nullptr;

	if (retval->_status == kResStatusNoMalloc)
		loadResource(retval);
	else if (retval->_status == kResStatusEnqueued)
		// The resource is removed from its current position
		// in the LRU list because it has been requested
		// again. Below, it will either be locked, or it
		// will be added back to the LRU list at the 'most
		// recent' position.
		removeFromLRU(retval);

	// Unless an error occurred, the resource is now either
	// locked or allocated, but never queued or freed.

	freeOldResources();

	if (lock) {
		if (retval->_status == kResStatusAllocated) {
			retval->_status = kResStatusLocked;
			retval->_lockers = 0;
			_memoryLocked += retval->_size;
		}
		retval->_lockers++;
	} else if (retval->_status != kResStatusLocked) { // Don't lock it
		if (retval->_status == kResStatusAllocated)
			addToLRU(retval);
	}

	if (retval->data())
		return retval;
	else {
		warning("resMan: Failed to read %s", retval->_id.toString().c_str());
		return nullptr;
	}
}

void ResourceManager::unlockResource(const Resource *res) const {
	assert(res);

	if (res->_status != kResStatusLocked) {
		debugC(kDebugLevelResMan, 2, "[resMan] Attempt to unlock unlocked resource %s", res->_id.toString().c_str());
		return;
	}

	if (!--res->_lockers) { // No more lockers?
		res->_status = kResStatusAllocated;
		_memoryLocked -= res->size();
		addToLRU(res);
	}

	freeOldResources();
}

const char *ResourceManager::versionDescription(ResVersion version) const {
	switch (version) {
	case kResVersionUnknown:
		return "Unknown";
	case kResVersionSci0Sci1Early:
		return "SCI0 / Early SCI1";
	case kResVersionSci1Middle:
		return "Middle SCI1";
	case kResVersionKQ5FMT:
		return "KQ5 FM Towns";
	case kResVersionSci1Late:
		return "Late SCI1";
	case kResVersionSci11:
		return "SCI1.1";
	case kResVersionSci11Mac:
		return "Mac SCI1.1+";
	case kResVersionSci2:
		return "SCI2/2.1";
	case kResVersionSci3:
		return "SCI3";
	}

	return "Version not valid";
}

ResVersion ResourceManager::detectMapVersion() {
	Common::ScopedPtr<Common::SeekableReadStream> fileStream;
	byte buff[6];
	ResourceSource *rsrc = nullptr;

	for (SourcesList::iterator it = _sources.begin(); it != _sources.end(); ++it) {
		rsrc = *it;

		if (rsrc->getSourceType() == kSourceExtMap) {
			if (rsrc->_resourceFile) {
				fileStream.reset(rsrc->_resourceFile->createReadStream());
			} else {
				Common::File *file = new Common::File();
				file->open(rsrc->getLocationName());
				if (file->isOpen())
					fileStream.reset(file);
				else
					delete file;
			}
			break;
		} else if (rsrc->getSourceType() == kSourceMacResourceFork) {
			return kResVersionSci11Mac;
		}
	}

	if (!fileStream) {
		warning("Failed to open resource map file");
		return kResVersionUnknown;
	}

	// detection
	// SCI0 and SCI01 maps have last 6 bytes set to FF
	fileStream->seek(-4, SEEK_END);
	uint32 uEnd = fileStream->readUint32LE();
	if (uEnd == 0xFFFFFFFF) {
		// check if the last 7 bytes are all ff, indicating a KQ5 FM-Towns map
		fileStream->seek(-7, SEEK_END);
		fileStream->read(buff, 3);
		if (buff[0] == 0xff && buff[1] == 0xff && buff[2] == 0xff) {
			return kResVersionKQ5FMT;
		}

		// check if 0 or 01 - try to read resources in SCI0 format and see if exists
		fileStream->seek(0, SEEK_SET);
		while (fileStream->read(buff, 6) == 6 && !(buff[0] == 0xFF && buff[1] == 0xFF && buff[2] == 0xFF)) {
			if (!findVolumeForMap(rsrc, (buff[5] & 0xFC) >> 2)) {
				return kResVersionSci1Middle;
			}
		}
		return kResVersionSci0Sci1Early;
	}

	// SCI1 and SCI1.1 maps consist of a fixed 3-byte header, a directory list (3-bytes each) that has one entry
	// of id FFh and points to EOF. The actual entries have 6-bytes on SCI1 and 5-bytes on SCI1.1
	byte directoryType = 0;
	uint16 directoryOffset = 0;
	uint16 lastDirectoryOffset = 0;
	uint16 directorySize = 0;
	ResVersion mapDetected = kResVersionUnknown;
	fileStream->seek(0, SEEK_SET);

	while (!fileStream->eos()) {
		directoryType = fileStream->readByte();
		directoryOffset = fileStream->readUint16LE();

		// Only SCI32 has directory type < 0x80
		if (directoryType < 0x80 && (mapDetected == kResVersionUnknown || mapDetected == kResVersionSci2))
			mapDetected = kResVersionSci2;
		else if (directoryType < 0x80 || ((directoryType & 0x7f) > 0x20 && directoryType != 0xFF))
			break;

		// Offset is above file size? -> definitely not SCI1/SCI1.1
		if (directoryOffset > fileStream->size())
			break;

		if (lastDirectoryOffset && mapDetected == kResVersionUnknown) {
			directorySize = directoryOffset - lastDirectoryOffset;
			if ((directorySize % 5) && (directorySize % 6 == 0))
				mapDetected = kResVersionSci1Late;
			if ((directorySize % 5 == 0) && (directorySize % 6))
				mapDetected = kResVersionSci11;
		}

		if (directoryType == 0xFF) {
			// FFh entry needs to point to EOF
			if (directoryOffset != fileStream->size())
				break;

			if (mapDetected)
				return mapDetected;
			return kResVersionSci1Late;
		}

		lastDirectoryOffset = directoryOffset;
	}

	return kResVersionUnknown;
}

ResVersion ResourceManager::detectVolVersion() {
	Common::ScopedPtr<Common::SeekableReadStream> fileStream;

	for (SourcesList::iterator it = _sources.begin(); it != _sources.end(); ++it) {
		ResourceSource *rsrc = *it;

		if (rsrc->getSourceType() == kSourceVolume) {
			if (rsrc->_resourceFile) {
				fileStream.reset(rsrc->_resourceFile->createReadStream());
			} else {
				Common::File *file = new Common::File();
				file->open(rsrc->getLocationName());
				if (file->isOpen())
					fileStream.reset(file);
				else
					delete file;
			}
			break;
		} else if (rsrc->getSourceType() == kSourceMacResourceFork)
			return kResVersionSci11Mac;
	}

	if (!fileStream) {
		warning("Failed to open volume file - if you got resource.p01/resource.p02/etc. files, merge them together into resource.000");
		// resource.p01/resource.p02/etc. may be there when directly copying the files from the original floppies
		// the sierra installer would merge those together (perhaps we could do this as well?)
		// possible TODO
		// example for such game: Laura Bow 2
		return kResVersionUnknown;
	}

	// SCI0 volume format:  {wResId wPacked+4 wUnpacked wCompression} = 8 bytes
	// SCI1 volume format:  {bResType wResNumber wPacked+4 wUnpacked wCompression} = 9 bytes
	// SCI1.1 volume format:  {bResType wResNumber wPacked wUnpacked wCompression} = 9 bytes
	// SCI32 volume format:   {bResType wResNumber dwPacked dwUnpacked wCompression} = 13 bytes
	// Try to parse volume with SCI0 scheme to see if it make sense
	// Checking 1MB of data should be enough to determine the version
	uint16 wCompression;
	uint32 dwPacked, dwUnpacked;
	ResVersion curVersion = kResVersionSci0Sci1Early;
	bool failed = false;
	bool sci11Align = false;

	// Check for SCI0, SCI1, SCI1.1, SCI32 v2 (Gabriel Knight 1 CD) and SCI32 v3 (LSL7) formats
	while (!fileStream->eos() && fileStream->pos() < 0x100000) {
		if (curVersion > kResVersionSci0Sci1Early)
			fileStream->readByte();
		fileStream->skip(2);	// resId
		dwPacked = (curVersion < kResVersionSci2) ? fileStream->readUint16LE() : fileStream->readUint32LE();
		dwUnpacked = (curVersion < kResVersionSci2) ? fileStream->readUint16LE() : fileStream->readUint32LE();

		// The compression field is present, but bogus when
		// loading SCI3 volumes, the format is otherwise
		// identical to SCI2. We therefore get the compression
		// indicator here, but disregard it in the following
		// code.
		wCompression = fileStream->readUint16LE();

		if (fileStream->eos()) {
			return curVersion;
		}

		int chk;

		if (curVersion == kResVersionSci0Sci1Early)
			chk = 4;
		else if (curVersion < kResVersionSci2)
			chk = 20;
		else
			chk = 32; // We don't need this, but include it for completeness

		int offs = curVersion < kResVersionSci11 ? 4 : 0;
		if ((curVersion < kResVersionSci2 && wCompression > chk)
				|| (curVersion == kResVersionSci2 && wCompression != 0 && wCompression != 32)
				|| (wCompression == 0 && dwPacked != dwUnpacked + offs)
		        || (dwUnpacked < dwPacked - offs)) {

			// Retry with a newer SCI version
			if (curVersion == kResVersionSci0Sci1Early) {
				curVersion = kResVersionSci1Late;
			} else if (curVersion == kResVersionSci1Late) {
				curVersion = kResVersionSci11;
			} else if (curVersion == kResVersionSci11 && !sci11Align) {
				// Later versions (e.g. QFG1VGA) have resources word-aligned
				sci11Align = true;
			} else if (curVersion == kResVersionSci11) {
				curVersion = kResVersionSci2;
			} else if (curVersion == kResVersionSci2) {
				curVersion = kResVersionSci3;
			} else {
				// All version checks failed, exit loop
				failed = true;
				break;
			}

			fileStream->seek(0);
			continue;
		}

		if (curVersion < kResVersionSci11)
			fileStream->seek(dwPacked - 4, SEEK_CUR);
		else if (curVersion == kResVersionSci11)
			fileStream->seek(sci11Align && ((9 + dwPacked) % 2) ? dwPacked + 1 : dwPacked, SEEK_CUR);
		else if (curVersion >= kResVersionSci2)
			fileStream->seek(dwPacked, SEEK_CUR);
	}

	if (!failed)
		return curVersion;

	// Failed to detect volume version
	return kResVersionUnknown;
}

bool ResourceManager::isBlacklistedPatch(const ResourceId &resId) const {
	switch (g_sci->getGameId()) {
	case GID_SHIVERS:
		// The SFX resource map patch in the Shivers interactive demo has
		// broken offsets for some sounds; ignore it so that the correct map
		// from RESSCI.000 will be used instead.
		return g_sci->isDemo() &&
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

// version-agnostic patch application
void ResourceManager::processPatch(ResourceSource *source, ResourceType resourceType, uint16 resourceNr, uint32 tuple) {
	Common::ScopedPtr<Common::SeekableReadStream> fileStream;
	Resource *newrsc = nullptr;
	ResourceId resId = ResourceId(resourceType, resourceNr, tuple);
	ResourceType checkForType = resourceType;

	if (isBlacklistedPatch(resId)) {
		debug("Skipping blacklisted patch file %s", source->getLocationName().c_str());
		delete source;
		return;
	}

	// base36 encoded patches (i.e. audio36 and sync36) have the same type as their non-base36 encoded counterparts
	if (checkForType == kResourceTypeAudio36)
		checkForType = kResourceTypeAudio;
	else if (checkForType == kResourceTypeSync36)
		checkForType = kResourceTypeSync;

	if (source->_resourceFile) {
		fileStream.reset(source->_resourceFile->createReadStream());
	} else {
		Common::File *file = new Common::File();
		if (!file->open(source->getLocationName())) {
			warning("ResourceManager::processPatch(): failed to open %s", source->getLocationName().c_str());
			delete source;
			delete file;
			return;
		}
		fileStream.reset(file);
	}

	int fsize = fileStream->size();
	if (fsize < 3) {
		debug("Patching %s failed - file too small", source->getLocationName().c_str());
		delete source;
		return;
	}

	byte patchType;
	if (fileStream->readUint32BE() == MKTAG('R','I','F','F')) {
		fileStream->seek(-4, SEEK_CUR);
		patchType = kResourceTypeAudio;
	} else {
		fileStream->seek(-4, SEEK_CUR);
		patchType = convertResType(fileStream->readByte());
	}

	enum {
		kExtraHeaderSize    = 2, ///< extra header used in gfx resources
		kViewHeaderSize     = 22 ///< extra header used in view resources
	};

	int32 patchDataOffset = kResourceHeaderSize;
	if (_volVersion < kResVersionSci11) {
		patchDataOffset += fileStream->readByte();
	} else {
		switch (patchType) {
		case kResourceTypeView:
			fileStream->seek(3, SEEK_SET);
			patchDataOffset += fileStream->readByte() + kViewHeaderSize + kExtraHeaderSize;
			break;
		case kResourceTypePic:
			if (_volVersion < kResVersionSci2) {
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
		debug("Patching %s failed - resource type mismatch", source->getLocationName().c_str());
		delete source;
		return;
	}

	if (patchDataOffset >= fsize) {
		debug("Patching %s failed - patch starting at offset %d can't be in file of size %d",
		      source->getLocationName().c_str(), patchDataOffset, fsize);
		delete source;
		return;
	}

	// Overwrite everything, because we're patching
	newrsc = updateResource(resId, source, 0, fsize - patchDataOffset, source->getLocationName());
	newrsc->_headerSize = patchDataOffset;

	debugC(1, kDebugLevelResMan, "Patching %s - OK", source->getLocationName().c_str());
}

void ResourceManager::processWavePatch(const ResourceId &resourceId, const Common::String &name) {
	ResourceSource *resSrc = new WaveResourceSource(name);
	Common::File file;
	file.open(name);

	updateResource(resourceId, resSrc, 0, file.size(), name);
	_sources.push_back(resSrc);

	debugC(1, kDebugLevelResMan, "Patching %s - OK", name.c_str());
}

#ifdef ENABLE_SCI32
void ResourceManager::findDisc(const int16 discNo) {
	// Since all resources are expected to be copied from the original discs
	// into a single game directory, this call just records the number of the CD
	// that the game has requested
	_currentDiscNo = discNo;
}
#endif

const Resource *ResourceManager::testResource(ResourceId id) const {
	return _resMap.getVal(id, nullptr);
}

void ResourceManager::addAppropriateSources() {
#ifdef ENABLE_SCI32
	_multiDiscAudio = false;
#endif
	if (Common::File::exists("resource.map")) {
		// SCI0-SCI2 file naming scheme
		ResourceSource *map = addExternalMap("resource.map");

		Common::ArchiveMemberList files;
		SearchMan.listMatchingMembers(files, "resource.0??");

		for (Common::ArchiveMemberList::const_iterator x = files.begin(); x != files.end(); ++x) {
			const Common::String name = (*x)->getName();
			const char *dot = strrchr(name.c_str(), '.');
			int number = atoi(dot + 1);

			addSource(new VolumeResourceSource(name, map, number));
		}
#ifdef ENABLE_SCI32
		// GK1CD hires content
		if (Common::File::exists("alt.map") && Common::File::exists("resource.alt"))
			addSource(new VolumeResourceSource("resource.alt", addExternalMap("alt.map", 10), 10));
#endif
	} else if (Common::MacResManager::exists("Data1")) {
		// Mac SCI1.1+ file naming scheme
		Common::StringArray files;
		Common::MacResManager::listFiles(files, "Data?");

		for (Common::StringArray::const_iterator x = files.begin(); x != files.end(); ++x) {
			addSource(new MacResourceForkResourceSource(*x, atoi(x->c_str() + 4)));
		}

#ifdef ENABLE_SCI32
		// There can also be a "Patches" resource fork with patches
		if (Common::MacResManager::exists("Patches"))
			addSource(new MacResourceForkResourceSource("Patches", 100));
	} else {
		// SCI2.1-SCI3 file naming scheme
		Common::ArchiveMemberList mapFiles, files;
		SearchMan.listMatchingMembers(mapFiles, "resmap.0??");
		SearchMan.listMatchingMembers(files, "ressci.0??");

		if (mapFiles.empty() || files.empty()) {
			warning("Could not find any resource bundles");
			_hasBadResources = true;
			return;
		}

		if (Common::File::exists("ressci.001")) {
			_multiDiscAudio = true;
		}

		for (Common::ArchiveMemberList::const_iterator mapIterator = mapFiles.begin(); mapIterator != mapFiles.end(); ++mapIterator) {
			Common::String mapName = (*mapIterator)->getName();
			int mapNumber = atoi(strrchr(mapName.c_str(), '.') + 1);
			bool foundVolume = false;

			for (Common::ArchiveMemberList::const_iterator fileIterator = files.begin(); fileIterator != files.end(); ++fileIterator) {
				Common::String resName = (*fileIterator)->getName();
				int resNumber = atoi(strrchr(resName.c_str(), '.') + 1);

				if (mapNumber == resNumber) {
					foundVolume = true;
					addSource(new VolumeResourceSource(resName, addExternalMap(mapName, mapNumber), mapNumber));
					break;
				}
			}

			if (!foundVolume &&
				// GK2 on Steam comes with an extra bogus resource map file;
				// ignore it instead of treating it as a bad resource
				(g_sci->getGameId() != GID_GK2 || mapFiles.size() != 2 || mapNumber != 1)) {

				warning("Could not find corresponding volume for %s", mapName.c_str());
				_hasBadResources = true;
			}
		}

		// SCI2.1 resource patches
		if (Common::File::exists("resmap.pat") && Common::File::exists("ressci.pat")) {
			// We add this resource with a map which surely won't exist
			addSource(new VolumeResourceSource("ressci.pat", addExternalMap("resmap.pat", kResPatVolumeNumber), kResPatVolumeNumber));
		}
	}
#else
	} else
		return;
#endif

	addPatchDir(".");

	if (Common::File::exists("message.map"))
		addSource(new VolumeResourceSource("resource.msg", addExternalMap("message.map"), 0));

	if (Common::File::exists("altres.map"))
		addSource(new VolumeResourceSource("altres.000", addExternalMap("altres.map"), 0));

	_patcher = new ResourcePatcher(g_sci->getGameId(), g_sci->getLanguage());
	addSource(_patcher);
}

void ResourceManager::addAppropriateSourcesForDetection(const Common::FSList &fslist) {
	ResourceSource *map = nullptr;
	Common::Array<ResourceSource *> sci21Maps;

#ifdef ENABLE_SCI32
	ResourceSource *sci21PatchMap = nullptr;
	const Common::FSNode *sci21PatchRes = nullptr;
	_multiDiscAudio = false;
#endif

	// First, find resource.map
	for (Common::FSList::const_iterator file = fslist.begin(); file != fslist.end(); ++file) {
		if (file->isDirectory())
			continue;

		Common::String filename = file->getName();
		filename.toLowercase();

		if (filename.contains("resource.map"))
			map = addExternalMap(file);

		if (filename.contains("resmap.0")) {
			const char *dot = strrchr(filename.c_str(), '.');
			uint number = atoi(dot + 1);

			// We need to store each of these maps for use later on
			if (number >= sci21Maps.size())
				sci21Maps.resize(number + 1);

			sci21Maps[number] = addExternalMap(file, number);
		}

#ifdef ENABLE_SCI32
		// SCI2.1 resource patches
		if (filename.contains("resmap.pat"))
			sci21PatchMap = addExternalMap(file, kResPatVolumeNumber);

		if (filename.contains("ressci.pat"))
			sci21PatchRes = file;
#endif
	}

	if (!map && sci21Maps.empty())
		return;

#ifdef ENABLE_SCI32
	if (sci21PatchMap && sci21PatchRes)
		addSource(new VolumeResourceSource(sci21PatchRes->getName(), sci21PatchMap, kResPatVolumeNumber, sci21PatchRes));
#endif

	// Now find all the resource.0?? files
	for (Common::FSList::const_iterator file = fslist.begin(); file != fslist.end(); ++file) {
		if (file->isDirectory())
			continue;

		Common::String filename = file->getName();
		filename.toLowercase();

		if (filename.contains("resource.0")) {
			const char *dot = strrchr(filename.c_str(), '.');
			int number = atoi(dot + 1);

			addSource(new VolumeResourceSource(file->getName(), map, number, file));
		} else if (filename.contains("ressci.0")) {
			const char *dot = strrchr(filename.c_str(), '.');
			int number = atoi(dot + 1);

			// Match this volume to its own map
			addSource(new VolumeResourceSource(file->getName(), sci21Maps[number], number, file));
		}
	}

	// This function is only called by the advanced detector, and we don't really need
	// to add a patch directory or message.map here
}

#ifdef ENABLE_SCI32
void ResourceManager::scanMultiDiscAudioMap(const ResourceSource *source, const int mapVolumeNr, const ResourceId resId) {
	IntMapResourceSource *audioMap = new IntMapResourceSource(source->getLocationName(), mapVolumeNr, resId.getNumber());
	addSource(audioMap);

	Common::String volumeName;
	if (mapVolumeNr == kResPatVolumeNumber) {
		if (resId.getNumber() == kSfxModule) {
			volumeName = "RESSCI.PAT";
		} else {
			volumeName = "RESAUD.001";
		}
	} else if (resId.getNumber() == kSfxModule) {
		volumeName = Common::String::format("RESSFX.%03d", mapVolumeNr);

		if (g_sci->getGameId() == GID_RAMA && !Common::File::exists(volumeName)) {
			if (Common::File::exists("RESOURCE.SFX")) {
				volumeName = "RESOURCE.SFX";
			} else if (Common::File::exists("RESSFX.001")) {
				volumeName = "RESSFX.001";
			}
		}
	} else {
		volumeName = Common::String::format("RESAUD.%03d", mapVolumeNr);
	}

	ResourceSource *audioVolume = new AudioVolumeResourceSource(this, volumeName, audioMap, mapVolumeNr);
	addSource(audioVolume);
	if (!audioMap->_scanned) {
		audioVolume->_scanned = true;
		audioMap->_scanned = true;
		audioMap->scanSource(this);
	}
}

void ResourceManager::addScriptChunkSource() {
	if (_mapVersion >= kResVersionSci2) {
		// If we have no scripts, but chunk 0 is present, open up the chunk
		// to try to get to any scripts in there. The Lighthouse SCI2.1 demo
		// does exactly this.
		Common::List<ResourceId> resources = listResources(kResourceTypeScript);
		if (resources.empty() && testResource(ResourceId(kResourceTypeChunk, 0))) {
			addSource(new ChunkResourceSource("Chunk 0", 0));
		}
	}
}
#endif

void ResourceManager::scanNewSources() {
	_hasBadResources = false;

	for (SourcesList::iterator it = _sources.begin(); it != _sources.end(); ++it) {
		ResourceSource *source = *it;
		if (!source->_scanned) {
			source->_scanned = true;
			if (!source->scanSource(this)) {
				_hasBadResources = true;
			}
		}
	}

	// The warning dialog is shown here instead of someplace more obvious like
	// SciEngine::run because resource sources can be dynamically added
	// (e.g. KQ5 via kDoAudio, MGDX via kSetLanguage), and users really should
	// be warned of bad resources in this situation (KQ Collection 1997 has a
	// bad copy of KQ5 on CD 1; the working copy is on CD 2)
	if (!_detectionMode && _hasBadResources) {
		showScummVMDialog(_("Missing or corrupt game resources have been detected. "
							"Some game features may not work properly. Please check "
							"the console for more information, and verify that your "
							"game files are valid."));
	}
}

ResourceSource *ResourceManager::addExternalMap(const Common::String &filename, int volumeNo) {
#ifdef ENABLE_SCI32
	const bool scanAudioMaps = _multiDiscAudio;
#else
	const bool scanAudioMaps = false;
#endif

	ResourceSource *newsrc = new ExtMapResourceSource(filename, volumeNo, scanAudioMaps);
	_sources.push_back(newsrc);
	return newsrc;
}

ResourceSource *ResourceManager::addExternalMap(const Common::FSNode *mapFile, int volumeNo) {
#ifdef ENABLE_SCI32
	const bool scanAudioMaps = _multiDiscAudio;
#else
	const bool scanAudioMaps = false;
#endif

	ResourceSource *newsrc = new ExtMapResourceSource(mapFile->getName(), volumeNo, scanAudioMaps, mapFile);
	_sources.push_back(newsrc);
	return newsrc;
}

void ResourceManager::addPatchDir(const Common::String &dirname) {
	ResourceSource *newsrc = new DirectoryResourceSource(dirname);
	_sources.push_back(newsrc);
}

const ResourceSource *ResourceManager::findVolumeForMap(const ResourceSource *map, int volumeNo) const {
	for (SourcesList::const_iterator it = _sources.begin(); it != _sources.end(); ++it) {
		const ResSourceType type = (*it)->getSourceType();
		if (type != kSourceVolume && type != kSourceAudioVolume) {
			continue;
		}
		if (static_cast<const VolumeResourceSource *>(*it)->isVolumeForMap(map, volumeNo)) {
			return *it;
		}
	}

	return nullptr;
}

Common::SeekableReadStream *ResourceManager::getVolumeFile(const ResourceSource *source) const {
	VolumeFiles::iterator it = _volumeFiles.begin();
	Common::File *file;

#ifdef ENABLE_SCI32
	if (source->getSourceType() == kSourceChunk) {
		const ChunkResourceSource *chunkSource = static_cast<const ChunkResourceSource *>(source);
		const Resource *res = findResource(ResourceId(kResourceTypeChunk, chunkSource->getNumber()), false);
		return res ? res->makeStream() : nullptr;
	}
#endif

	if (source->_resourceFile)
		return source->_resourceFile->createReadStream();

	const char *filename = source->getLocationName().c_str();

	// check if file is already opened
	while (it != _volumeFiles.end()) {
		file = *it;
		if (scumm_stricmp(file->getName(), filename) == 0) {
			// move file to top
			if (it != _volumeFiles.begin()) {
				_volumeFiles.erase(it);
				_volumeFiles.push_front(file);
			}
			return file;
		}
		++it;
	}
	// adding a new file
	file = new Common::File;
	if (file->open(filename)) {
		if (_volumeFiles.size() == kMaxOpenVolumes) {
			it = --_volumeFiles.end();
			delete *it;
			_volumeFiles.erase(it);
		}
		_volumeFiles.push_front(file);
		return file;
	}
	// failed
	delete file;
	return nullptr;
}

void ResourceManager::disposeVolumeFileStream(Common::SeekableReadStream *fileStream, const ResourceSource *source) const {
#ifdef ENABLE_SCI32
	if (source->getSourceType() == kSourceChunk) {
		delete fileStream;
		return;
	}
#endif

	if (source->_resourceFile) {
		delete fileStream;
		return;
	}

	// Other volume file streams are cached in _volumeFiles and should only be
	// deleted from _volumeFiles
}

void ResourceManager::loadResource(Resource *res) const {
	res->_source->loadResource(this, res);
	if (_patcher) {
		_patcher->applyPatch(*res);
	}
}

ResourceCompression ResourceManager::getViewCompression() {
	int viewsTested = 0;

	// Test 10 views to see if any are compressed
	for (int i = 0; i < 1000; i++) {
		const Resource *res = testResource(ResourceId(kResourceTypeView, i));

		if (!res || res->_source->getSourceType() != kSourceVolume)
			continue;

		Common::SeekableReadStream *fileStream = getVolumeFile(res->_source);

		if (!fileStream)
			continue;

		fileStream->seek(res->_fileOffset, SEEK_SET);

		ResourceHeader header;

		if (readResourceHeader(fileStream, header) != SCI_ERROR_NONE) {
			disposeVolumeFileStream(fileStream, res->_source);
			continue;
		}

		disposeVolumeFileStream(fileStream, res->_source);

		if (header.compression != kCompNone)
			return header.compression;

		if (++viewsTested == 10)
			break;
	}

	return kCompNone;
}

ViewType ResourceManager::detectViewType() {
	for (int i = 0; i < 1000; i++) {
		const Resource *res = findResource(ResourceId(kResourceTypeView, i), false);

		if (res) {
			// Skip views coming from patch files
			if (res->_source->getSourceType() == kSourcePatch)
				continue;

			switch (res->getUint8At(1)) {
			case 128:
				// If the 2nd byte is 128, it's a VGA game.
				// However, Longbow Amiga (AGA, 64 colors), also sets this byte
				// to 128, but it's a mixed VGA/Amiga format. Detect this from
				// the platform here.
				if (g_sci && g_sci->getPlatform() == Common::kPlatformAmiga)
					return kViewAmiga64;

				return kViewVga;
			case 0:
				// EGA or Amiga, try to read as Amiga view

				if (res->size() < 10)
					return kViewUnknown;

				// Read offset of first loop
				uint16 offset = res->getUint16LEAt(8);

				if (offset + 6U >= res->size())
					return kViewUnknown;

				// Read offset of first cel
				offset = res->getUint16LEAt(offset + 4);

				if (offset + 4U >= res->size())
					return kViewUnknown;

				// Check palette offset, amiga views have no palette
				if (res->getUint16LEAt(6) != 0)
					return kViewEga;

				uint16 width = res->getUint16LEAt(offset);
				offset += 2;
				uint16 height = res->getUint16LEAt(offset);
				offset += 6;

				// To improve the heuristic, we skip very small views
				if (height < 10)
					continue;

				// Check that the RLE data stays within bounds
				int y;
				for (y = 0; y < height; y++) {
					int x = 0;

					while ((x < width) && (offset < res->size())) {
						byte op = res->getUint8At(offset++);
						x += (op & 0x07) ? op & 0x07 : op >> 3;
					}

					// Make sure we got exactly the right number of pixels for this row
					if (x != width)
						return kViewEga;
				}

				return kViewAmiga;
			}
		}
	}

	// this may happen if there are serious system issues (or trying to add a broken game)
	warning("resMan: Couldn't find any views");
	return kViewUnknown;
}

// to detect selector "wordFail" in LE vocab resource
static const byte detectSci21EarlySignature[] = {
	10, // size of signature
	0x08, 0x00, 'w', 'o', 'r', 'd', 'F', 'a', 'i', 'l'
};

// to detect selector "wordFail" in BE vocab resource (SCI2.1 Early)
static const byte detectSci21EarlyBESignature[] = {
	10, // size of signature
	0x00, 0x08, 'w', 'o', 'r', 'd', 'F', 'a', 'i', 'l'
};

// to detect new kString calling to detect SCI2.1 Late
static const byte detectSci21NewStringSignature[] = {
	8, // size of signature
	0x78, // push1
	0x78, // push1
	0x39, 0x09, // pushi 09
	0x59, 0x01, // rest 01
	0x43, 0x5c, // callk String
};

bool ResourceManager::checkResourceDataForSignature(const Resource *resource, const byte *signature) {
	byte signatureSize = *signature;

	signature++; // skip over size byte
	if (signatureSize < 4)
		error("resource signature is too small, internal error");
	if (signatureSize > resource->size())
		return false;

	const uint32 signatureDWord = READ_UINT32(signature);
	signature += 4; signatureSize -= 4;

	const uint32 searchLimit = resource->size() - signatureSize + 1;
	uint32 DWordOffset = 0;
	while (DWordOffset < searchLimit) {
		if (signatureDWord == resource->getUint32At(DWordOffset)) {
			// magic DWORD found, check if the rest matches as well
			uint32 offset = DWordOffset + 4;
			uint32 signaturePos  = 0;
			while (signaturePos < signatureSize) {
				if (resource->getUint8At(offset) != signature[signaturePos])
					break;
				offset++;
				signaturePos++;
			}
			if (signaturePos >= signatureSize)
				return true; // signature found
		}
		DWordOffset++;
	}
	return false;
}

bool ResourceManager::checkResourceForSignatures(ResourceType resourceType, uint16 resourceNr, const byte *signature1, const byte *signature2) {
	const Resource *resource = findResource(ResourceId(resourceType, resourceNr), false);

	if (resource) {
		if (signature1 && checkResourceDataForSignature(resource, signature1)) {
			return true;
		}

		if (signature2 && checkResourceDataForSignature(resource, signature2)) {
			return true;
		}
	}

	return false;
}

void ResourceManager::detectSciVersion() {
	s_sciVersion = SCI_VERSION_0_EARLY;
	bool oldDecompressors = true;

	// We use the view compression to set a preliminary s_sciVersion for the
	// sake of getResourceInfo
	ResourceCompression viewCompression;
#ifdef ENABLE_SCI32
	viewCompression = getViewCompression();
#else
	if (_volVersion >= kResVersionSci2) {
		// SCI32 support isn't built in, thus view detection will fail
		viewCompression = kCompUnknown;
	} else {
		viewCompression = getViewCompression();
	}
#endif

	if (viewCompression != kCompLZW) {
		// If it's a different compression type from kCompLZW, the game is probably
		// SCI_VERSION_1_EGA_ONLY or later. If the views are uncompressed, it is
		// likely not an early disk game.
		s_sciVersion = SCI_VERSION_1_EGA_ONLY;
		oldDecompressors = false;
	}

	// Set view type
	if (viewCompression == kCompDCL
		|| _volVersion == kResVersionSci11 // pq4demo
		|| _volVersion == kResVersionSci11Mac
#ifdef ENABLE_SCI32
		|| viewCompression == kCompSTACpack
		|| _volVersion == kResVersionSci2 // kq7
#endif
		) {
		// SCI1.1 VGA views
		_viewType = kViewVga11;
	} else {
#ifdef ENABLE_SCI32
		// Otherwise we detect it from a view
		_viewType = detectViewType();
#else
		if (_volVersion == kResVersionSci2 && viewCompression == kCompUnknown) {
			// A SCI32 game, but SCI32 support is disabled. Force the view type
			// to kViewVga11, as we can't read from the game's resource files
			_viewType = kViewVga11;
		} else {
			_viewType = detectViewType();
		}
#endif
	}

	if (_volVersion == kResVersionSci11Mac) {
		const Resource *res = testResource(ResourceId(kResourceTypeScript, 64920));
		// Distinguish between SCI1.1 and SCI32 games here. SCI32 games will
		// always include script 64920 (the Array class). Note that there are
		// no Mac SCI2 games. Yes, that means that GK1 Mac is SCI2.1 and not SCI2.

		// TODO: Decide between SCI2.1 and SCI3
		if (res) {
			s_sciVersion = SCI_VERSION_2_1_EARLY; // we check for SCI2.1 specifics a bit later
		} else {
			s_sciVersion = SCI_VERSION_1_1;
			return;
		}
	}

	// Handle SCI32 versions here
	if (s_sciVersion != SCI_VERSION_2_1_EARLY) {
		if (_volVersion >= kResVersionSci2) {
			Common::List<ResourceId> heaps = listResources(kResourceTypeHeap);
			bool hasHeapResources = !heaps.empty();

			// SCI2.1/3 and SCI1 Late resource maps are the same, except that
			// SCI1 Late resource maps have the resource types or'd with
			// 0x80. We differentiate between SCI2 and SCI2.1/3 based on that.
			if (_mapVersion == kResVersionSci1Late) {
				s_sciVersion = SCI_VERSION_2;
				return;
			} else if (hasHeapResources) {
				s_sciVersion = SCI_VERSION_2_1_EARLY; // exact SCI2.1 version is checked a bit later
			} else {
				s_sciVersion = SCI_VERSION_3;
				return;
			}
		}
	}

	if (s_sciVersion == SCI_VERSION_2_1_EARLY) {
		// we only know that it's SCI2.1, not which exact version it is

		// check, if selector "wordFail" inside vocab 997 exists, if it does it's SCI2.1 Early
		if ((checkResourceForSignatures(kResourceTypeVocab, 997, detectSci21EarlySignature, detectSci21EarlyBESignature))) {
			// found -> it is SCI2.1 early
			return;
		}

		s_sciVersion = SCI_VERSION_2_1_MIDDLE;
		if (checkResourceForSignatures(kResourceTypeScript, 64918, detectSci21NewStringSignature, nullptr)) {
			// new kString call detected, it's SCI2.1 late
			// TODO: this call seems to be different on Mac
			s_sciVersion = SCI_VERSION_2_1_LATE;
			return;
		}
		return;
	}

	// Check for transitive SCI1/SCI1.1 games, like PQ1 here
	// If the game has any heap file (here we check for heap file 0), then
	// it definitely uses a SCI1.1 kernel
	if (testResource(ResourceId(kResourceTypeHeap, 0))) {
		s_sciVersion = SCI_VERSION_1_1;
		return;
	}

	switch (_mapVersion) {
	case kResVersionSci0Sci1Early:
		if (_viewType == kViewVga) {
			// VGA
			s_sciVersion = SCI_VERSION_1_EARLY;
			return;
		}

		// EGA
		if (hasOldScriptHeader()) {
			s_sciVersion = SCI_VERSION_0_EARLY;
			return;
		}

		if (hasSci0Voc999()) {
			s_sciVersion = SCI_VERSION_0_LATE;
			return;
		}

		if (oldDecompressors) {
			// It's either SCI_VERSION_0_LATE or SCI_VERSION_01

			// We first check for SCI1 vocab.999
			if (testResource(ResourceId(kResourceTypeVocab, 999))) {
				s_sciVersion = SCI_VERSION_01;
				return;
			}

			// If vocab.999 is missing, we try vocab.900
			if (testResource(ResourceId(kResourceTypeVocab, 900))) {
				if (hasSci1Voc900()) {
					s_sciVersion = SCI_VERSION_01;
					return;
				} else {
					s_sciVersion = SCI_VERSION_0_LATE;
					return;
				}
			}

			error("Failed to accurately determine SCI version");
		}

		// New decompressors. It's either SCI_VERSION_1_EGA_ONLY or SCI_VERSION_1_EARLY.
		if (hasSci1Voc900()) {
			s_sciVersion = SCI_VERSION_1_EGA_ONLY;
			return;
		}

		// SCI_VERSION_1_EARLY EGA versions lack the parser vocab
		s_sciVersion = SCI_VERSION_1_EARLY;
		return;
	case kResVersionSci1Middle:
	case kResVersionKQ5FMT:
		s_sciVersion = SCI_VERSION_1_MIDDLE;
		// Amiga SCI1 middle games are actually SCI1 late
		if (_viewType == kViewAmiga || _viewType == kViewAmiga64)
			s_sciVersion = SCI_VERSION_1_LATE;
		// Same goes for Mac SCI1 middle games
		if (g_sci && g_sci->getPlatform() == Common::kPlatformMacintosh)
			s_sciVersion = SCI_VERSION_1_LATE;
		return;
	case kResVersionSci1Late:
		if (_volVersion == kResVersionSci11) {
			s_sciVersion = SCI_VERSION_1_1;
			return;
		}
		s_sciVersion = SCI_VERSION_1_LATE;
		return;
	case kResVersionSci11:
		s_sciVersion = SCI_VERSION_1_1;
		return;
	default:
		s_sciVersion = SCI_VERSION_NONE;
		error("detectSciVersion(): Unable to detect the game's SCI version");
	}
}

// Functions below are based on PD code by Brian Provinciano (SCI Studio)
bool ResourceManager::hasOldScriptHeader() {
	const Resource *res = findResource(ResourceId(kResourceTypeScript, 0), false);

	if (!res) {
		// Script 0 missing -> corrupted / non-SCI resource files.
		// Don't error out here, because this might have been called
		// from the fallback detector
		return false;
	}

	uint offset = 2;
	const int objTypes = 17;

	while (offset < res->size()) {
		uint16 objType = res->getUint16LEAt(offset);

		if (!objType) {
			offset += 2;
			// We should be at the end of the resource now
			return offset == res->size();
		}

		if (objType >= objTypes) {
			// Invalid objType
			return false;
		}

		int skip = res->getUint16LEAt(offset + 2);

		if (skip < 2) {
			// Invalid size
			return false;
		}

		offset += skip;
	}

	return false;
}

bool ResourceManager::hasSci0Voc999() {
	const Resource *res = findResource(ResourceId(kResourceTypeVocab, 999), false);

	if (!res) {
		// No vocab present, possibly a demo version
		return false;
	}

	if (res->size() < 2)
		return false;

	uint16 count = res->getUint16LEAt(0);

	// Make sure there's enough room for the pointers
	if (res->size() < (uint)count * 2)
		return false;

	// Iterate over all pointers
	for (uint i = 0; i < count; i++) {
		// Offset to string
		uint16 offset = res->getUint16LEAt(2 + count * 2);

		// Look for end of string
		do {
			if (offset >= res->size()) {
				// Out of bounds
				return false;
			}
		} while (res->getUint8At(offset++));
	}

	return true;
}

bool ResourceManager::hasSci1Voc900() {
	const Resource *res = findResource(ResourceId(kResourceTypeVocab, 900), false);

	if (!res )
		return false;

	if (res->size() < 0x1fe)
		return false;

	uint16 offset = 0x1fe;

	while (offset < res->size()) {
		offset++;
		do {
			if (offset >= res->size()) {
				// Out of bounds;
				return false;
			}
		} while (res->getUint8At(offset++));
		offset += 3;
	}

	return offset == res->size();
}

reg_t ResourceManager::findGameObject(const bool addSci11ScriptOffset, const bool isBE) {
	const Resource *script = findResource(ResourceId(kResourceTypeScript, 0), false);

	if (!script)
		return NULL_REG;

	if (getSciVersion() <= SCI_VERSION_1_LATE) {
		SciSpan<const byte> buf = (getSciVersion() == SCI_VERSION_0_EARLY) ? script->subspan(2) : *script;
		SciSpan<const byte> block;

		// Check if the first block is the exports block (in most cases, it is)
		bool exportsIsFirst = buf.getUint16LEAt(4) == SCI_OBJ_EXPORTS;
		if (exportsIsFirst) {
			block = buf.subspan(4 + sizeof(uint16));
		} else {
			block = Script::findBlockSCI0(*script, SCI_OBJ_EXPORTS);
			if (!block)
				error("Unable to find exports block from script 0");
			block += 4 + sizeof(uint16);
		}

		int16 offset = !isSci11Mac() ? block.getUint16LEAt(0) : block.getUint16BEAt(0);
		return make_reg(1, offset);
	} else if (getSciVersion() >= SCI_VERSION_1_1 && getSciVersion() <= SCI_VERSION_2_1_LATE) {
		SciSpan<const byte> block = script->subspan(4 + 2 + 2);

		// In SCI1.1 - SCI2.1, the heap is appended at the end of the script,
		// so adjust the offset accordingly if requested
		int16 offset = !isSci11Mac() ? block.getUint16LEAt(0) : block.getUint16BEAt(0);
		if (addSci11ScriptOffset) {
			offset += script->size();

			// Ensure that the start of the heap is word-aligned - same as in Script::init()
			if (script->size() & 2)
				offset++;
		}

		return make_reg(1, offset);
	} else {
#ifdef ENABLE_SCI32
		return make_reg(1, Script::relocateOffsetSci3(*script, 22, isBE));
#else
		return NULL_REG;
#endif
	}
}

bool ResourceManager::validateResource(const ResourceId &resourceId, const Common::String &sourceMapLocation, const Common::String &sourceName, const uint32 offset, const uint32 size, const uint32 sourceSize) const {
	if (size != 0) {
		if (offset + size > sourceSize) {
			warning("Resource %s from %s points beyond end of %s (%u + %u > %u)", resourceId.toString().c_str(), sourceMapLocation.c_str(), sourceName.c_str(), offset, size, sourceSize);
			return false;
		}
	} else {
		if (offset >= sourceSize) {
			warning("Resource %s from %s points beyond end of %s (%u >= %u)", resourceId.toString().c_str(), sourceMapLocation.c_str(), sourceName.c_str(), offset, sourceSize);
			return false;
		}
	}

	return true;
}

Resource *ResourceManager::addResource(ResourceId resId, const ResourceSource *src, uint32 offset, uint32 size, const Common::String &sourceMapLocation) {
	// Adding new resource only if it does not exist
	if (_resMap.contains(resId) == false) {
		return updateResource(resId, src, offset, size, sourceMapLocation);
	} else {
		return _resMap.getVal(resId);
	}
}

Resource *ResourceManager::updateResource(ResourceId resId, const ResourceSource *src, uint32 size, const Common::String &sourceMapLocation) {
	uint32 offset = 0;
	if (_resMap.contains(resId)) {
		const Resource *res = _resMap.getVal(resId);
		offset = res->_fileOffset;
	}
	return updateResource(resId, src, offset, size, sourceMapLocation);
}

Resource *ResourceManager::addResourceWithoutValidation(ResourceId resId, const ResourceSource *src, uint32 offset, uint32 size) {
	Resource *res;
	if (_resMap.contains(resId)) {
		res = _resMap.getVal(resId);
	} else {
		res = new Resource(resId);
		res->_source = src;
		res->_fileOffset = offset;
		res->_size = size;
		_resMap.setVal(resId, res);
	}
	return res;
}

Resource *ResourceManager::updateResource(ResourceId resId, const ResourceSource *src, uint32 offset, uint32 size, const Common::String &sourceMapLocation) {
	// Update a patched resource, whether it exists or not
	Resource *res = _resMap.getVal(resId, nullptr);

	Common::SeekableReadStream *volumeFile = getVolumeFile(src);
	if (volumeFile == nullptr) {
		error("Could not open %s for reading", src->getLocationName().c_str());
	}

	if (src->getSourceType() == kSourceAudioVolume) {
		const AudioVolumeResourceSource *avSrc = static_cast<const AudioVolumeResourceSource *>(src);
		if (!avSrc->relocateMapOffset(offset, size)) {
			warning("Compressed volume %s does not contain a valid entry for %s (map offset %u)", src->getLocationName().c_str(), resId.toString().c_str(), offset);
			_hasBadResources = true;
			disposeVolumeFileStream(volumeFile, src);
			return res;
		}
	}

	if (validateResource(resId, sourceMapLocation, src->getLocationName(), offset, size, volumeFile->size())) {
		if (res == nullptr) {
			res = new Resource(resId);
			_resMap.setVal(resId, res);
		}

		res->_status = kResStatusNoMalloc;
		res->_source = src;
		res->_headerSize = 0;
		res->_fileOffset = offset;
		res->_size = size;
	} else {
		_hasBadResources = true;
	}

	disposeVolumeFileStream(volumeFile, src);
	return res;
}

ResourceType ResourceManager::convertResType(byte type) const {
	type &= 0x7f;

	bool useSci0 = _mapVersion < kResVersionSci2;

	// LSL6 hires doesn't have the chunk resource type, to match
	// the resource types of the lowres version, thus we use the
	// older resource types here.
	// PQ4 CD and QFG4 CD are SCI2.1, but use the resource types of the
	// corresponding SCI2 floppy disk versions.
	if (g_sci && (g_sci->getGameId() == GID_LSL6HIRES ||
	        g_sci->getGameId() == GID_QFG4 || g_sci->getGameId() == GID_PQ4))
		useSci0 = true;

	if (useSci0) {
		if (type < ARRAYSIZE(s_resTypeMapSci0))
			return s_resTypeMapSci0[type];
	} else {
		if (type < ARRAYSIZE(s_resTypeMapSci21))
			return s_resTypeMapSci21[type];
	}

	return kResourceTypeInvalid;
}

void ResourceManager::addNewGMPatch() {
	Common::String gmPatchFile;

	switch (g_sci->getGameId()) {
	case GID_ECOQUEST:
		gmPatchFile = "ECO1GM.PAT";
		break;
	case GID_HOYLE3:
		gmPatchFile = "HOY3GM.PAT";
		break;
	case GID_LSL1:
		gmPatchFile = "LL1_GM.PAT";
		break;
	case GID_LSL5:
		gmPatchFile = "LL5_GM.PAT";
		break;
	case GID_LONGBOW:
		gmPatchFile = "ROBNGM.PAT";
		break;
	case GID_SQ1:
		gmPatchFile = "SQ1_GM.PAT";
		break;
	case GID_SQ4:
		gmPatchFile = "SQ4_GM.PAT";
		break;
	case GID_FAIRYTALES:
		gmPatchFile = "TALEGM.PAT";
		break;
	default:
		break;
	}

	if (!gmPatchFile.empty() && Common::File::exists(gmPatchFile)) {
		ResourceSource *psrcPatch = new PatchResourceSource(gmPatchFile);
		processPatch(psrcPatch, kResourceTypePatch, 4);
	}
}

void ResourceManager::removeAudioResource(ResourceId resId) {
	Resource *res = _resMap.getVal(resId);

	if (res && res->_source->getSourceType() == kSourceAudioVolume) {
		if (res->_status == kResStatusLocked) {
			warning("Failed to remove resource %s (still in use)", resId.toString().c_str());
		} else {
			if (res->_status == kResStatusEnqueued)
				removeFromLRU(res);

			_resMap.erase(resId);
			delete res;
		}
	}
}

void ResourceManager::forcePurge(const ResourceId resId) {
	Resource *res = _resMap.getVal(resId);
	if (res->_status == kResStatusEnqueued) {
		removeFromLRU(res);
	}
	res->unalloc();
}

void ResourceManager::setAudioLanguage(int language) {
	if (_audioMapSCI1) {
		if (_audioMapSCI1->_volumeNumber == language) {
			// This language is already loaded
			return;
		}

		// We already have a map loaded, so we unload it first
		if (_audioMapSCI1->readAudioMapSCI1(this, true) != SCI_ERROR_NONE) {
			_hasBadResources = true;
		}

		// Remove all volumes that use this map from the source list
		SourcesList::iterator it = _sources.begin();
		while (it != _sources.end()) {
			const ResSourceType type = (*it)->getSourceType();
			if (type != kSourceVolume && type != kSourceAudioVolume) {
				++it;
				continue;
			}
			VolumeResourceSource *src = static_cast<VolumeResourceSource *>(*it);
			if (src->isVolumeForMap(_audioMapSCI1, src->_volumeNumber)) {
				it = _sources.erase(it);
				delete src;
			} else {
				++it;
			}
		}

		// Remove the map itself from the source list
		_sources.remove(_audioMapSCI1);
		delete _audioMapSCI1;

		_audioMapSCI1 = nullptr;
	}

	Common::String filename = Common::String::format("AUDIO%03d", language);
	Common::String fullname = filename + ".MAP";
	if (!Common::File::exists(fullname)) {
		warning("No audio map found for language %i", language);
		return;
	}

	_audioMapSCI1 = new ExtAudioMapResourceSource(fullname, language);
	addSource(_audioMapSCI1);

	// Search for audio volumes for this language and add them to the source list
	Common::ArchiveMemberList files;
	SearchMan.listMatchingMembers(files, filename + ".0??");
	for (Common::ArchiveMemberList::const_iterator x = files.begin(); x != files.end(); ++x) {
		const Common::String name = (*x)->getName();
		const char *dot = strrchr(name.c_str(), '.');
		int number = atoi(dot + 1);

		addSource(new AudioVolumeResourceSource(this, name, _audioMapSCI1, number));
	}

	scanNewSources();
}

int ResourceManager::getAudioLanguage() const {
	return (_audioMapSCI1 ? _audioMapSCI1->_volumeNumber : 0);
}

void ResourceManager::addAudioSources() {
#ifdef ENABLE_SCI32
	// Multi-disc audio is added during addAppropriateSources for those titles
	// that require it
	if (_multiDiscAudio) {
		return;
	}
#endif

	Common::List<ResourceId> resources = listResources(kResourceTypeMap);
	Common::List<ResourceId>::iterator itr;

	for (itr = resources.begin(); itr != resources.end(); ++itr) {
		const Resource *mapResource = _resMap.getVal(*itr);
		ResourceSource *src = new IntMapResourceSource(mapResource->getResourceLocation(), 0, itr->getNumber());
		addSource(src);

		if (itr->getNumber() == kSfxModule && Common::File::exists("RESOURCE.SFX"))
			addSource(new AudioVolumeResourceSource(this, "RESOURCE.SFX", src, 0));
		else if (Common::File::exists("RESOURCE.AUD"))
			addSource(new AudioVolumeResourceSource(this, "RESOURCE.AUD", src, 0));
		else
			return;
	}

	return;
}

#ifdef ENABLE_SCI32
void ResourceManager::changeAudioDirectory(Common::String path) {
	if (!path.empty()) {
		path += "/";
	}

	const Common::String resAudPath = path + "RESOURCE.AUD";

	if (!SearchMan.hasFile(resAudPath)) {
		error("Could not find %s", resAudPath.c_str());
	}

	// When a IntMapResourceSource is scanned, it will not update existing
	// resources. There is also no guarantee that there are exactly the same
	// number of audio36/sync36/map resources in each audio directory.
	// Therefore, all of these resources must be deleted before scanning.
	for (ResourceMap::iterator it = _resMap.begin(); it != _resMap.end(); ++it) {
		const ResourceType type = it->_key.getType();

		if (type == kResourceTypeMap || type == kResourceTypeAudio36 || type == kResourceTypeSync36) {
			if (type == kResourceTypeMap && it->_key.getNumber() == kSfxModule) {
				continue;
			}

			Resource *resource = it->_value;
			if (resource) {
				// If one of these resources ends up being locked here, it
				// probably means Audio32 is using it and we need to stop
				// playback of audio before switching directories
				assert(!resource->isLocked());

				if (resource->_status == kResStatusEnqueued) {
					removeFromLRU(resource);
				}

				// A PatchResourceSource is not added to _sources and is
				// automatically deleted when the corresponding Resource is
				// deleted
				delete resource;
			}

			_resMap.erase(it);
		}
	}

	for (SourcesList::iterator it = _sources.begin(); it != _sources.end(); ) {
		const ResSourceType type = (*it)->getSourceType();
		if (type == kSourceIntMap) {
			IntMapResourceSource *mapSource = static_cast<IntMapResourceSource *>(*it);
			if (mapSource->_mapNumber != kSfxModule) {
				delete mapSource;
				it = _sources.erase(it);
				continue;
			}
		}

		if (type == kSourceAudioVolume) {
			AudioVolumeResourceSource *volSource = static_cast<AudioVolumeResourceSource *>(*it);
			if (volSource->getLocationName().contains("RESOURCE.AUD")) {
				delete volSource;
				it = _sources.erase(it);
				continue;
			}
		}

		++it;
	}

	// # is used as the first pattern character to avoid matching non-audio maps
	// like RESOURCE.MAP
	Common::ArchiveMemberList mapFiles;
	SearchMan.listMatchingMembers(mapFiles, path + "#*.MAP");

	for (Common::ArchiveMemberList::const_iterator it = mapFiles.begin(); it != mapFiles.end(); ++it) {
		const Common::ArchiveMemberPtr &file = *it;
		assert(file);

		const Common::String fileName = file->getName();
		const int mapNo = atoi(fileName.c_str());

		// Sound effects are the same across all audio directories, so ignore
		// any new SFX map
		if (mapNo == kSfxModule) {
			continue;
		}

		ResourceSource *newSource = new PatchResourceSource(path + fileName);
		processPatch(newSource, kResourceTypeMap, mapNo);
		Resource *mapResource = _resMap.getVal(ResourceId(kResourceTypeMap, mapNo));
		assert(mapResource);

		ResourceSource *audioMap = new IntMapResourceSource(mapResource->getResourceLocation(), 0, mapNo);
		addSource(audioMap);
		addSource(new AudioVolumeResourceSource(this, resAudPath, audioMap, 0));
	}

	scanNewSources();
}
#endif

} // End of namespace Sci
