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

#include "common/md5.h"
#include "sci/debugger.h"
#ifdef ENABLE_SCI32
#include "sci/graphics/frameout.h"
#include "sci/sound/audio32.h"
#include "sci/sound/decoders/sol.h"
#endif
#include "sci/engine/state.h"
#include "sci/resource/resource.h"
#include "sci/resource/manager.h"

namespace Sci {

Debugger::Debugger(ResourceManager *resMan, GfxFrameout *frameout, Audio32 *audio, EngineState *state) :
	GUI::Debugger(),
	_resMan(resMan),
	_gfxFrameout(frameout),
	_audio32(audio),
	_gameState(state) {

	// General
	registerCmd("help",				WRAP_METHOD(Debugger, cmdHelp));

	// Resources
	registerCmd("diskdump",			WRAP_METHOD(Debugger, cmdDiskDump));
	registerCmd("hexdump",			WRAP_METHOD(Debugger, cmdHexDump));
	registerCmd("resource_id",		WRAP_METHOD(Debugger, cmdResourceId));
	registerCmd("resource_info",		WRAP_METHOD(Debugger, cmdResourceInfo));
	registerCmd("resource_types",		WRAP_METHOD(Debugger, cmdResourceTypes));
	registerCmd("list",				WRAP_METHOD(Debugger, cmdList));
	registerCmd("alloc_list",				WRAP_METHOD(Debugger, cmdAllocList));
	registerCmd("hexgrep",			WRAP_METHOD(Debugger, cmdHexgrep));
	registerCmd("integrity_dump",	WRAP_METHOD(Debugger, cmdResourceIntegrityDump));

	// Graphics
	registerCmd("plane_list",         WRAP_METHOD(Debugger, cmdPlaneList));
	registerCmd("pl",                 WRAP_METHOD(Debugger, cmdPlaneList));	// alias
	registerCmd("visible_plane_list", WRAP_METHOD(Debugger, cmdVisiblePlaneList));
	registerCmd("vpl",                WRAP_METHOD(Debugger, cmdVisiblePlaneList));	// alias
	registerCmd("plane_items",        WRAP_METHOD(Debugger, cmdPlaneItemList));
	registerCmd("pi",                 WRAP_METHOD(Debugger, cmdPlaneItemList));	// alias
	registerCmd("visible_plane_items", WRAP_METHOD(Debugger, cmdVisiblePlaneItemList));
	registerCmd("vpi",                WRAP_METHOD(Debugger, cmdVisiblePlaneItemList));	// alias

	// Music/SFX
	registerCmd("audio_list",		WRAP_METHOD(Debugger, cmdAudioList));
	registerCmd("audio_dump",		WRAP_METHOD(Debugger, cmdAudioDump));
}

#pragma mark -
#pragma mark Command argument parsers

// Returns 0 on success
int Debugger::parse_reg_t(EngineState *s, const char *str, reg_t *dest, bool mayBeValue) const {
	// Pointer to the part of str which contains a numeric offset (if any)
	const char *offsetStr = NULL;

	// Flag that tells whether the value stored in offsetStr is an absolute offset,
	// or a relative offset against dest->offset.
	bool relativeOffset = false;

	// Non-NULL: Parse end of string for relative offsets
	char *endptr;

	if (*str == '$') { // Register: "$FOO" or "$FOO+NUM" or "$FOO-NUM
		if (!s) {
			return 1;
		}

		relativeOffset = true;

		if (!scumm_strnicmp(str + 1, "PC", 2)) {
			reg32_t pc = s->_executionStack.back().addr.pc;
			dest->setSegment(pc.getSegment());
			dest->setOffset(pc.getOffset());
			offsetStr = str + 3;
		} else if (!scumm_strnicmp(str + 1, "P", 1)) {
			reg32_t pc = s->_executionStack.back().addr.pc;
			dest->setSegment(pc.getSegment());
			dest->setOffset(pc.getOffset());
			offsetStr = str + 2;
		} else if (!scumm_strnicmp(str + 1, "PREV", 4)) {
			*dest = s->r_prev;
			offsetStr = str + 5;
		} else if (!scumm_strnicmp(str + 1, "ACC", 3)) {
			*dest = s->r_acc;
			offsetStr = str + 4;
		} else if (!scumm_strnicmp(str + 1, "A", 1)) {
			*dest = s->r_acc;
			offsetStr = str + 2;
		} else if (!scumm_strnicmp(str + 1, "OBJ", 3)) {
			*dest = s->_executionStack.back().objp;
			offsetStr = str + 4;
		} else if (!scumm_strnicmp(str + 1, "O", 1)) {
			*dest = s->_executionStack.back().objp;
			offsetStr = str + 2;
		} else
			return 1; // No matching register

		if (!*offsetStr)
			offsetStr = NULL;
		else if (*offsetStr != '+' && *offsetStr != '-')
			return 1;
	} else if (*str == '&') { // Script relative: "&SCRIPT-ID:OFFSET"
		if (!s) {
			return 1;
		}

		// Look up by script ID. The text from start till just before the colon
		// (resp. end of string, if there is no colon) contains the script ID.
		const char *colon = strchr(str, ':');
		if (!colon)
			return 1;

		// Extract the script id and parse it
		Common::String scriptStr(str, colon);
		int script_nr = strtol(scriptStr.c_str() + 1, &endptr, 10);
		if (*endptr)
			return 1;

		// Now lookup the script's segment
		dest->setSegment(s->_segMan->getScriptSegment(script_nr));
		if (!dest->getSegment()) {
			return 1;
		}

		// Finally, after the colon comes the offset
		offsetStr = colon + 1;

	} else {
		// Now we either got an object name, or segment:offset or plain value
		//  segment:offset is recognized by the ":"
		//  plain value may be "123" or "123h" or "fffh" or "0xfff"
		//  object name is assumed if nothing else matches or a "?" is used as prefix as override
		//  object name may contain "+", "-" and "." for relative calculations, those chars are used nowhere else

		// First we cycle through the string counting special chars
		const char *strLoop = str;
		int charsCount = strlen(str);
		int charsCountObject = 0;
		int charsCountSegmentOffset = 0;
		int charsCountLetter = 0;
		int charsCountNumber = 0;
		bool charsForceHex = false;
		bool charsForceObject = false;

		while (*strLoop) {
			switch (*strLoop) {
			case '+':
			case '-':
			case '.':
				charsCountObject++;
				break;
			case '?':
				if (strLoop == str) {
					charsForceObject = true;
					str++; // skip over prefix
				}
				break;
			case ':':
				charsCountSegmentOffset++;
				break;
			case 'h':
				if (*(strLoop + 1) == 0)
					charsForceHex = true;
				else
					charsCountObject++;
				break;
			case '0':
				if (*(strLoop + 1) == 'x') {
					str += 2; // skip "0x"
					strLoop++; // skip "x"
					charsForceHex = true;
				}
				charsCountNumber++;
				break;
			default:
				if ((*strLoop >= '0') && (*strLoop <= '9'))
					charsCountNumber++;
				if ((*strLoop >= 'a') && (*strLoop <= 'f'))
					charsCountLetter++;
				if ((*strLoop >= 'A') && (*strLoop <= 'F'))
					charsCountLetter++;
				if ((*strLoop >= 'i') && (*strLoop <= 'z'))
					charsCountObject++;
				if ((*strLoop >= 'I') && (*strLoop <= 'Z'))
					charsCountObject++;
				if (*strLoop == '_')	// underscores are used as substitutes for spaces in object names
					charsCountObject++;
			}
			strLoop++;
		}

		if ((charsCountObject) && (charsCountSegmentOffset))
			return 1; // input doesn't make sense

		if (!charsForceObject) {
			// input may be values/segment:offset

			if (charsCountSegmentOffset) {
				// ':' found, so must be segment:offset
				const char *colon = strchr(str, ':');

				offsetStr = colon + 1;

				Common::String segmentStr(str, colon);
				dest->setSegment(strtol(segmentStr.c_str(), &endptr, 16));
				if (*endptr)
					return 1;
			} else {
				int val = 0;
				dest->setSegment(0);

				if (charsCountNumber == charsCount) {
					// Only numbers in input, assume decimal value
					val = strtol(str, &endptr, 10);
					if (*endptr)
						return 1; // strtol failed?
					dest->setOffset(val);
					return 0;
				} else {
					// We also got letters, check if there were only hexadecimal letters and '0x' at the start or 'h' at the end
					if ((charsForceHex) && (!charsCountObject)) {
						val = strtol(str, &endptr, 16);
						if ((*endptr != 'h') && (*endptr != 0))
							return 1;
						dest->setOffset(val);
						return 0;
					} else {
						// Something else was in input, assume object name
						charsForceObject = true;
					}
				}
			}
		}

		if (charsForceObject) {
			if (!s) {
				return 1;
			}

			// We assume now that input is object name
			// Object by name: "?OBJ" or "?OBJ.INDEX" or "?OBJ.INDEX+OFFSET" or "?OBJ.INDEX-OFFSET"
			// The (optional) index can be used to distinguish multiple object with the same name.
			int index = -1;

			// Look for an offset. It starts with + or -
			relativeOffset = true;
			offsetStr = strchr(str, '+');
			if (!offsetStr)	// No + found, look for -
				offsetStr = strchr(str, '-');

			// Strip away the offset and the leading '?'
			Common::String str_objname;
			if (offsetStr)
				str_objname = Common::String(str, offsetStr);
			else
				str_objname = str;

			// Scan for a period, after which (if present) we'll find an index
			const char *tmp = Common::find(str_objname.begin(), str_objname.end(), '.');
			if (tmp != str_objname.end()) {
				index = strtol(tmp + 1, &endptr, 16);
				if (*endptr) {
					// The characters after the dot do not represent an index.
					// This can happen if an object contains a dot in its name,
					// like 'dominoes.opt' in Hoyle 3.
					index = -1;
				} else {
					// Valid index found, chop it off
					str_objname = Common::String(str_objname.c_str(), tmp);
				}
			}

			// Replace all underscores in the name with spaces
			for (uint i = 0; i < str_objname.size(); i++) {
				if (str_objname[i] == '_')
					str_objname.setChar(' ', i);
			}

			// Now all values are available; iterate over all objects.
			*dest = s->_segMan->findObjectByName(str_objname, index);
			if (dest->isNull())
				return 1;
		}
	}
	if (offsetStr) {
		int val = strtol(offsetStr, &endptr, 16);

		if (relativeOffset)
			dest->incOffset(val);
		else
			dest->setOffset(val);

		if (*endptr)
			return 1;
	}

	return 0;
}

bool Debugger::parseInteger(const char *argument, int &result) const {
	char *endPtr = 0;
	int idxLen = strlen(argument);
	const char *lastChar = argument + idxLen - (idxLen == 0 ? 0 : 1);

	if ((strncmp(argument, "0x", 2) == 0) || (*lastChar == 'h')) {
		// hexadecimal number
		result = strtol(argument, &endPtr, 16);
		if ((*endPtr != 0) && (*endPtr != 'h')) {
			debugPrintf("Invalid hexadecimal number '%s'\n", argument);
			return false;
		}
	} else {
		// decimal number
		result = strtol(argument, &endPtr, 10);
		if (*endPtr != 0) {
			debugPrintf("Invalid decimal number '%s'\n", argument);
			return false;
		}
	}
	return true;
}

ResourceType Debugger::parseResourceType(const char *resid) const {
	// Gets the resource number of a resource string, or returns -1
	ResourceType res = kResourceTypeInvalid;

	for (int i = 0; i < kResourceTypeInvalid; i++)
		if (strcmp(getResourceTypeName((ResourceType)i), resid) == 0)
			res = (ResourceType)i;

	return res;
}

bool Debugger::parseResourceNumber36(const char *userParameter, uint16 &resourceNumber, uint32 &resourceTuple) const {
	int userParameterLen = strlen(userParameter);

	if (userParameterLen != 10) {
		debugPrintf("Audio36/Sync36 resource numbers must be specified as RRRNNVVCCS\n");
		debugPrintf("where RRR is the resource number/map\n");
		debugPrintf("      NN is the noun\n");
		debugPrintf("      VV is the verb\n");
		debugPrintf("      CC is the cond\n");
		debugPrintf("      S  is the seq\n");
		return false;
	}

	// input: RRRNNVVCCS
	resourceNumber = strtol(Common::String(userParameter, 3).c_str(), 0, 36);
	uint16 noun = strtol(Common::String(userParameter + 3, 2).c_str(), 0, 36);
	uint16 verb = strtol(Common::String(userParameter + 5, 2).c_str(), 0, 36);
	uint16 cond = strtol(Common::String(userParameter + 7, 2).c_str(), 0, 36);
	uint16 seq = strtol(Common::String(userParameter + 9, 1).c_str(), 0, 36);
	resourceTuple = ((noun & 0xff) << 24) | ((verb & 0xff) << 16) | ((cond & 0xff) << 8) | (seq & 0xff);
	return true;
}

#pragma mark -
#pragma mark Resources

void Debugger::printResourcesHelp() const {
	debugPrintf(" diskdump - Dumps the specified resource to disk as a patch file\n");
	debugPrintf(" hexdump - Dumps the specified resource to standard output\n");
	debugPrintf(" resource_id - Identifies a resource number by splitting it up in resource type and resource number\n");
	debugPrintf(" resource_info - Shows info about a resource\n");
	debugPrintf(" resource_types - Shows the valid resource types\n");
	debugPrintf(" list - Lists all the resources of a given type\n");
	debugPrintf(" alloc_list - Lists all allocated resources\n");
	debugPrintf(" hexgrep - Searches some resources for a particular sequence of bytes, represented as hexadecimal numbers\n");
	debugPrintf(" verify_scripts - Performs sanity checks on SCI1.1-SCI2.1 game scripts (e.g. if they're up to 64KB in total)\n");
	debugPrintf(" integrity_dump - Dumps integrity data about resources in the current game to disk\n");
}

bool Debugger::cmdDiskDump(int argc, const char **argv) {
	bool resourceAll = false;
	uint16 resourceNumber = 0;
	uint32 resourceTuple = 0;

	if (argc != 3) {
		debugPrintf("Dumps the specified resource to disk as a patch file\n");
		debugPrintf("Usage: %s <resource type> <resource number>\n", argv[0]);
		debugPrintf("       <resource number> may be '*' to dump all resources of given type\n");
		cmdResourceTypes(argc, argv);
		return true;
	}

	ResourceType resourceType = parseResourceType(argv[1]);
	if (resourceType == kResourceTypeInvalid) {
		debugPrintf("Resource type '%s' is not valid\n", argv[1]);
		return true;
	}

	if (strcmp(argv[2], "*") == 0) {
		resourceAll = true;
	} else {
		switch (resourceType) {
		case kResourceTypeAudio36:
		case kResourceTypeSync36:
			if (!parseResourceNumber36(argv[2], resourceNumber, resourceTuple)) {
				return true;
			}
			break;
		default:
			resourceNumber = atoi(argv[2]);
			break;
		}
	}

	if (resourceType == kResourceTypeInvalid) {
		debugPrintf("Resource type '%s' is not valid\n", argv[1]);
		return true;
	}

	if (resourceAll) {
		// "*" used, dump everything of that type
		Common::List<ResourceId> resources = _resMan->listResources(resourceType);
		Common::sort(resources.begin(), resources.end());

		Common::List<ResourceId>::iterator itr;
		for (itr = resources.begin(); itr != resources.end(); ++itr) {
			resourceNumber = itr->getNumber();
			resourceTuple = itr->getTuple();
			cmdDiskDumpWorker(resourceType, resourceNumber, resourceTuple);
		}
	} else {
		// id was given, dump only this resource
		cmdDiskDumpWorker(resourceType, resourceNumber, resourceTuple);
	}

	return true;
}

void Debugger::cmdDiskDumpWorker(ResourceType resourceType, int resourceNumber, uint32 resourceTuple) {
	const char *resourceTypeName = getResourceTypeName(resourceType);
	ResourceId resourceId;
	const Resource *resource = nullptr;
	char outFileName[50];

	switch (resourceType) {
	case kResourceTypeAudio36:
	case kResourceTypeSync36: {
		resourceId = ResourceId(resourceType, resourceNumber, resourceTuple);
		resource = _resMan->findResource(resourceId, 0);
		sprintf(outFileName, "%s", resourceId.toPatchNameBase36().c_str());
		// patch filename is: [type:1 char] [map:3 chars] [noun:2 chars] [verb:2 chars] "." [cond: 2 chars] [seq:1 char]
		//  e.g. "@5EG0000.014"
		break;
	}
#ifdef ENABLE_SCI32S2
	case kResourceTypePano:
		resourceId = ResourceId(resourceType, resourceNumber);
		resource = _resMan->findResource(resourceId, false);
		sprintf(outFileName, "%d.bmp", resourceNumber);
		break;
#endif
	default:
		resourceId = ResourceId(resourceType, resourceNumber);
		resource = _resMan->findResource(resourceId, 0);
		sprintf(outFileName, "%s.%03d", resourceTypeName, resourceNumber);
		// patch filename is: [resourcetype].[resourcenumber]
		//  e.g. "Script.0"
		break;
	}

	if (resource) {
		Common::DumpFile outFile;
		outFile.open(outFileName);
		resource->writeToStream(&outFile);
		outFile.flush();
		debugPrintf("Resource %s (located in %s) has been dumped to disk\n", outFileName, resource->getResourceLocation().c_str());
	} else {
		debugPrintf("Resource %s not found\n", outFileName);
	}
}

bool Debugger::cmdHexDump(int argc, const char **argv) {
	if (argc != 3) {
		debugPrintf("Dumps the specified resource to standard output\n");
		debugPrintf("Usage: %s <resource type> <resource number>\n", argv[0]);
		cmdResourceTypes(argc, argv);
		return true;
	}

	int resNum = atoi(argv[2]);
	ResourceType res = parseResourceType(argv[1]);

	if (res == kResourceTypeInvalid)
		debugPrintf("Resource type '%s' is not valid\n", argv[1]);
	else {
		const Resource *resource = _resMan->findResource(ResourceId(res, resNum), false);
		if (resource) {
			Common::hexdump(resource->getUnsafeDataAt(0), resource->size(), 16, 0);
			debugPrintf("Resource %s.%03d has been dumped to standard output\n", argv[1], resNum);
		} else {
			debugPrintf("Resource %s.%03d not found\n", argv[1], resNum);
		}
	}

	return true;
}

bool Debugger::cmdResourceId(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Identifies a resource number by splitting it up in resource type and resource number\n");
		debugPrintf("Usage: %s <resource number>\n", argv[0]);
		return true;
	}

	int id = atoi(argv[1]);
	debugPrintf("%s.%d (0x%x)\n", getResourceTypeName((ResourceType)(id >> 11)), id & 0x7ff, id & 0x7ff);

	return true;
}

bool Debugger::cmdResourceInfo(int argc, const char **argv) {
	if (argc != 3) {
		debugPrintf("Shows information about a resource\n");
		debugPrintf("Usage: %s <resource type> <resource number>\n", argv[0]);
		return true;
	}

	int resNum = atoi(argv[2]);
	ResourceType res = parseResourceType(argv[1]);

	if (res == kResourceTypeInvalid)
		debugPrintf("Resource type '%s' is not valid\n", argv[1]);
	else {
		const Resource *resource = _resMan->findResource(ResourceId(res, resNum), false);
		if (resource) {
			debugPrintf("Resource size: %u\n", resource->size());
			debugPrintf("Resource location: %s\n", resource->getResourceLocation().c_str());
			Common::MemoryReadStream stream = resource->toStream();
			const Common::String hash = Common::computeStreamMD5AsString(stream);
			debugPrintf("Resource hash (decompressed): %s\n", hash.c_str());
		} else {
			debugPrintf("Resource %s.%03d not found\n", argv[1], resNum);
		}
	}

	return true;
}

bool Debugger::cmdResourceTypes(int argc, const char **argv) {
	debugPrintf("The %d valid resource types are:\n", kResourceTypeInvalid);
	for (int i = 0; i < kResourceTypeInvalid; i++) {
		debugPrintf("%s", getResourceTypeName((ResourceType) i));
		debugPrintf((i < kResourceTypeInvalid - 1) ? ", " : "\n");
	}

	return true;
}

bool Debugger::cmdList(int argc, const char **argv) {
	int selectedMapNumber = -1;
	Common::List<ResourceId> resources;
	Common::List<ResourceId>::iterator itr;
	int displayCount = 0;
	int currentMap = -1;

	if (argc < 2) {
		debugPrintf("Lists all the resources of a given type\n");
		cmdResourceTypes(argc, argv);
		return true;
	}

	ResourceType resourceType = parseResourceType(argv[1]);
	if (resourceType == kResourceTypeInvalid) {
		debugPrintf("Unknown resource type: '%s'\n", argv[1]);
		return true;
	}

	switch (resourceType) {
	case kResourceTypeAudio36:
	case kResourceTypeSync36:
		if (argc != 3) {
			debugPrintf("Please specify map number (-1: all maps)\n");
			return true;
		}
		selectedMapNumber = atoi(argv[2]);
		resources = _resMan->listResources(resourceType, selectedMapNumber);
		Common::sort(resources.begin(), resources.end());

		for (itr = resources.begin(); itr != resources.end(); ++itr) {
			const uint16 map = itr->getNumber();
			const uint32 resourceTuple = itr->getTuple();
			const uint16 noun = (resourceTuple >> 24) & 0xff;
			const uint16 verb = (resourceTuple >> 16) & 0xff;
			const uint16 cond = (resourceTuple >> 8) & 0xff;
			const uint16 seq = resourceTuple & 0xff;

			if (currentMap != map) {
				if (displayCount % 3)
					debugPrintf("\n");
				debugPrintf("Map %04x (%i):\n", map, map);
				currentMap = map;
				displayCount = 0;
			}

			if (displayCount % 3 == 0)
				debugPrintf("  ");

			debugPrintf("%02x %02x %02x %02x (%3i %3i %3i %3i)    ", noun, verb, cond, seq, noun, verb, cond, seq);

			if (++displayCount % 3 == 0)
				debugPrintf("\n");
		}
		break;
	default:
		resources = _resMan->listResources(resourceType);
		Common::sort(resources.begin(), resources.end());

		for (itr = resources.begin(); itr != resources.end(); ++itr) {
			debugPrintf("%8i", itr->getNumber());
			if (++displayCount % 10 == 0)
				debugPrintf("\n");
		}
		break;
	}

	debugPrintf("\n");
	return true;
}

bool Debugger::cmdResourceIntegrityDump(int argc, const char **argv) {
	if (argc < 2) {
		debugPrintf("Dumps integrity data about resources in the current game to disk.\n");
		debugPrintf("Usage: %s <filename> [<skip video file hashing>] [<skip video files altogether>]\n", argv[0]);
		return true;
	}

	Common::DumpFile outFile;
	if (!outFile.open(argv[1])) {
		debugPrintf("Failed to open output file %s.\n", argv[1]);
		return true;
	}

	const bool hashVideoFiles = argc < 3;
	const bool videoFiles = argc < 4;

	for (int i = 0; i < kResourceTypeInvalid; ++i) {
		const ResourceType resType = (ResourceType)i;

		// This will list video resources inside of resource bundles even if
		// video files are skipped, but this seems fine since those files are
		// small because they were intended to load into memory. (This happens
		// with VMDs in GK2.)
		Common::List<ResourceId> resources = _resMan->listResources(resType);

		const char *extension = "";
		if (videoFiles) {
			switch (resType) {
			case kResourceTypeRobot:
			case kResourceTypeVMD:
			case kResourceTypeDuck:
			case kResourceTypeClut: {
				extension = getResourceTypeExtension(resType);
				assert(*extension != '\0');

				const Common::String filesGlob = Common::String::format("*.%s", extension).c_str();
				Common::ArchiveMemberList files;
				const int numMatches = SearchMan.listMatchingMembers(files, filesGlob);
				if (numMatches > 0) {
					Common::ArchiveMemberList::const_iterator it;
					for (it = files.begin(); it != files.end(); ++it) {
						const uint resNo = atoi((*it)->getName().c_str());
						resources.push_back(ResourceId(resType, resNo));
					}
				}

				break;
			}
			default:
				break;
			}
		}

		if (resources.size()) {
			Common::sort(resources.begin(), resources.end());
			Common::List<ResourceId>::const_iterator it;
			debugPrintf("%s: ", getResourceTypeName(resType));
			for (it = resources.begin(); it != resources.end(); ++it) {
				Common::String statusName;
				if (resType == kResourceTypeAudio36 || resType == kResourceTypeSync36) {
					statusName = it->toPatchNameBase36();
				} else {
					statusName = Common::String::format("%d", it->getNumber());
				}

				const Common::String resourceName = it->toString();

				const Resource *resource = _resMan->findResource(*it, false);
				if (resource) {
					Common::MemoryReadStream stream = resource->toStream();
					writeIntegrityDumpLine(statusName, resourceName, outFile, &stream, resource->size(), true);
				} else if (videoFiles && *extension != '\0') {
					const Common::String fileName = Common::String::format("%u.%s", it->getNumber(), extension);
					Common::File file;
					Common::ReadStream *stream = nullptr;
					if (file.open(fileName)) {
						stream = &file;
					}
					writeIntegrityDumpLine(statusName, resourceName, outFile, stream, file.size(), hashVideoFiles);
				}
			}

			debugPrintf("\n");
		}
	}

	const char *otherVideoFiles[] = { "avi", "seq" };
	for (uint i = 0; i < ARRAYSIZE(otherVideoFiles); ++i) {
		const char *extension = otherVideoFiles[i];

		Common::ArchiveMemberList files;
		if (SearchMan.listMatchingMembers(files, Common::String::format("*.%s", extension).c_str()) > 0) {
			debugPrintf("%s: ", extension);
			Common::sort(files.begin(), files.end(), Common::ArchiveMemberListComparator());
			Common::ArchiveMemberList::const_iterator it;
			for (it = files.begin(); it != files.end(); ++it) {
				const Common::ArchiveMember &file = **it;
				Common::ScopedPtr<Common::SeekableReadStream> stream(file.createReadStream());
				writeIntegrityDumpLine(file.getName(), file.getName(), outFile, stream.get(), stream->size(), hashVideoFiles);
			}
			debugPrintf("\n");
		}
	}

	return true;
}

bool Debugger::cmdAllocList(int argc, const char **argv) {
	for (int i = 0; i < kResourceTypeInvalid; ++i) {
		Common::List<ResourceId> resources = _resMan->listResources((ResourceType)i);
		if (resources.size()) {
			Common::sort(resources.begin(), resources.end());
			bool hasAlloc = false;
			Common::List<ResourceId>::const_iterator it;
			for (it = resources.begin(); it != resources.end(); ++it) {
				const Resource *resource = _resMan->testResource(*it);
				if (resource != nullptr && resource->data() != nullptr) {
					if (hasAlloc) {
						debugPrintf(", ");
					} else {
						debugPrintf("%s: ", getResourceTypeName((ResourceType)i));
					}
					hasAlloc = true;
					debugPrintf("%u (%u locks)", resource->getId().getNumber(), resource->getNumLockers());
				}
			}
			if (hasAlloc) {
				debugPrintf("\n");
			}
		}
	}

	return true;
}

bool Debugger::cmdHexgrep(int argc, const char **argv) {
	if (argc < 4) {
		debugPrintf("Searches some resources for a particular sequence of bytes, represented as decimal or hexadecimal numbers.\n");
		debugPrintf("Usage: %s <resource type> <resource number> <search string>\n", argv[0]);
		debugPrintf("<resource number> can be a specific resource number, or \"all\" for all of the resources of the specified type\n");
		debugPrintf("EXAMPLES:\n  hexgrep script all 0xe8 0x03 0xc8 0x00\n  hexgrep pic 0x42 0xfe\n");
		cmdResourceTypes(argc, argv);
		return true;
	}

	ResourceType restype = parseResourceType(argv[1]);
	int resNumber = 0, resMax = 0;
	const Resource *script = nullptr;

	if (restype == kResourceTypeInvalid) {
		debugPrintf("Resource type '%s' is not valid\n", argv[1]);
		return true;
	}

	if (!scumm_stricmp(argv[2], "all")) {
		resNumber = 0;
		resMax = 65535;
	} else {
		resNumber = resMax = atoi(argv[2]);
	}

	// Convert the bytes
	Common::Array<int> byteString;
	byteString.resize(argc - 3);

	for (uint i = 0; i < byteString.size(); i++)
		if (!parseInteger(argv[i + 3], byteString[i]))
			return true;

	for (; resNumber <= resMax; resNumber++) {
		script = _resMan->findResource(ResourceId(restype, resNumber), false);
		if (script) {
			uint32 seeker = 0, seekerold = 0;
			uint32 comppos = 0;
			int output_script_name = 0;

			while (seeker < script->size()) {
				if (script->getUint8At(seeker) == byteString[comppos]) {
					if (comppos == 0)
						seekerold = seeker;

					comppos++;

					if (comppos == byteString.size()) {
						comppos = 0;
						seeker = seekerold + 1;

						if (!output_script_name) {
							debugPrintf("\nIn %s.%03d:\n", getResourceTypeName((ResourceType)restype), resNumber);
							output_script_name = 1;
						}
						debugPrintf("   0x%04x\n", seekerold);
					}
				} else
					comppos = 0;

				seeker++;
			}
		}
	}

	return true;
}

void Debugger::writeIntegrityDumpLine(const Common::String &statusName, const Common::String &resourceName, Common::WriteStream &out, Common::ReadStream *const data, const int size, const bool writeHash) const {
	debugPrintf("%s", statusName.c_str());

	out.writeString(resourceName);
	if (!data) {
		out.writeString(" ERROR\n");
		debugPrintf("[ERR] ");
	} else {
		out.writeString(Common::String::format(" %d ", size));
		if (writeHash) {
			out.writeString(Common::computeStreamMD5AsString(*data));
		} else {
			out.writeString("disabled");
		}
		out.writeString("\n");
		debugPrintf("[OK] ");
	}
}

#pragma mark -
#pragma mark Graphics

void Debugger::printGraphicsHelp() const {
	debugPrintf(" plane_list / pl - Shows a list of all the planes in the draw list (SCI2+)\n");
	debugPrintf(" visible_plane_list / vpl - Shows a list of all the planes in the visible draw list (SCI2+)\n");
	debugPrintf(" plane_items / pi - Shows a list of all items for a plane (SCI2+)\n");
	debugPrintf(" visible_plane_items / vpi - Shows a list of all items for a plane in the visible draw list (SCI2+)\n");
}

bool Debugger::cmdPlaneList(int argc, const char **argv) {
#ifdef ENABLE_SCI32
	if (_gfxFrameout) {
		debugPrintf("Plane list:\n");
		_gfxFrameout->printPlaneList(this);
	} else {
		debugPrintf("This SCI version does not have a list of planes\n");
	}
#else
	debugPrintf("SCI32 isn't included in this compiled executable\n");
#endif
	return true;
}

bool Debugger::cmdVisiblePlaneList(int argc, const char **argv) {
#ifdef ENABLE_SCI32
	if (_gfxFrameout) {
		debugPrintf("Visible plane list:\n");
		_gfxFrameout->printVisiblePlaneList(this);
	} else {
		debugPrintf("This SCI version does not have a list of planes\n");
	}
#else
	debugPrintf("SCI32 isn't included in this compiled executable\n");
#endif
	return true;
}


bool Debugger::cmdPlaneItemList(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Shows the list of items for a plane\n");
		debugPrintf("Usage: %s <plane address>\n", argv[0]);
		return true;
	}

	reg_t planeObject = NULL_REG;

	if (parse_reg_t(_gameState, argv[1], &planeObject, false)) {
		debugPrintf("Invalid address passed.\n");
		debugPrintf("Check the \"addresses\" command on how to use addresses\n");
		return true;
	}

#ifdef ENABLE_SCI32
	if (_gfxFrameout) {
		debugPrintf("Plane item list:\n");
		_gfxFrameout->printPlaneItemList(this, planeObject);
	} else {
		debugPrintf("This SCI version does not have a list of plane items\n");
	}
#else
	debugPrintf("SCI32 isn't included in this compiled executable\n");
#endif
	return true;
}

bool Debugger::cmdVisiblePlaneItemList(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Shows the list of items for a plane\n");
		debugPrintf("Usage: %s <plane address>\n", argv[0]);
		return true;
	}

	reg_t planeObject = NULL_REG;

	if (parse_reg_t(_gameState, argv[1], &planeObject, false)) {
		debugPrintf("Invalid address passed.\n");
		debugPrintf("Check the \"addresses\" command on how to use addresses\n");
		return true;
	}

#ifdef ENABLE_SCI32
	if (_gfxFrameout) {
		debugPrintf("Visible plane item list:\n");
		_gfxFrameout->printVisiblePlaneItemList(this, planeObject);
	} else {
		debugPrintf("This SCI version does not have a list of plane items\n");
	}
#else
	debugPrintf("SCI32 isn't included in this compiled executable\n");
#endif
	return true;
}

#pragma mark -
#pragma mark Music/SFX

void Debugger::printAudioHelp() const {
	debugPrintf(" audio_list - Lists currently active digital audio samples (SCI2+)\n");
	debugPrintf(" audio_dump - Dumps the requested audio resource as an uncompressed wave file (SCI2+)\n");
}

bool Debugger::cmdAudioList(int argc, const char **argv) {
#ifdef ENABLE_SCI32
	if (_audio32) {
		debugPrintf("Audio list (%d active channels):\n", _audio32->getNumActiveChannels());
		_audio32->printAudioList(this);
	} else {
		debugPrintf("This SCI version does not have a software digital audio mixer\n");
	}
#else
	debugPrintf("SCI32 isn't included in this compiled executable\n");
#endif

	return true;
}

bool Debugger::cmdAudioDump(int argc, const char **argv) {
#ifdef ENABLE_SCI32
	if (argc != 2 && argc != 6) {
		debugPrintf("Dumps the requested audio resource as an uncompressed wave file.\n");
		debugPrintf("Usage (audio): %s <audio resource id>\n", argv[0]);
		debugPrintf("Usage (audio36): %s <audio map id> <noun> <verb> <cond> <seq>\n", argv[0]);
		return true;
	}

	ResourceId id;
	if (argc == 2) {
		id = ResourceId(kResourceTypeAudio, atoi(argv[1]));
	} else {
		id = ResourceId(kResourceTypeAudio36, atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
	}

	const Resource *resource = _resMan->findResource(id, false);
	if (!resource) {
		debugPrintf("Not found.\n");
		return true;
	}

	Common::MemoryReadStream stream = resource->toStream();

	Common::DumpFile outFile;
	const Common::String fileName = Common::String::format("%s.wav", id.toString().c_str());
	if (!outFile.open(fileName)) {
		debugPrintf("Could not open dump file %s.\n", fileName.c_str());
		return true;
	}

	const bool isSol = detectSolAudio(stream);
	const bool isWave = !isSol && detectWaveAudio(stream);
	const bool isRaw = !isSol && !isWave;

	if (isSol || isRaw) {
		uint16 sampleRate = 11025;
		int numChannels = 1;
		int bytesPerSample = 1;
		bool sourceIs8Bit = true;
		uint32 compressedSize;
		uint32 decompressedSize;

		if (isSol) {
			stream.seek(6, SEEK_SET);
			sampleRate = stream.readUint16LE();
			const byte flags = stream.readByte();
			compressedSize = stream.readUint32LE();

			// All AudioStreams must output 16-bit samples
			bytesPerSample = 2;

			decompressedSize = compressedSize;

			if (flags & kCompressed) {
				decompressedSize *= 2;
			}
			if (flags & k16Bit) {
				sourceIs8Bit = false;
			} else {
				// 8-bit is implicitly up-converted by AudioStream to 16-bit
				decompressedSize *= 2;
			}
			if (flags & kStereo) {
				numChannels = 2;
			}
		} else {
			decompressedSize = resource->size();
		}

		enum {
			kWaveHeaderSize = 36
		};

		outFile.writeString("RIFF");
		outFile.writeUint32LE(kWaveHeaderSize + decompressedSize);
		outFile.writeString("WAVEfmt ");
		outFile.writeUint32LE(16);
		outFile.writeUint16LE(1);
		outFile.writeUint16LE(numChannels);
		outFile.writeUint32LE(sampleRate);
		outFile.writeUint32LE(sampleRate * bytesPerSample * numChannels);
		outFile.writeUint16LE(bytesPerSample * numChannels);
		outFile.writeUint16LE(bytesPerSample * 8);
		outFile.writeString("data");
		outFile.writeUint32LE(decompressedSize);

		if (isSol) {
			stream.seek(0, SEEK_SET);
			Common::ScopedPtr<Audio::SeekableAudioStream> audioStream(makeSOLStream(&stream, DisposeAfterUse::NO));

			if (!audioStream) {
				debugPrintf("Could not create SOL stream.\n");
				return true;
			}

			byte buffer[4096];
			const int samplesToRead = ARRAYSIZE(buffer) / 2;
			uint bytesWritten = 0;
			int samplesRead;
			while ((samplesRead = audioStream->readBuffer((int16 *)buffer, samplesToRead))) {
				uint bytesToWrite = samplesRead * bytesPerSample;
				outFile.write(buffer, bytesToWrite);
				bytesWritten += bytesToWrite;
			}

			if (bytesWritten != decompressedSize) {
				debugPrintf("WARNING: Should have written %u bytes but wrote %u bytes!\n", decompressedSize, bytesWritten);
				while (bytesWritten < decompressedSize) {
					outFile.writeByte(0);
					++bytesWritten;
				}
			}

			const char *bits;
			if (sourceIs8Bit) {
				bits = "upconverted 16";
			} else {
				bits = "16";
			}

			debugPrintf("%s-bit %uHz %d-channel SOL audio, %u -> %u bytes\n", bits, sampleRate, numChannels, compressedSize, decompressedSize);
		} else {
			outFile.write(resource->data(), resource->size());
			debugPrintf("%d-bit %uHz %d-channel raw audio, %u bytes\n", bytesPerSample * 8, sampleRate, numChannels, decompressedSize);
		}
	} else if (isWave) {
		outFile.write(resource->data(), resource->size());
		debugPrintf("Raw wave file\n");
	} else {
		error("Impossible situation");
	}

	debugPrintf("Written to %s successfully.\n", fileName.c_str());
#else
	debugPrintf("SCI32 isn't included in this compiled executable\n");
#endif
	return true;
}

} // End of namespace Sci
