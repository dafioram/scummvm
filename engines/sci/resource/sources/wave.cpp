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

#include "sci/resource/sources/wave.h"
#include "sci/resource/manager.h"
#include "sci/resource/resource.h"

namespace Sci {

void WaveResourceSource::loadResource(const ResourceManager *resMan, Resource *res) const {
	Common::SeekableReadStream *fileStream = getVolumeFile(resMan, res);
	if (!fileStream)
		return;

	fileStream->seek(res->_fileOffset, SEEK_SET);
	loadFromFile(fileStream, res);
	resMan->disposeVolumeFileStream(fileStream, this);
}

bool WaveResourceSource::loadFromFile(Common::SeekableReadStream *file, Resource *res) {
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

} // End of namespace Sci
