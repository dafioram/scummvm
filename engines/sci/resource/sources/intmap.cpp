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

#include "sci/resource/sources/intmap.h"
#include "sci/resource/manager.h"

namespace Sci {

bool IntMapResourceSource::scanSource(ResourceManager *resMan) {
	return readAudioMapSCI11(resMan) == SCI_ERROR_NONE;
}

// Early SCI1.1 65535.MAP structure (uses RESOURCE.AUD):
// =========
// 6-byte entries:
// w nEntry
// dw offset

// Late SCI1.1 65535.MAP structure (uses RESOURCE.SFX):
// =========
// 5-byte entries:
// w nEntry
// tb offset (cumulative)

// QFG3 Demo 0.MAP structure:
// =========
// 10-byte entries:
// w nEntry
// dw offset
// dw size

// LB2 Floppy/Mother Goose SCI1.1 0.MAP structure:
// =========
// 8-byte entries:
// w nEntry
// w 0xffff
// dw offset

// Early SCI1.1 MAP structure:
// ===============
// 10-byte entries:
// b noun
// b verb
// b cond
// b seq
// dw offset
// w syncSize + syncAscSize

// Late SCI1.1 MAP structure:
// ===============
// Header:
// dw baseOffset
// Followed by 7 or 11-byte entries:
// b noun
// b verb
// b cond
// b seq
// tb cOffset (cumulative offset)
// w syncSize (iff seq has bit 7 set)
// w syncAscSize (iff seq has bit 6 set)

ResourceErrorCode IntMapResourceSource::readAudioMapSCI11(ResourceManager *resMan) const {
#ifndef ENABLE_SCI32
	// SCI32 support is not built in. Check if this is a SCI32 game
	// and if it is abort here.
	if (resMan->getVolVersion() >= kResVersionSci2)
		return SCI_ERROR_RESMAP_NOT_FOUND;
#endif

	uint32 offset = 0;
	const ResourceId mapResId(kResourceTypeMap, _mapNumber);
	const Resource *mapRes = resMan->findResource(mapResId, false);

	if (!mapRes) {
		warning("Failed to open %s", mapResId.toString().c_str());
		return SCI_ERROR_RESMAP_NOT_FOUND;
	}

	const ResourceSource *src = resMan->findVolumeForMap(this, _volumeNumber);

	if (!src) {
		warning("Failed to find volume for %s", mapResId.toString().c_str());
		resMan->forcePurge(mapResId);
		return SCI_ERROR_NO_RESOURCE_FILES_FOUND;
	}

	Common::SeekableReadStream *fileStream = resMan->getVolumeFile(src);

	if (!fileStream) {
		warning("Failed to open file stream for %s", src->getLocationName().c_str());
		resMan->forcePurge(mapResId);
		return SCI_ERROR_NO_RESOURCE_FILES_FOUND;
	}

	const uint32 srcSize = fileStream->size();
	resMan->disposeVolumeFileStream(fileStream, src);

	SciSpan<const byte>::const_iterator ptr = mapRes->cbegin();

	uint32 entrySize = 0;
	if (resMan->getVolVersion() >= kResVersionSci2) {
		// The heuristic size detection is incompatible with at least Torin RU,
		// which is fine because it is not needed for SCI32
		entrySize = 11;
	} else {
		// Heuristic to detect entry size
		for (int i = mapRes->size() - 1; i >= 0; --i) {
			if (ptr[i] == 0xff)
				entrySize++;
			else
				break;
		}
	}

	if (_mapNumber == kSfxModule) {
		while (ptr != mapRes->cend()) {
			uint16 n = ptr.getUint16LE();
			ptr += 2;

			if (n == 0xffff)
				break;

			if (entrySize == 6) {
				offset = ptr.getUint32LE();
				ptr += 4;
			} else {
				offset += ptr.getUint24LE();
				ptr += 3;
			}

			resMan->addResource(ResourceId(kResourceTypeAudio, n), src, offset, 0, getLocationName());
		}
	} else if (_mapNumber == 0 && entrySize == 10 && ptr[3] == 0) {
		// QFG3 demo format
		// ptr[3] would be 'seq' in the normal format and cannot possibly be 0
		while (ptr != mapRes->cend()) {
			uint16 n = ptr.getUint16BE();
			ptr += 2;

			if (n == 0xffff)
				break;

			offset = ptr.getUint32LE();
			ptr += 4;
			uint32 size = ptr.getUint32LE();
			ptr += 4;

			resMan->addResource(ResourceId(kResourceTypeAudio, n), src, offset, size, getLocationName());
		}
	} else if (_mapNumber == 0 && entrySize == 8 && (ptr + 2).getUint16LE() == 0xffff) {
		// LB2 Floppy/Mother Goose SCI1.1 format
		Common::SeekableReadStream *stream = resMan->getVolumeFile(src);

		while (ptr != mapRes->cend()) {
			uint16 n = ptr.getUint16LE();
			ptr += 4;

			if (n == 0xffff)
				break;

			const ResourceId audioResId(kResourceTypeAudio, n);

			offset = ptr.getUint32LE();
			ptr += 4;

			uint32 size;
			if (src->getAudioCompressionType() == 0) {
				// The size is not stored in the map and the entries have no order.
				// We need to dig into the audio resource in the volume to get the size.
				stream->seek(offset + 1);
				byte headerSize = stream->readByte();
				if (headerSize != 11 && headerSize != 12) {
					error("Unexpected header size in %s: should be 11 or 12, got %d", audioResId.toString().c_str(), headerSize);
				}

				stream->skip(7);
				size = stream->readUint32LE() + headerSize + 2;
			} else {
				size = 0;
			}
			resMan->addResource(audioResId, src, offset, size, getLocationName());
		}

		resMan->disposeVolumeFileStream(stream, src);
	} else {
		// EQ1CD & SQ4CD are "early" games; KQ6CD and all SCI32 are "late" games
		const bool isEarly = (entrySize != 11);

		if (!isEarly) {
			offset = ptr.getUint32LE();
			ptr += 4;
		}

		enum {
			kRaveFlag = 0x40,
			kSyncFlag = 0x80,
			kEndOfMapFlag = 0xFF
		};

		while (ptr != mapRes->cend()) {
			uint32 n = ptr.getUint32BE();
			uint32 syncSize = 0;
			ptr += 4;

			// Checking the entire tuple breaks Torin RU and is not how SSCI
			// works
			if ((n & kEndOfMapFlag) == kEndOfMapFlag) {
				const uint32 bytesLeft = mapRes->cend() - ptr;
				if (bytesLeft >= entrySize) {
					warning("End of %s reached, but %u entries remain", mapResId.toString().c_str(), bytesLeft / entrySize);
				}
				break;
			}

			if (isEarly) {
				offset = ptr.getUint32LE();
				ptr += 4;
			} else {
				offset += ptr.getUint24LE();
				ptr += 3;
			}

			if (isEarly || (n & kSyncFlag)) {
				syncSize = ptr.getUint16LE();
				ptr += 2;

				// FIXME: The sync36 resource seems to be two bytes too big in KQ6CD
				// (bytes taken from the RAVE resource right after it)
				if (syncSize > 0) {
					resMan->addResource(ResourceId(kResourceTypeSync36, _mapNumber, n & 0xffffff3f), src, offset, syncSize, getLocationName());
				}
			}

			// Checking for this 0x40 flag breaks at least Laura Bow 2 CD 1.1
			// map 448
			if (g_sci->getGameId() == GID_KQ6 && (n & kRaveFlag)) {
				// This seems to define the size of raw lipsync data (at least
				// in KQ6 CD Windows).
				uint32 kq6HiresSyncSize = ptr.getUint16LE();
				ptr += 2;

				if (kq6HiresSyncSize > 0) {
					// Rave resources do not have separate entries in the audio
					// map (their data was just appended to sync resources), so
					// we have to add the resource without validation, otherwise
					// offset validation will fail for compressed volumes (since
					// the relocation table in a compressed volume only contains
					// offsets that existed in the original audio map)
					resMan->addResourceWithoutValidation(ResourceId(kResourceTypeRave, _mapNumber, n & 0xffffff3f), src, offset + syncSize, kq6HiresSyncSize);
					syncSize += kq6HiresSyncSize;
				}
			}

			const ResourceId id(kResourceTypeAudio36, _mapNumber, n & 0xffffff3f);

			// Map 405 on CD 1 of the US release of PQ:SWAT 1.000 is broken
			// and points to garbage in the RESOURCE.AUD. The affected audio36
			// assets seem to be able to load successfully from one of the later
			// CDs, so just ignore the map on this disc
			if (g_sci->getGameId() == GID_PQSWAT &&
				g_sci->getLanguage() == Common::EN_ANY &&
				_volumeNumber == 1 &&
				_mapNumber == 405) {
				continue;
			}

			if (g_sci->getGameId() == GID_GK2) {
				// At least version 1.00 of the US release, and the German
				// release, of GK2 have multiple invalid audio36 map entries on
				// CD 6
				if (_volumeNumber == 6 && offset + syncSize >= srcSize) {
					bool skip;
					switch (g_sci->getLanguage()) {
					case Common::EN_ANY:
						skip = (_mapNumber == 22 || _mapNumber == 160);
						break;
					case Common::DE_DEU:
						skip = (_mapNumber == 22);
						break;
					default:
						skip = false;
					}

					if (skip) {
						continue;
					}
				}

				// Map 2020 on CD 1 of the German release of GK2 is invalid.
				// This content does not appear to ever be used by the game (it
				// does not even exist in the US release), and there is a
				// correct copy of it on CD 6, so just ignore the bad copy on
				// CD 1
				if (g_sci->getLanguage() == Common::DE_DEU &&
					_volumeNumber == 1 &&
					_mapNumber == 2020) {
					continue;
				}
			}

			// Map 800 and 4176 contain content that was cut from the game. The
			// French version of the game includes map files from the US
			// release, but the audio resources are French so the maps don't
			// match. Since the content was never used, just ignore these maps
			// everywhere
			if (g_sci->getGameId() == GID_PHANTASMAGORIA2 &&
				(_mapNumber == 800 || _mapNumber == 4176)) {
				continue;
			}

			resMan->addResource(id, src, offset + syncSize, 0, getLocationName());
		}
	}

	// Audio map resources must be read and then purged in games with multi-disc
	// audio in order to read the audio maps from every CD. For other games it
	// is not necessary for these resources to be loaded at all, since they are
	// only used upon game startup to populate _resMap, so just purge them
	// always after we've read the map.
	resMan->forcePurge(mapResId);

	return SCI_ERROR_NONE;
}

} // End of namespace Sci
