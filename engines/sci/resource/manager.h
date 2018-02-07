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
	kResourceHeaderSize = 2, ///< patch type + header size

	/** The maximum allowed size for a compressed or decompressed resource */
	kMaxResourceSize = 0x1000000,

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

	typedef Common::HashMap<ResourceId, Resource *, ResourceIdHash> ResourceMap;

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
	 * Converts a map resource type to our type
	 * @param sciType The type from the map/patch
	 * @return The ResourceType
	 */
	ResourceType convertResType(byte type);

#pragma mark -
#pragma mark Detection
public:
	/**
	 * Finds the location of the game object from script 0.
	 * @param addSci11ScriptOffset	Adjust the return value for SCI1.1 and newer
	 *        games. Needs to be false when the heap is accessed directly inside
	 *        findSierraGameId().
	 */
	reg_t findGameObject(const bool addSci11ScriptOffset, const bool isBE);

	bool isSci11Mac() const { return _volVersion == kResVersionSci11Mac; }
	ViewType getViewType() const { return _viewType; }
	const char *getMapVersionDesc() const { return versionDescription(_mapVersion); }
	const char *getVolVersionDesc() const { return versionDescription(_volVersion); }
	ResVersion getVolVersion() const { return _volVersion; }

private:
	bool _detectionMode;
	ViewType _viewType; // Used to determine if the game has EGA or VGA graphics
	ResVersion _volVersion; ///< resource.0xx version
	ResVersion _mapVersion; ///< resource.map version

	/**
	 * Returns a string describing a ResVersion.
	 * @param version	The resource version
	 * @return		The description of version
	 */
	const char *versionDescription(ResVersion version) const;

	/**--- Resource map decoding functions ---*/
	ResVersion detectMapVersion();
	ResVersion detectVolVersion();

	/**
	 * Applies to all versions before 0.000.395 (i.e. KQ4 old, XMAS 1988 and LSL2).
	 * Old SCI versions used two word header for script blocks (first word equal
	 * to 0x82, meaning of the second one unknown). New SCI versions used one
	 * word header.
	 * Also, old SCI versions assign 120 degrees to left & right, and 60 to up
	 * and down. Later versions use an even 90 degree distribution.
	 */
	bool hasOldScriptHeader();

	ResourceCompression getViewCompression();

	ViewType detectViewType();

	bool hasSci0Voc999();

	bool hasSci1Voc900();

	bool checkResourceDataForSignature(Resource *resource, const byte *signature);

	bool checkResourceForSignatures(ResourceType resourceType, uint16 resourceNr, const byte *signature1, const byte *signature2);

	void detectSciVersion();

#pragma mark -
#pragma mark Resource scanning
public:
	/**
	 * Adds all of the resource files for a game
	 */
	int addAppropriateSources();

	/**
	 * Similar to the function above, only called from the fallback detector
	 */
	int addAppropriateSourcesForDetection(const Common::FSList &fslist);	// TODO: Switch from FSList to Common::Archive?

private:
	/**
	 * Add a path to the resource manager's list of sources.
	 * @return a pointer to the added source structure, or NULL if an error occurred.
	 */
	ResourceSource *addPatchDir(const Common::String &path);

	/**
	 * Adds a source to the resource manager's list of sources.
	 * @param source	The new source to add
	 * @return		A pointer to the added source structure, or NULL if an error occurred.
	 */
	ResourceSource *addSource(ResourceSource *source);

	/**
	 * Add an external (i.e., separate file) map resource to the resource
	 * manager's list of sources.
	 * @param filename	 The name of the volume to add
	 * @param volume_nr  The volume number the map starts at, 0 for <SCI2.1
	 * @return		A pointer to the added source structure, or NULL if an error occurred.
	 */
	ResourceSource *addExternalMap(const Common::String &filename, int volume_nr = 0);

	ResourceSource *addExternalMap(const Common::FSNode *mapFile, int volume_nr = 0);

	/**
	 * Scans newly registered resource sources for resources, earliest addition first.
	 * @param detected_version Pointer to the detected version number,
	 *					 used during startup. May be NULL.
	 * @return One of SCI_ERROR_*.
	 */
	void scanNewSources();

	void addAudioSources();
	void addScriptChunkSources();
	void freeResourceSources();

	/**
	 * Reads the SCI0 resource.map file from a local directory.
	 * @param map The map
	 */
	ResourceErrorCode readResourceMapSCI0(ResourceSource *map);

	/**
	 * Reads the SCI1 resource.map file from a local directory.
	 * @param map The map
	 */
	ResourceErrorCode readResourceMapSCI1(ResourceSource *map);

	/**
	 * Reads SCI1.1 audio map resources.
	 * @param map The map
	 */
	ResourceErrorCode readAudioMapSCI11(IntMapResourceSource *map);

	/**
	 * Reads SCI1 audio map files.
	 * @param map The map
	 * @param unload Unload the map instead of loading it
	 * @return 0 on success, an SCI_ERROR_* code otherwise
	 */
	ResourceErrorCode readAudioMapSCI1(ResourceSource *map, bool unload = false);

	/**
	 * Reads patch files from a local directory.
	 */
	void readResourcePatches();
	void readResourcePatchesBase36();

	/**
	 * Determines whether or not a patch file matching the given resource ID
	 * should be ignored when processing patches.
	 */
	bool isBlacklistedPatch(const ResourceId &resId) const;

	/**
	 * Returns whether or not patches using the SCI0 naming convention should be
	 * search for when looking for patch files.
	 */
	bool shouldFindSci0Patches() const;

	/**
	 * The ownership of `source` is transferred into this function.
	 */
	void processPatch(ResourceSource *source, ResourceType resourceType, uint16 resourceNr, uint32 tuple = 0);

	/**
	 * Process wave files as patches for Audio resources.
	 */
	void readWaveAudioPatches();
	void processWavePatch(ResourceId resourceId, const Common::String &name);

	typedef Common::List<ResourceSource *> SourcesList;
	SourcesList _sources;

	// For better or worse, because the patcher is added as a ResourceSource,
	// its destruction is managed by freeResourceSources.
	ResourcePatcher *_patcher;

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
	 * Use scir_find_resource() if you want to use the data contained in the resource.
	 *
	 * @param id	Id of the resource to check
	 * @return		non-NULL if the resource exists, NULL otherwise
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
	/**
	 * Adds the appropriate GM patch from the Sierra MIDI utility as 4.pat, without
	 * requiring the user to rename the file to 4.pat. Thus, the original Sierra
	 * archive can be extracted in the extras directory, and the GM patches can be
	 * applied per game, if applicable.
	 */
	void addNewGMPatch();

	ResourceSource *findVolume(ResourceSource *map, int volume_nr);

	/**
	 * All calls to getVolumeFile must be followed with a corresponding
	 * call to disposeVolumeFileStream once the stream is finished being used.
	 * Do NOT call delete directly on returned streams, as they may be cached.
	 */
	Common::SeekableReadStream *getVolumeFile(ResourceSource *source);

	void disposeVolumeFileStream(Common::SeekableReadStream *fileStream, ResourceSource *source);

	void loadResource(Resource *res);

	bool validateResource(const ResourceId &resourceId, const Common::String &sourceMapLocation, const Common::String &sourceName, const uint32 offset, const uint32 size, const uint32 sourceSize) const;

	Resource *addResource(ResourceId resId, ResourceSource *src, uint32 offset, uint32 size = 0, const Common::String &sourceMapLocation = Common::String("(no map location)"));

	Resource *updateResource(ResourceId resId, ResourceSource *src, uint32 size, const Common::String &sourceMapLocation = Common::String("(no map location)"));

	Resource *updateResource(ResourceId resId, ResourceSource *src, uint32 offset, uint32 size, const Common::String &sourceMapLocation = Common::String("(no map location)"));
	void removeAudioResource(ResourceId resId);

	ResourceMap _resMap;
	Common::List<Common::File *> _volumeFiles; ///< list of opened volume files
	ResourceSource *_audioMapSCI1; ///< Currently loaded audio map for SCI1

#ifdef ENABLE_SCI32
	/**
	 * Parses all resources from a SCI2.1 chunk resource and adds them to the
	 * resource manager.
	 */
	void addResourcesFromChunk(uint16 id);
#endif

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

#ifdef ENABLE_SCI32
#pragma mark -
#pragma mark Multi-disc handling

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
#pragma mark Resource caching
private:
	// Maximum number of bytes to allow being allocated for resources
	// Note: maxMemory will not be interpreted as a hard limit, only as a restriction
	// for resources which are not explicitly locked. However, a warning will be
	// issued whenever this limit is exceeded.
	int _maxMemoryLRU;

	int _memoryLocked;	///< Amount of resource bytes in locked memory
	int _memoryLRU;		///< Amount of resource bytes under LRU control
	Common::List<Resource *> _LRU; ///< Last Resource Used list

	void printLRU();
	void addToLRU(Resource *res);
	void removeFromLRU(Resource *res);
	void freeOldResources();
};

} // End of namespace Sci

#endif
