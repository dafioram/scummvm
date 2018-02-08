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
#include "sci/resource/sources/patch.h"

namespace Sci {

void PatchResourceSource::loadResource(const ResourceManager *resMan, Resource *res) const {
	bool result = loadFromPatchFile(res);
	if (!result) {
		// TODO: We used to fallback to the "default" code here if loadFromPatchFile
		// failed, but I am not sure whether that is really appropriate.
		// In fact it looks like a bug to me, so I commented this out for now.
		//ResourceSource::loadResource(res);
	}
}

bool PatchResourceSource::loadFromPatchFile(Resource *res) const {
	Common::File file;
	if (!file.open(getLocationName())) {
		warning("Failed to open patch file %s", getLocationName().c_str());
		res->unalloc();
		return false;
	}
	file.seek(0, SEEK_SET);
	return loadPatch(&file, res);
}

} // End of namespace Sci
