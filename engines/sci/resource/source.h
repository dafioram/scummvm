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

#ifndef SCI_RESOURCE_SOURCES_H
#define SCI_RESOURCE_SOURCES_H

#include "common/hashmap.h"
#include "common/str.h"

namespace Common {
class FSNode;
class SeekableReadStream;
}

namespace Sci {
class ResourceManager;
class Resource;

enum ResSourceType {
	kSourceDirectory = 0,	///< Directories containing game resources/patches
	kSourcePatch,			///< External resource patches
	kSourceVolume,			///< Game resources (resource.* or ressci.*)
	kSourceExtMap,			///< Non-audio resource maps
	kSourceIntMap,			///< SCI1.1 and later audio resource maps
	kSourceAudioVolume,		///< Audio resources - resource.sfx / resource.aud
	kSourceExtAudioMap,		///< SCI1 audio resource maps
	kSourceWave,			///< External WAVE files, patched in as sound resources
	kSourceMacResourceFork,	///< Mac SCI1.1 and later resource forks
#ifdef ENABLE_SCI32
	kSourceChunk,			///< Script chunk resources (*.chk)
#endif
	kSourceScummVM			///< Built-in resource patcher
};

/** Resource error codes. Should be in sync with s_errorDescriptions */
enum ResourceErrorCode {
	SCI_ERROR_NONE = 0,
	SCI_ERROR_IO_ERROR = 1,
	SCI_ERROR_EMPTY_RESOURCE = 2,
	SCI_ERROR_RESMAP_INVALID_ENTRY = 3,	/**< Invalid resource.map entry */
	SCI_ERROR_RESMAP_NOT_FOUND = 4,
	SCI_ERROR_NO_RESOURCE_FILES_FOUND = 5,	/**< No resource at all was found */
	SCI_ERROR_UNKNOWN_COMPRESSION = 6,
	SCI_ERROR_DECOMPRESSION_ERROR = 7,	/**< sanity checks failed during decompression */
	SCI_ERROR_RESOURCE_TOO_BIG = 8	/**< Resource size exceeds SCI_MAX_RESOURCE_SIZE */
};

class ResourceSource {
public:
	virtual ~ResourceSource() {}

	ResSourceType getSourceType() const { return _sourceType; }
	const Common::String &getLocationName() const { return _name; }

	/**
	 * Scan this source for resources to add to the resource manager.
	 */
	virtual bool scanSource(ResourceManager *) = 0;

	/**
	 * Load a resource from this source into the given Resource object.
	 */
	virtual void loadResource(const ResourceManager *resMan, Resource *res) const = 0;

	// FIXME: This audio specific method is a hack. After all, why should a
	// ResourceSource or a Resource (which uses this method) have audio
	// specific methods? But for now we keep this, as it eases transition.
	virtual uint32 getAudioCompressionType() const { return 0; }

	bool _scanned;
	const Common::FSNode *const _resourceFile;
	const int _volumeNumber;

protected:
	ResourceSource(ResSourceType type, const Common::String &name, int volumeNo = 0, const Common::FSNode *resFile = nullptr);

	// Auxiliary method, used by loadResource implementations.
	Common::SeekableReadStream *getVolumeFile(const ResourceManager *resMan, const Resource *res) const;

	/**
	 * Loads data from the file directly to the resource.
	 */
	bool loadFromStream(Common::SeekableReadStream *file, Resource *res) const;

	const ResSourceType _sourceType;
	const Common::String _name;
};

class DataOnlyResourceSource : public ResourceSource {
public:
	virtual bool scanSource(ResourceManager *) override { return true; }

protected:
	DataOnlyResourceSource(ResSourceType type, const Common::String &name, int volumeNo = 0, const Common::FSNode *resFile = nullptr) :
		ResourceSource(type, name, volumeNo, resFile) {}
};

class IndexOnlyResourceSource : public ResourceSource {
public:
	virtual void loadResource(const ResourceManager *resMan, Resource *res) const override;

protected:
	IndexOnlyResourceSource(ResSourceType type, const Common::String &name, int volumeNo = 0, const Common::FSNode *resFile = nullptr) :
		ResourceSource(type, name, volumeNo, resFile) {}
};

} // End of namespace Sci

#endif
