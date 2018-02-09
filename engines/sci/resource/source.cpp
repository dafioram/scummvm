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

#include "common/error.h"
#include "common/stream.h"
#include "sci/resource/manager.h"
#include "sci/resource/source.h"

namespace Sci {

ResourceSource::ResourceSource(ResSourceType type, const Common::String &name, int volNum, const Common::FSNode *resFile)
 : _sourceType(type), _name(name), _volumeNumber(volNum), _resourceFile(resFile) {
	_scanned = false;
}

Common::SeekableReadStream *ResourceSource::getVolumeFile(const ResourceManager *resMan, const Resource *res) const {
	Common::SeekableReadStream *fileStream = resMan->getVolumeFile(this);

	if (!fileStream) {
		warning("Failed to open %s", getLocationName().c_str());
		// TODO: trigger resman resource failure
	}

	return fileStream;
}

bool ResourceSource::loadFromStream(Common::SeekableReadStream *file, Resource *res) const {
	byte *ptr = new byte[res->_size];
	res->_data = ptr;

	uint32 bytesRead = file->read(ptr, res->_size);
	if (bytesRead != res->_size) {
		warning("Read %d bytes from %s but expected %u", bytesRead, res->name().c_str(), res->_size);
		res->unalloc();
		return false;
	}

	res->_status = kResStatusAllocated;
	return true;
}

void IndexOnlyResourceSource::loadResource(const ResourceManager *, Resource *res) const {
	error("Attempt to load %s from an index resource source", res->name().c_str());
}

} // End of namespace Sci
