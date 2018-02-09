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

#ifndef SCI_RESOURCE_SOURCES_PATCH_H
#define SCI_RESOURCE_SOURCES_PATCH_H

#include "sci/resource/source.h"

namespace Sci {

class PatchResourceSource final : public DataOnlyResourceSource {
public:
	PatchResourceSource(const Common::String &name) :
		DataOnlyResourceSource(kSourcePatch, name) {}

	static bool loadPatch(Common::SeekableReadStream *file, Resource *res);

	virtual void loadResource(const ResourceManager *resMan, Resource *res) const override;

	/**
	 * Updates a resource with a patch file, if the patch file is valid.
	 * @returns `true` if ownership of this object has been transferred.
	 */
	bool processPatch(ResourceManager *resMan, ResourceType resourceType, uint16 resourceNr, uint32 tuple = 0) const;

private:
	/**
	 * Determines whether or not a patch file matching the given resource ID
	 * should be ignored when processing patch files.
	 */
	static bool isBlacklistedPatch(const ResourceManager *resMan, const ResourceId &resId);
};

} // End of namespace Sci

#endif
