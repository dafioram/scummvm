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

#include "common/archive.h"
#include "sci/resource/sources/directory.h"
#include "sci/resource/sources/patch.h"
#include "sci/resource/sources/wave.h"
#include "sci/resource/manager.h"

namespace Sci {

bool DirectoryResourceSource::scanSource(ResourceManager *resMan) {
	readResourcePatches(resMan);

	// We can't use getSciVersion() at this point, thus using _volVersion
	if (resMan->getVolVersion() >= kResVersionSci11)	// SCI1.1+
		readResourcePatchesBase36(resMan);

	readWaveAudioPatches(resMan);
	return true;
}

void DirectoryResourceSource::readResourcePatches(ResourceManager *resMan) const {
	// Since some SCI1 games (KQ5 floppy, SQ4) use SCI0 naming scheme for patch
	// files, this function tries to read patch file with any supported naming
	// scheme regardless of SCI version

	Common::String mask, name;
	Common::ArchiveMemberList files;
	uint16 resourceNr = 0;
	const char *szResType;
	const bool shouldUseSci0 = shouldFindSci0Patches(resMan);

	for (int i = kResourceTypeView; i < kResourceTypeInvalid; ++i) {
		const ResourceType type = ResourceType(i);

		if (*getResourceTypeExtension(type) == '\0' || (i >= kResourceTypeRobot && i != kResourceTypeChunk))
			continue;

		files.clear();
		if (shouldUseSci0) {
			szResType = getResourceTypeName(type);
			// SCI0 naming - type.nnn
			mask = szResType;
			mask += ".???";
			SearchMan.listMatchingMembers(files, mask);
		}
		// SCI1 and later naming - nnn.typ
		mask = "*.";
		mask += getResourceTypeExtension(type);
		SearchMan.listMatchingMembers(files, mask);

		if (i == kResourceTypeView) {
			SearchMan.listMatchingMembers(files, "*.v16");	// EGA SCI1 view patches
			SearchMan.listMatchingMembers(files, "*.v32");	// Amiga SCI1 view patches
			SearchMan.listMatchingMembers(files, "*.v64");	// Amiga AGA SCI1 (i.e. Longbow) view patches
		} else if (i == kResourceTypePic) {
			SearchMan.listMatchingMembers(files, "*.p16");	// EGA SCI1 picture patches
			SearchMan.listMatchingMembers(files, "*.p32");	// Amiga SCI1 picture patches
			SearchMan.listMatchingMembers(files, "*.p64");	// Amiga AGA SCI1 (i.e. Longbow) picture patches
		} else if (i == kResourceTypeScript) {
			// SCI3 (we can't use getSciVersion() at this point)
			SearchMan.listMatchingMembers(files, "*.csc");
		}

		for (Common::ArchiveMemberList::const_iterator x = files.begin(); x != files.end(); ++x) {
			bool bAdd = false;
			name = (*x)->getName();

			// SCI1 scheme
			if (Common::isDigit(name[0])) {
				char *end = 0;
				resourceNr = strtol(name.c_str(), &end, 10);
				bAdd = (*end == '.'); // Ensure the next character is the period
			} else if (shouldUseSci0) {
				// SCI0 scheme
				int resname_len = strlen(szResType);
				if (scumm_strnicmp(name.c_str(), szResType, resname_len) == 0
					&& !Common::isAlpha(name[resname_len + 1])) {
					resourceNr = atoi(name.c_str() + resname_len + 1);
					bAdd = true;
				}
			}

			if (bAdd) {
				PatchResourceSource *psrcPatch = new PatchResourceSource(name);
				if (!psrcPatch->processPatch(resMan, ResourceType(i), resourceNr)) {
					delete psrcPatch;
				}
			}
		}
	}
}

void DirectoryResourceSource::readResourcePatchesBase36(ResourceManager *resMan) const {
	// The base36 encoded audio36 and sync36 resources use a different naming scheme, because they
	// cannot be described with a single resource number, but are a result of a
	// <number, noun, verb, cond, seq> tuple. Please don't be confused with the normal audio patches
	// (*.aud) and normal sync patches (*.syn). audio36 patches can be seen for example in the AUD
	// folder of GK1CD, and are like this file: @0CS0M00.0X1. GK1CD is the first game where these
	// have been observed. The actual audio36 and sync36 resources exist in SCI1.1 as well, but the
	// first game where external patch files for them have been found is GK1CD. The names of these
	// files are base36 encoded, and we handle their decoding here. audio36 files start with a '@',
	// whereas sync36 start with a '#'. Mac versions begin with 'A' (probably meaning AIFF). Torin
	// has several that begin with 'B'.

	Common::String name, inputName;
	Common::ArchiveMemberList files;

	for (int i = kResourceTypeAudio36; i <= kResourceTypeSync36; ++i) {
		files.clear();

		// audio36 resources start with a @, A, or B
		// sync36 resources start with a #, S, or T
		if (i == kResourceTypeAudio36) {
			SearchMan.listMatchingMembers(files, "@???????.???");
			SearchMan.listMatchingMembers(files, "A???????.???");
			SearchMan.listMatchingMembers(files, "B???????.???");
		} else {
			SearchMan.listMatchingMembers(files, "#???????.???");
#ifdef ENABLE_SCI32
			SearchMan.listMatchingMembers(files, "S???????.???");
			SearchMan.listMatchingMembers(files, "T???????.???");
#endif
		}

		for (Common::ArchiveMemberList::const_iterator x = files.begin(); x != files.end(); ++x) {
			name = (*x)->getName();
			name.toUppercase();

			// The S/T prefixes often conflict with non-patch files and generate
			// spurious warnings about invalid patches
			if (name.hasSuffix(".DLL") || name.hasSuffix(".EXE") || name.hasSuffix(".TXT") || name.hasSuffix(".OLD") || name.hasSuffix(".WIN") || name.hasSuffix(".DOS") ||
				name.hasSuffix(".HLP") || name.hasSuffix(".DRV")) {
				continue;
			}

			ResourceId resource36 = convertPatchNameBase36((ResourceType)i, name);

			// Make sure that the audio patch is a valid resource
			if (i == kResourceTypeAudio36) {
				Common::ScopedPtr<Common::SeekableReadStream> stream(SearchMan.createReadStreamForMember(name));
				uint32 tag = stream->readUint32BE();

				if (tag == MKTAG('R','I','F','F') || tag == MKTAG('F','O','R','M')) {
					processWavePatch(resMan, resource36, name);
					continue;
				}

				// Check for SOL as well
				tag = (tag << 16) | stream->readUint16BE();

				if (tag != MKTAG('S','O','L',0)) {
					continue;
				}
			}

			PatchResourceSource *psrcPatch = new PatchResourceSource(name);
			if (!psrcPatch->processPatch(resMan, ResourceType(i), resource36.getNumber(), resource36.getTuple())) {
				delete psrcPatch;
			}
		}
	}
}

void DirectoryResourceSource::readWaveAudioPatches(ResourceManager *resMan) const {
	// Here we do check for SCI1.1+ so we can patch wav files in as audio resources
	Common::ArchiveMemberList files;
	SearchMan.listMatchingMembers(files, "*.wav");

	for (Common::ArchiveMemberList::const_iterator x = files.begin(); x != files.end(); ++x) {
		Common::String name = (*x)->getName();

		if (Common::isDigit(name[0])) {
			processWavePatch(resMan, ResourceId(kResourceTypeAudio, atoi(name.c_str())), name);
		}
	}
}

bool DirectoryResourceSource::shouldFindSci0Patches(const ResourceManager *resMan) const {
	const GameMetadata &game = resMan->getGameMetadata();
	if (game.isCD && game.id == GID_MOTHERGOOSE256) {
		return false;
	}

	return true;
}

void DirectoryResourceSource::processWavePatch(ResourceManager *resMan, const ResourceId &resourceId, const Common::String &name) const {
	ResourceSource *resSrc = new WaveResourceSource(name);
	Common::File file;
	file.open(name);

	resMan->updateResource(resourceId, resSrc, 0, file.size(), name);
	resMan->addSource(resSrc);

	debugC(1, kDebugLevelResMan, "Patching %s - OK", name.c_str());
}

} // End of namespace Sci
