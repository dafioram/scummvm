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
#include "sci/resource/source.h"

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

Resource::Resource(ResourceId id) :
	SciSpan<const byte>(nullptr, 0, id.toString()),
	_id(id),
	_fileOffset(0),
	_status(kResStatusNoMalloc),
	_lockers(0),
	_source(nullptr),
	_header(nullptr),
	_headerSize(0) {}

Resource::~Resource() {
	delete[] _data;
	delete[] _header;
	if (_source && _source->getSourceType() == kSourcePatch) {
		delete _source;
	}
}

void Resource::unalloc() {
	delete[] _data;
	_data = nullptr;
	_status = kResStatusNoMalloc;
}

void Resource::writeToStream(Common::WriteStream *stream) const {
	stream->writeByte(_id.getType() | 0x80); // 0x80 is required by old Sierra SCI, otherwise it wont accept the patch file
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

} // End of namespace Sci
