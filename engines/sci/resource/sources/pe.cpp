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

#include "common/ptr.h"
#include "common/stream.h"
#include "sci/resource/sources/pe.h"
#include "sci/resource/manager.h"
#include "common/winexe_pe.h"

namespace Sci {

bool PEResourceSource::scanSource(ResourceManager *resMan) {
	const auto names = _exe.getNameList(Common::kPEString);
	for (const auto &name : names) {
		Common::ScopedPtr<Common::SeekableReadStream> data(_exe.getResource(Common::kPEString, name));
		const uint16 entryNo = (name.getID() - 1) << 4;
		for (uint index = 0; index <= 0xf; ++index) {
			const uint16 numChars = data->readUint16LE();
			if (numChars) {
				resMan->addResource(ResourceId(kResourceTypeText, entryNo | index), this, data->pos(), numChars, _name);
				data->skip(numChars * 2);
			}
		}
	}

	return true;
}

void PEResourceSource::loadResource(const ResourceManager *resMan, Resource *res) const {
	Common::ScopedPtr<Common::SeekableReadStream> inData(_exe.getResource(Common::kPEString, (res->getId().getNumber() >> 4) + 1));
	inData->skip(res->_fileOffset);
	byte *outData = new byte[res->_size];
	assert(outData);
	for (uint i = 0; i < res->_size; ++i) {
		const uint16 character = inData->readUint16LE();
		// Non-ASCII characters will need conversion to IBM 850 (there is a map
		// in event.cpp to help with this)
		assert(character <= 0x7f);
		outData[i] = character;
	}
	res->_data = outData;
	res->_status = kResStatusAllocated;
}

} // End of namespace Sci
