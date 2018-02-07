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
#include "sci/resource/resource.h"
#include "sci/resource/sources.h"

namespace Sci {

static const char *const s_errorDescriptions[] = {
	"No error",
	"I/O error",
	"Resource is empty (size 0)",
	"resource.map entry is invalid",
	"resource.map file not found",
	"No resource files found",
	"Unknown compression method",
	"Decompression failed: Sanity check failed",
	"Decompression failed: Resource too big"
};

const char *getResourceErrorDescription(ResourceErrorCode code) {
	assert(code < ARRAYSIZE(s_errorDescriptions));
	return s_errorDescriptions[code];
}

static const char *const s_resourceTypeNames[] = {
	"view", "pic", "script", "text", "sound",
	"memory", "vocab", "font", "cursor",
	"patch", "bitmap", "palette", "cdaudio",
	"audio", "sync", "message", "map", "heap",
	"audio36", "sync36", "xlate", "robot", "vmd",
	"chunk", "animation", "etc", "duck", "clut",
	"tga", "zzz", "macibin", "macibis", "macpict",
	"rave"
};

const char *getResourceTypeName(ResourceType restype) {
	if (restype < ARRAYSIZE(s_resourceTypeNames))
		return s_resourceTypeNames[restype];
	else
		return "invalid";
}

// Resource type suffixes. Note that the
// suffix of SCI3 scripts has been changed from
// scr to csc
static const char *const s_resourceTypeSuffixes[] = {
	"v56", "p56", "scr", "tex", "snd",
	   "", "voc", "fon", "cur", "pat",
	"bit", "pal", "cda", "aud", "syn",
	"msg", "map", "hep",    "",    "",
	"trn", "rbt", "vmd", "chk",    "",
	"etc", "duk", "clu", "tga", "zzz",
	   "",    "",    "", ""
};

const char *getResourceTypeExtension(ResourceType restype) {
	if (restype < ARRAYSIZE(s_resourceTypeSuffixes))
		return s_resourceTypeSuffixes[restype];
	else
		return "";
}

Common::String ResourceId::toPatchNameBase36() const {
	Common::String output;

	if (getSciVersion() >= SCI_VERSION_2) {
		output += (getType() == kResourceTypeAudio36) ? 'A' : 'S'; // Identifier
	} else {
		output += (getType() == kResourceTypeAudio36) ? '@' : '#'; // Identifier
	}
	output += intToBase36(getNumber(), 3);                     // Map
	output += intToBase36(getTuple() >> 24, 2);                // Noun
	output += intToBase36((getTuple() >> 16) & 0xff, 2);       // Verb
	output += '.';                                                   // Separator
	output += intToBase36((getTuple() >> 8) & 0xff, 2);        // Cond
	output += intToBase36(getTuple() & 0xff, 1);               // Seq

	assert(output.size() == 12); // We should always get 12 characters in the end
	return output;
}

Resource::Resource(ResourceManager *resMan, ResourceId id) : SciSpan<const byte>(nullptr, 0, id.toString()), _resMan(resMan), _id(id) {
	_fileOffset = 0;
	_status = kResStatusNoMalloc;
	_lockers = 0;
	_source = nullptr;
	_header = nullptr;
	_headerSize = 0;
}

Resource::~Resource() {
	delete[] _data;
	delete[] _header;
	if (_source && _source->getSourceType() == kSourcePatch)
		delete _source;
}

void Resource::unalloc() {
	delete[] _data;
	_data = nullptr;
	_status = kResStatusNoMalloc;
}

void Resource::writeToStream(Common::WriteStream *stream) const {
	stream->writeByte(getType() | 0x80); // 0x80 is required by old Sierra SCI, otherwise it wont accept the patch file
	stream->writeByte(_headerSize);
	if (_headerSize > 0)
		stream->write(_header, _headerSize);
	stream->write(_data, _size);
}

#ifdef ENABLE_SCI32
Common::SeekableReadStream *Resource::makeStream() const {
	return new Common::MemoryReadStream(_data, _size, DisposeAfterUse::NO);
}
#endif

const Common::String &Resource::getResourceLocation() const {
	return _source->getLocationName();
}

uint32 Resource::getAudioCompressionType() const {
	return _source->getAudioCompressionType();
}

bool Resource::loadPatch(Common::SeekableReadStream *file) {
	// We assume that the resource type matches `type`
	//  We also assume that the current file position is right at the actual data (behind resourceid/headersize byte)

	byte *ptr = new byte[size()];
	_data = ptr;

	if (_headerSize > 0)
		_header = new byte[_headerSize];

	if (data() == nullptr || (_headerSize > 0 && _header == nullptr)) {
		error("Can't allocate %u bytes needed for loading %s", size() + _headerSize, _id.toString().c_str());
	}

	uint32 bytesRead;
	if (_headerSize > 0) {
		bytesRead = file->read(_header, _headerSize);
		if (bytesRead != _headerSize)
			error("Read %d bytes from %s but expected %d", bytesRead, _id.toString().c_str(), _headerSize);
	}

	bytesRead = file->read(ptr, size());
	if (bytesRead != size())
		error("Read %d bytes from %s but expected %u", bytesRead, _id.toString().c_str(), size());

	_status = kResStatusAllocated;
	return true;
}

bool Resource::loadFromPatchFile() {
	Common::File file;
	const Common::String &filename = _source->getLocationName();
	if (!file.open(filename)) {
		warning("Failed to open patch file %s", filename.c_str());
		unalloc();
		return false;
	}
	file.seek(0, SEEK_SET);
	return loadPatch(&file);
}

bool Resource::loadFromWaveFile(Common::SeekableReadStream *file) {
	byte *ptr = new byte[_size];
	_data = ptr;

	uint32 bytesRead = file->read(ptr, _size);
	if (bytesRead != _size)
		error("Read %d bytes from %s but expected %u", bytesRead, _id.toString().c_str(), _size);

	_status = kResStatusAllocated;
	return true;
}

bool Resource::loadFromAudioVolumeSCI1(Common::SeekableReadStream *file) {
	byte *ptr = new byte[size()];
	_data = ptr;

	if (!ptr) {
		error("Can't allocate %u bytes needed for loading %s", _size, _id.toString().c_str());
	}

	uint32 bytesRead = file->read(ptr, size());
	if (bytesRead != size())
		warning("Read %d bytes from %s but expected %u", bytesRead, _id.toString().c_str(), _size);

	_status = kResStatusAllocated;
	return true;
}

bool Resource::loadFromAudioVolumeSCI11(Common::SeekableReadStream *file) {
	// Check for WAVE files here
	uint32 riffTag = file->readUint32BE();
	if (riffTag == MKTAG('R','I','F','F')) {
		_size = file->readUint32LE() + 8;
		file->seek(-8, SEEK_CUR);
		return loadFromWaveFile(file);
	}
	file->seek(-4, SEEK_CUR);

	// Rave-resources (King's Quest 6) don't have any header at all
	if (getType() != kResourceTypeRave) {
		ResourceType type = _resMan->convertResType(file->readByte());

		if (((getType() == kResourceTypeAudio || getType() == kResourceTypeAudio36) && (type != kResourceTypeAudio))
			|| ((getType() == kResourceTypeSync || getType() == kResourceTypeSync36) && (type != kResourceTypeSync))) {
			warning("Resource type mismatch loading %s", _id.toString().c_str());
			unalloc();
			return false;
		}

		const uint8 headerSize = file->readByte();

		if (type == kResourceTypeAudio) {
			if (headerSize != 7 && headerSize != 11 && headerSize != 12) {
				warning("Unsupported audio header size %d in %s", headerSize, _id.toString().c_str());
				unalloc();
				return false;
			}

			if (headerSize != 7) { // Size is defined already from the map
				// Load sample size
				file->seek(7, SEEK_CUR);
				_size = file->readUint32LE() + headerSize + kResourceHeaderSize;
				if (file->err() || file->eos()) {
					warning("Error while reading size of %s", _id.toString().c_str());
					unalloc();
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
	return loadPatch(file);
}

ResourceErrorCode Resource::decompress(ResVersion volVersion, Common::SeekableReadStream *file) {
	ResourceErrorCode errorNum;
	uint32 szPacked = 0;
	ResourceCompression compression = kCompUnknown;

	// fill resource info
	errorNum = readResourceInfo(volVersion, file, szPacked, compression);
	if (errorNum)
		return errorNum;

	if (_size > kMaxResourceSize) {
		return SCI_ERROR_RESOURCE_TOO_BIG;
	}

	// getting a decompressor
	Common::ScopedPtr<Decompressor> dec;
	switch (compression) {
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
		dec.reset(new DecompressorLZW(compression));
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
		error("Resource %s: Compression method %d not supported", _id.toString().c_str(), compression);
	}

	byte *ptr = new byte[_size];
	_data = ptr;
	_status = kResStatusAllocated;
	if (!_data) {
		errorNum = SCI_ERROR_RESOURCE_TOO_BIG;
	} else if (dec->unpack(file, ptr, szPacked, _size)) {
		errorNum = SCI_ERROR_DECOMPRESSION_ERROR;
	} else {
		errorNum = SCI_ERROR_NONE;
	}

	if (errorNum != SCI_ERROR_NONE) {
		unalloc();
	} else {
		// At least Lighthouse puts sound effects in RESSCI.00n/RESSCI.PAT
		// instead of using a RESOURCE.SFX
		if (getType() == kResourceTypeAudio) {
			const uint8 headerSize = _data[1];
			if (headerSize < 11) {
				error("Unexpected audio header size for %s: should be >= 11, but got %d", _id.toString().c_str(), headerSize);
			}
			const uint32 audioSize = READ_LE_UINT32(_data + 9);
			const uint32 calculatedTotalSize = audioSize + headerSize + kResourceHeaderSize;
			if (calculatedTotalSize != _size) {
				warning("Unexpected audio file size: the size of %s in %s is %d, but the volume says it should be %d", _id.toString().c_str(), _source->getLocationName().c_str(), calculatedTotalSize, _size);
			}
			_size = MIN(_size - kResourceHeaderSize, headerSize + audioSize);
		}
	}

	return errorNum;
}

ResourceErrorCode Resource::readResourceInfo(ResVersion volVersion, Common::SeekableReadStream
											 *file,
                                      uint32 &szPacked, ResourceCompression &compression) {
	// SCI0 volume format:  {wResId wPacked+4 wUnpacked wCompression} = 8 bytes
	// SCI1 volume format:  {bResType wResNumber wPacked+4 wUnpacked wCompression} = 9 bytes
	// SCI1.1 volume format:  {bResType wResNumber wPacked wUnpacked wCompression} = 9 bytes
	// SCI32 volume format :  {bResType wResNumber dwPacked dwUnpacked wCompression} = 13 bytes
	uint16 w, number;
	uint32 wCompression, szUnpacked;
	ResourceType type;

	if (file->size() == 0)
		return SCI_ERROR_EMPTY_RESOURCE;

	switch (volVersion) {
	case kResVersionSci0Sci1Early:
	case kResVersionSci1Middle:
		w = file->readUint16LE();
		type = _resMan->convertResType(w >> 11);
		number = w & 0x7FF;
		szPacked = file->readUint16LE() - 4;
		szUnpacked = file->readUint16LE();
		wCompression = file->readUint16LE();
		break;
	case kResVersionSci1Late:
		type = _resMan->convertResType(file->readByte());
		number = file->readUint16LE();
		szPacked = file->readUint16LE() - 4;
		szUnpacked = file->readUint16LE();
		wCompression = file->readUint16LE();
		break;
	case kResVersionSci11:
		type = _resMan->convertResType(file->readByte());
		number = file->readUint16LE();
		szPacked = file->readUint16LE();
		szUnpacked = file->readUint16LE();
		wCompression = file->readUint16LE();
		break;
#ifdef ENABLE_SCI32
	case kResVersionSci2:
	case kResVersionSci3:
		type = _resMan->convertResType(file->readByte());
		number = file->readUint16LE();
		szPacked = file->readUint32LE();
		szUnpacked = file->readUint32LE();

		// The same comment applies here as in
		// detectVolVersion regarding SCI3. We ignore the
		// compression field for SCI3 games, but must presume
		// it exists in the file.
		wCompression = file->readUint16LE();

		if (volVersion == kResVersionSci3)
			wCompression = szPacked != szUnpacked ? 32 : 0;

		break;
#endif
	default:
		return SCI_ERROR_RESMAP_INVALID_ENTRY;
	}

	// check if there were errors while reading
	if ((file->eos() || file->err()))
		return SCI_ERROR_IO_ERROR;

	_id = ResourceId(type, number);
	_size = szUnpacked;

	// checking compression method
	switch (wCompression) {
	case 0:
		compression = kCompNone;
		break;
	case 1:
		compression = (getSciVersion() <= SCI_VERSION_01) ? kCompLZW : kCompHuffman;
		break;
	case 2:
		compression = (getSciVersion() <= SCI_VERSION_01) ? kCompHuffman : kCompLZW1;
		break;
	case 3:
		compression = kCompLZW1View;
		break;
	case 4:
		compression = kCompLZW1Pic;
		break;
	case 18:
	case 19:
	case 20:
		compression = kCompDCL;
		break;
#ifdef ENABLE_SCI32
	case 32:
		compression = kCompSTACpack;
		break;
#endif
	default:
		compression = kCompUnknown;
		return SCI_ERROR_UNKNOWN_COMPRESSION;
	}

	return SCI_ERROR_NONE;
}

} // End of namespace Sci
