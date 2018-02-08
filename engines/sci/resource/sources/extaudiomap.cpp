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

#include "sci/resource/sources/extaudiomap.h"
#include "sci/resource/manager.h"

namespace Sci {

bool ExtAudioMapResourceSource::scanSource(ResourceManager *resMan) {
	return (readAudioMapSCI1(resMan, false) == SCI_ERROR_NONE);
}

// AUDIOnnn.MAP contains 10-byte entries:
// Early format:
// w 5 bits resource type and 11 bits resource number
// dw 7 bits volume number and 25 bits offset
// dw size
// Later format:
// w nEntry
// dw offset+volume (as in resource.map)
// dw size
// ending with 10 0xFFs
ResourceErrorCode ExtAudioMapResourceSource::readAudioMapSCI1(ResourceManager *resMan, bool unload) const {
	Common::File file;

	if (!file.open(getLocationName()))
		return SCI_ERROR_RESMAP_NOT_FOUND;

	bool oldFormat = (file.readUint16LE() >> 11) == kResourceTypeAudio;
	file.seek(0);

	for (;;) {
		uint16 n = file.readUint16LE();
		uint32 offset = file.readUint32LE();
		uint32 size = file.readUint32LE();

		if (file.eos() || file.err()) {
			warning("Error while reading %s", getLocationName().c_str());
			return SCI_ERROR_RESMAP_NOT_FOUND;
		}

		if (n == 0xffff)
			break;

		byte volumeNo;

		if (oldFormat) {
			n &= 0x07ff; // Mask out resource type
			volumeNo = offset >> 25; // most significant 7 bits
			offset &= 0x01ffffff; // least significant 25 bits
		} else {
			volumeNo = offset >> 28; // most significant 4 bits
			offset &= 0x0fffffff; // least significant 28 bits
		}

		const ResourceSource *src = resMan->findVolumeForMap(this, volumeNo);

		if (src) {
			const ResourceId resId(kResourceTypeAudio, n);

			if (unload)
				resMan->removeAudioResource(resId);
			else
				resMan->addResource(resId, src, offset, size, getLocationName());
		} else {
			warning("Failed to find audio volume %i", volumeNo);
			return SCI_ERROR_NO_RESOURCE_FILES_FOUND;
		}
	}

	return SCI_ERROR_NONE;
}

} // End of namespace Sci
