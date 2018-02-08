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

#ifndef SCI_RESOURCE_SOURCES_DIRECTORY_H
#define SCI_RESOURCE_SOURCES_DIRECTORY_H

#include "sci/resource/resource.h"
#include "sci/resource/source.h"

namespace Sci {

class DirectoryResourceSource : public ResourceSource {
public:
	DirectoryResourceSource(const Common::String &name) :
		ResourceSource(kSourceDirectory, name) {}

	virtual bool scanSource(ResourceManager *resMan) override;

private:
	/**
	 * Adds all resources from patch files in the game directory.
	 */
	void readResourcePatches(ResourceManager *resMan) const;

	/**
	 * Adds all audio36/sync36 resources from patch files in the game directory.
	 */
	void readResourcePatchesBase36(ResourceManager *resMan) const;

	/**
	 * Adds all audio resources from standard WAV files in the game directory.
	 */
	void readWaveAudioPatches(ResourceManager *resMan) const;

	/**
	 * Returns whether or not patches using the SCI0 naming convention should be
	 * searched for when looking for patch files.
	 */
	bool shouldFindSci0Patches() const;
};

} // End of namespace Sci

#endif
