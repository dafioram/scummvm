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

#ifndef SCI_RESOURCE_MANAGER_H
#define SCI_RESOURCE_MANAGER_H

#include "common/func.h"
#include "sci/graphics/helpers.h"		// for ViewType
#include "sci/resource/resource.h"
#include "sci/sci.h"

namespace Sci {

enum {
	/**
	 * The size of a patch file resource header, consisting of the resource type
	 * in byte 0 and the header's variable-length size in byte 1.
	 */
	kResourceHeaderSize = 2,

	/** The maximum allowed size for a decompressed resource. */
	kMaxResourceSize = 0x1000000,

	/**
	 * The magic number used for requests to play Audio resources instead of
	 * Audio36 resources. Used by SCI1.1+.
	 */
	kSfxModule = 65535
};

class IntMapResourceSource;
class ResourcePatcher;

ResourceId convertPatchNameBase36(ResourceType type, const Common::String &filename);

class ResourceManager {
	enum {
#ifdef ENABLE_SCI32
		// Hack to treat RESMAP.PAT/RESSCI.PAT as the highest volume
		kResPatVolumeNumber = 100,
#endif

		kMaxOpenVolumes = 5 ///< Max number of simultaneously opened volumes
	};

	// FIXME: These 'friend' declarations are meant to be a temporary hack to
	// ease transition to the ResourceSource class system.
	friend class ResourceSource;
	friend class DirectoryResourceSource;
	friend class PatchResourceSource;
	friend class ExtMapResourceSource;
	friend class IntMapResourceSource;
	friend class AudioVolumeResourceSource;
	friend class ExtAudioMapResourceSource;
	friend class WaveResourceSource;
	friend class MacResourceForkResourceSource;
	friend class ResourcePatcher;
#ifdef ENABLE_SCI32
	friend class ChunkResourceSource;
#endif

public:
	/**
	 * Creates a new SCI resource manager.
	 */
	ResourceManager(const bool detectionMode = false);
	~ResourceManager();

	/**
	 * Initializes the resource manager.
	 */
	void init();

	/**
	 * Converts a resource type byte from a resource file to the correct
	 * engine resource type.
	 */
	ResourceType convertResType(byte type);

#pragma mark -
#pragma mark Version detection
public:
	/**
	 * Finds the location of the game object from script 0.
	 * @param addSci11ScriptOffset	Adjust the return value for SCI1.1 and newer
	 *        games. Needs to be false when the heap is accessed directly inside
	 *        findSierraGameId().
	 */
	reg_t findGameObject(const bool addSci11ScriptOffset, const bool isBE);

	/** Returns true if the resource volume version is SCI1.1 Mac. */
	bool isSci11Mac() const { return _volVersion == kResVersionSci11Mac; }

	/** Returns the detected view resource type. */
	ViewType getViewType() const { return _viewType; }

	/** Returns a human-readable name for the resource map version. */
	const char *getMapVersionDesc() const { return versionDescription(_mapVersion); }

	/** Returns a human-readable name for the resource volume version. */
	const char *getVolVersionDesc() const { return versionDescription(_volVersion); }

	/** Returns the detected resource volume version. */
	ResVersion getVolVersion() const { return _volVersion; }

private:
	/** Returns a human-readable name describing a ResVersion. */
	const char *versionDescription(ResVersion version) const;

	/**
	 * Detects the resource map version by inspecting the first discovered map
	 * resource source.
	 */
	ResVersion detectMapVersion();

	/**
	 * Detects the resource volume version by inspecting the first discovered
	 * volume resource source.
	 */
	ResVersion detectVolVersion();

	/**
	 * Defines the global SCI version using resource heuristics. This method
	 * must be called before any use of `getSciVersion`.
	 */
	void detectSciVersion();

	/**
	 * Detects whether script resources are in SCI0early format. This will
	 * decide if the game version is SCI0early.
	 */
	bool hasOldScriptHeader();

	/**
	 * Determines what compression type, if any, is used by views. This
	 * may decide the view type and the minimum possible SCI version.
	 */
	ResourceCompression getViewCompression();

	/**
	 * Detects the view type when it cannot be determined by compression type
	 * alone.
	 */
	ViewType detectViewType();

	/**
	 * Detects a valid vocab 999 resource. This will decide if the game is
	 * SCI0late.
	 */
	bool hasSci0Voc999();

	/**
	 * Detects a valid vocab 900 resource. This will decide if the game is
	 * SCI0late or SCI01.
	 */
	bool hasSci1Voc900();

	/**
	 * Checks whether either signature in a signature pair matches the given
	 * resource. This will decide which SCI2.1 variant the game uses.
	 */
	bool checkResourceForSignatures(ResourceType resourceType, uint16 resourceNr, const byte *signature1, const byte *signature2);

	/**
	 * Checks whether the given signature matches the given resource.
	 */
	bool checkResourceDataForSignature(Resource *resource, const byte *signature);

	/**
	 * If true, this resource manager is being used for game detection, not to
	 * run a game.
	 */
	bool _detectionMode;

	/** The detected view resource type. */
	ViewType _viewType;

	/** The detected RESOURCE.0nn file version. */
	ResVersion _volVersion;

	/** The detected RESOURCE.MAP file version. */
	ResVersion _mapVersion;

#pragma mark -
#pragma mark Resource scanning
public:
	/**
	 * Adds all possible resource sources for the game.
	 */
	void addAppropriateSources();

	/**
	 * Adds resource sources from the given FSList necessary for performing
	 * resource-based version detection.
	 */
	void addAppropriateSourcesForDetection(const Common::FSList &fslist);

private:
	/**
	 * Scans all newly added resource sources for resources, earliest addition
	 * first.
	 */
	void scanNewSources();

	/**
	 * Adds a source to the resource manager's list of sources.
	 * @param source The source to add. Ownership of the object is transferred.
	 */
	inline void addSource(ResourceSource *source) {
		assert(source);
		_sources.push_back(source);
	}

	/**
	 * Adds a patch file directory as a resource source.
	 */
	void addPatchDir(const Common::String &path);

	/**
	 * Add an external (e.g. non-audio, RESOURCE.MAP/RESMAP.00n) map resource to
	 * the resource manager's list of sources.
	 *
	 * @param filename  The name of the volume to add
	 * @param volumeNo The volume number of the map, for multi-CD games
	 */
	ResourceSource *addExternalMap(const Common::String &filename, int volumeNo = 0);

	/**
	 * Add an external (e.g. non-audio, RESOURCE.MAP/RESMAP.00n) map resource to
	 * the resource manager's list of sources.
	 *
	 * @param mapFile  The file node to add
	 * @param volumeNo The volume number of the map, for multi-CD games
	 */
	ResourceSource *addExternalMap(const Common::FSNode *mapFile, int volumeNo = 0);

	/**
	 * Adds audio sources for each audio map resource which has already been
	 * added to the resource manager. This function does nothing in multi-CD
	 * SCI32 games since these games have different audio maps on each CD so
	 * need to read audio maps multiple times when each external map file is
	 * processed.
	 */
	void addAudioSources();

#ifdef ENABLE_SCI32
	/**
	 * Adds SCI2.1 resource chunk 0 as a resource source if no scripts exist.
	 * Used only by a Lighthouse demo.
	 */
	void addScriptChunkSource();
#endif

	typedef Common::List<ResourceSource *> SourcesList;

	/**
	 * The list of resource sources currently loaded into the resource manager.
	 */
	SourcesList _sources;

	/**
	 * A reference to the built-in resource patcher. The patcher itself is owned
	 * by `_sources`.
	 */
	ResourcePatcher *_patcher;

	/**
	 * If true, a non-fatal problem occurred when adding a resource or resource
	 * source.
	 */
	bool _hasBadResources;

#pragma mark -
#pragma mark Resource management
public:
	/**
	 * Looks up a resource's data.
	 * @param id	The resource type to look for
	 * @param lock	non-zero iff the resource should be locked
	 * @return The resource, or NULL if it doesn't exist
	 * @note Locked resources are guaranteed not to have their contents freed until
	 *       they are unlocked explicitly (by unlockResource).
	 */
	Resource *findResource(ResourceId id, bool lock);

	/**
	 * Unlocks a previously locked resource.
	 * @param res	The resource to free
	 */
	void unlockResource(Resource *res);

	/**
	 * Tests whether a resource exists.
	 *
	 * This function may often be much faster than finding the resource
	 * and should be preferred for simple tests.
	 * The resource object returned is, indeed, the resource in question, but
	 * it should be used with care, as it may be unallocated.
	 *
	 * @param id ID of the resource to check
	 * @return non-NULL if the resource exists, NULL otherwise
	 */
	Resource *testResource(ResourceId id);

	/**
	 * Returns a list of all resources of the specified type.
	 * @param type		The resource type to look for
	 * @param mapNumber	For audio36 and sync36, limit search to this map
	 * @return			The resource list
	 */
	Common::List<ResourceId> listResources(ResourceType type, int mapNumber = -1);

private:
	typedef Common::HashMap<ResourceId, Resource *, ResourceIdHash> ResourceMap;
	typedef Common::List<Common::File *> VolumeFiles;

	/**
	 * Adds all resources from a SCI0 resource.map file.
	 * @param map The map
	 */
	ResourceErrorCode readResourceMapSCI0(ResourceSource *map);

	/**
	 * Adds all resources from a SCI1+ resource.map file.
	 * @param map The map
	 */
	ResourceErrorCode readResourceMapSCI1(ResourceSource *map);

	/**
	 * Adds or removes all audio resources from a SCI1 audio map file.
	 * @param map The map
	 * @param unload Unload the map instead of loading it
	 */
	ResourceErrorCode readAudioMapSCI1(ResourceSource *map, bool unload = false);

	/**
	 * Adds all audio resources from a SCI1.1 audio map file.
	 * @param map The map
	 */
	ResourceErrorCode readAudioMapSCI11(IntMapResourceSource *map);

	/**
	 * Adds all resources from patch files in the game directory.
	 */
	void readResourcePatches();

	/**
	 * Adds all audio36/sync36 resources from patch files in the game directory.
	 */
	void readResourcePatchesBase36();

	/**
	 * Adds all audio resources from standard WAV files in the game directory.
	 */
	void readWaveAudioPatches();

	/**
	 * Determines whether or not a patch file matching the given resource ID
	 * should be ignored when processing patch files.
	 */
	bool isBlacklistedPatch(const ResourceId &resId) const;

	/**
	 * Returns whether or not patches using the SCI0 naming convention should be
	 * searched for when looking for patch files.
	 */
	bool shouldFindSci0Patches() const;

	/**
	 * Processes a patch file into a resource. Ownership of the `source` object
	 * is transferred.
	 */
	void processPatch(ResourceSource *source, ResourceType resourceType, uint16 resourceNr, uint32 tuple = 0);

	/**
	 * Processes a standard WAV file into a resource.
	 */
	void processWavePatch(const ResourceId &resourceId, const Common::String &name);

	/**
	 * Adds the appropriate GM patch from the Sierra MIDI utility as 4.pat,
	 * without requiring the user to rename the file to 4.pat. Thus, the
	 * original Sierra archive can be extracted in the extras directory, and the
	 * GM patches can be applied per game, if applicable.
	 */
	void addNewGMPatch();

	/**
	 * Searches all resource sources to find the volume resource source which
	 * corresponds to the given map resource source + volume number.
	 * @returns nullptr if no corresponding volume could be found.
	 */
	ResourceSource *findVolumeForMap(ResourceSource *map, int volumeNo);

	/**
	 * Gets a possibly-cached pointer to a read stream for the given resource
	 * source.
	 *
	 * @note All calls to `getVolumeFile` must be followed with a corresponding
	 * call to `disposeVolumeFileStream` once the stream is finished being used.
	 */
	Common::SeekableReadStream *getVolumeFile(ResourceSource *source);

	/**
	 * Disposes a read stream returned by `getVolumeFile`.
	 */
	void disposeVolumeFileStream(Common::SeekableReadStream *fileStream, ResourceSource *source);

	/**
	 * Loads the given resource into memory, applying any patches from the
	 * built-in patcher.
	 */
	void loadResource(Resource *res);

	/**
	 * Performs basic validation of the given resource. Returns true if the
	 * resource passes validation.
	 */
	bool validateResource(const ResourceId &resourceId, const Common::String &sourceMapLocation, const Common::String &sourceName, const uint32 offset, const uint32 size, const uint32 sourceSize) const;

	/**
	 * Adds the given resource to the resource manager if it does not already
	 * exist.
	 */
	Resource *addResource(ResourceId resId, ResourceSource *src, uint32 offset, uint32 size = 0, const Common::String &sourceMapLocation = Common::String("(no map location)"));

	/**
	 * Adds or updates a resource in the resource manager, maintaining the
	 * offset to the resource if one is already set.
	 */
	Resource *updateResource(ResourceId resId, ResourceSource *src, uint32 size, const Common::String &sourceMapLocation = Common::String("(no map location)"));

	/**
	 * Adds or updates a resource in the resource manager.
	 */
	Resource *updateResource(ResourceId resId, ResourceSource *src, uint32 offset, uint32 size, const Common::String &sourceMapLocation = Common::String("(no map location)"));

	/**
	 * Removes the given audio resource, if one exists. Used only by SCI1.
	 */
	void removeAudioResource(ResourceId resId);

	/**
	 * The map of all resources currently available to the resource manager.
	 */
	ResourceMap _resMap;

	/**
	 * A cache of open volume files. The most recently used files are on the
	 * front of the list.
	 */
	VolumeFiles _volumeFiles;

#pragma mark -
#pragma mark Language settings
public:
	/**
	 * Sets the language used when looking up audio resources, Where `language`
	 * is a SCI1 audio volume file number. Used by SCI1.
	 */
	void setAudioLanguage(int language);

	/**
	 * Gets the audio map currently in use. Used by SCI1.
	 */
	int getAudioLanguage() const;

#ifdef ENABLE_SCI32
	/**
	 * Changes the directory used when looking up audio resources. Used by
	 * SCI2.1 (MGDX).
	 */
	void changeAudioDirectory(Common::String path);
#endif

private:
	/**
	 * The currently loaded audio map. Used by SCI1 only.
	 */
	ResourceSource *_audioMapSCI1;

#ifdef ENABLE_SCI32
#pragma mark -
#pragma mark Multi-disc handling
public:
	/**
	 * Updates the currently active disc number.
	 */
	void findDisc(const int16 discNo);

	/**
	 * Gets the currently active disc number.
	 */
	int16 getCurrentDiscNo() const { return _currentDiscNo; }

private:
	/**
	 * The currently active disc number.
	 */
	int16 _currentDiscNo;

	/**
	 * If true, the game has multiple audio volumes that contain different
	 * audio files for each disc.
	 */
	bool _multiDiscAudio;
#endif

#pragma mark -
#pragma mark Resource cache
private:
	typedef Common::List<Resource *> LRUList;

	/**
	 * The maximum amount of memory to allocate for unlocked cached resources,
	 * in bytes.
	 */
	int _maxMemoryLRU;

	/**
	 * The amount of memory currently allocated to locked resources, in bytes.
	 */
	int _memoryLocked;

	/**
	 * The amount of memory currently allocated to unlocked resources, in bytes.
	 */
	int _memoryLRU;

	/**
	 * The list of most recently used resources. The newest resources are on
	 * the front of the list.
	 */
	LRUList _LRU;

	/**
	 * Prints statistics about the LRU cache.
	 */
	void printLRU() const;

	/**
	 * Adds the given resource to the LRU,
	 */
	void addToLRU(Resource *res);

	/**
	 * Removes the given resource from the LRU.
	 */
	void removeFromLRU(Resource *res);

	/**
	 * Purges items from the LRU until the maximum cache size is reached.
	 */
	void freeOldResources();
};

} // End of namespace Sci

#endif
