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

#ifndef SCI_RESOURCE_RESOURCE_H
#define SCI_RESOURCE_RESOURCE_H

#include "common/scummsys.h"
#include "common/str.h"
#include "sci/decompressor.h"
#include "sci/util.h"

namespace Common {
	class SeekableReadStream;
	class Serializer;
	class WriteStream;
}

namespace Sci {

enum ResourceType {
	kResourceTypeView = 0,
	kResourceTypePic,
	kResourceTypeScript,
	kResourceTypeText,
	kResourceTypeSound,
	kResourceTypeMemory,
	kResourceTypeVocab,
	kResourceTypeFont,
	kResourceTypeCursor,
	kResourceTypePatch,
	kResourceTypeBitmap,
	kResourceTypePalette,
	kResourceTypeCdAudio = 12,
#ifdef ENABLE_SCI32
	kResourceTypeWave = 12,
#endif
	kResourceTypeAudio,
	kResourceTypeSync,
	kResourceTypeMessage,
	kResourceTypeMap,
	kResourceTypeHeap,
	kResourceTypeAudio36,
	kResourceTypeSync36,
	kResourceTypeTranslation, // Currently unsupported

	// SCI2.1+ Resources
	kResourceTypeRobot,
	kResourceTypeVMD,
	kResourceTypeChunk,
	kResourceTypeAnimation,

	// SCI3 Resources
	kResourceTypeEtc,
	kResourceTypeDuck,
	kResourceTypeClut,
	kResourceTypeTGA,
	kResourceTypeZZZ,

	// Mac-only resources
	kResourceTypeMacIconBarPictN, // IBIN resources (icon bar, not selected)
	kResourceTypeMacIconBarPictS, // IBIS resources (icon bar, selected)
	kResourceTypeMacPict,        // PICT resources (inventory)

	kResourceTypeRave,	// KQ6 hires RAVE (special sync) resources

	kResourceTypeInvalid
};

enum ResourceStatus {
	kResStatusNoMalloc = 0,
	kResStatusAllocated,
	kResStatusEnqueued, /**< In the LRU queue */
	kResStatusLocked /**< Allocated and in use */
};

enum ResVersion {
	kResVersionUnknown,
	kResVersionSci0Sci1Early,
	kResVersionSci1Middle,
	kResVersionKQ5FMT,
	kResVersionSci1Late,
	kResVersionSci11,
	kResVersionSci11Mac,
	kResVersionSci2,
	kResVersionSci3
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

const char *getResourceErrorDescription(ResourceErrorCode code);
const char *getResourceTypeName(ResourceType restype);
const char *getResourceTypeExtension(ResourceType restype);

class ResourceId {
	static inline ResourceType fixupType(ResourceType type) {
		if (type >= kResourceTypeInvalid)
			return kResourceTypeInvalid;
		return type;
	}

	ResourceType _type;
	uint16 _number;
	uint32 _tuple; // Only used for audio36 and sync36

	static Common::String intToBase36(uint32 number, int minChar) {
		// Convert from an integer to a base36 string
		Common::String string;

		while (minChar--) {
			int character = number % 36;
			string = ((character < 10) ? (character + '0') : (character + 'A' - 10)) + string;
			number /= 36;
		}

		return string;
	}

	friend void syncWithSerializer(Common::Serializer &s, ResourceId &obj);

public:
	ResourceId() : _type(kResourceTypeInvalid), _number(0), _tuple(0) { }

	ResourceId(ResourceType type_, uint16 number_, uint32 tuple_ = 0)
			: _type(fixupType(type_)), _number(number_), _tuple(tuple_) {
	}

	ResourceId(ResourceType type_, uint16 number_, byte noun, byte verb, byte cond, byte seq)
			: _type(fixupType(type_)), _number(number_) {
		_tuple = (noun << 24) | (verb << 16) | (cond << 8) | seq;
	}

	Common::String toString() const {
		Common::String retStr = Common::String::format("%s.%d", getResourceTypeName(_type), _number);

		if (_tuple != 0) {
			retStr += Common::String::format("(%d, %d, %d, %d)", _tuple >> 24, (_tuple >> 16) & 0xff, (_tuple >> 8) & 0xff, _tuple & 0xff);
		}

		return retStr;
	}

	// Convert from a resource ID to a base36 patch name
	Common::String toPatchNameBase36() const;

	inline ResourceType getType() const { return _type; }
	inline uint16 getNumber() const { return _number; }
	inline uint32 getTuple() const { return _tuple; }

	inline uint hash() const {
		return ((uint)((_type << 16) | _number)) ^ _tuple;
	}

	bool operator==(const ResourceId &other) const {
		return (_type == other._type) && (_number == other._number) && (_tuple == other._tuple);
	}

	bool operator!=(const ResourceId &other) const {
		return !operator==(other);
	}

	bool operator<(const ResourceId &other) const {
		return (_type < other._type) || ((_type == other._type) && (_number < other._number))
			    || ((_type == other._type) && (_number == other._number) && (_tuple < other._tuple));
	}
};

class ResourceSource;
class ResourceManager;

/** Class for storing resources in memory */
class Resource : public SciSpan<const byte> {
	friend class ResourceManager;
	friend class ResourcePatcher;

	// FIXME: These 'friend' declarations are meant to be a temporary hack to
	// ease transition to the ResourceSource class system.
	friend class ResourceSource;
	friend class PatchResourceSource;
	friend class WaveResourceSource;
	friend class AudioVolumeResourceSource;
	friend class MacResourceForkResourceSource;
#ifdef ENABLE_SCI32
	friend class ChunkResourceSource;
#endif

protected:
	/**
	 * Holds the extra header data from view, pic, and palette patches so that
	 * these patches can be rewritten to disk as valid patch files by the
	 * `diskdump` debugger command.
	 */
	byte *_header;
	uint32 _headerSize;

public:
	Resource(ResourceManager *resMan, ResourceId id);
	~Resource();
	void unalloc();

	inline ResourceId getId() const { return _id; }
	inline ResourceType getType() const { return _id.getType(); }
	inline uint16 getNumber() const { return _id.getNumber(); }
	bool isLocked() const { return _status == kResStatusLocked; }
	/**
	 * Write the resource to the specified stream.
	 * This method is used only by the "dump" debugger command.
	 */
	void writeToStream(Common::WriteStream *stream) const;

#ifdef ENABLE_SCI32
	Common::SeekableReadStream *makeStream() const;
#endif

	const Common::String &getResourceLocation() const;

	// FIXME: This audio specific method is a hack. After all, why should a
	// Resource have audio specific methods? But for now we keep this, as it
	// eases transition.
	uint32 getAudioCompressionType() const;

	uint16 getNumLockers() const { return _lockers; }

protected:
	ResourceId _id;	// TODO: _id could almost be made const, only readResourceInfo() modifies it...
	int32 _fileOffset; /**< Offset in file */
	ResourceStatus _status;
	uint16 _lockers; /**< Number of places where this resource was locked */
	ResourceSource *_source;
	ResourceManager *_resMan;

	bool loadPatch(Common::SeekableReadStream *file);
	bool loadFromPatchFile();
	bool loadFromWaveFile(Common::SeekableReadStream *file);
	bool loadFromAudioVolumeSCI1(Common::SeekableReadStream *file);
	bool loadFromAudioVolumeSCI11(Common::SeekableReadStream *file);
	ResourceErrorCode decompress(ResVersion volVersion, Common::SeekableReadStream *file);
	ResourceErrorCode readResourceInfo(ResVersion volVersion, Common::SeekableReadStream *file, uint32 &szPacked, ResourceCompression &compression);
};

struct ResourceIdHash : public Common::UnaryFunction<ResourceId, uint> {
	uint operator()(ResourceId val) const { return val.hash(); }
};

} // End of namespace Sci

#endif