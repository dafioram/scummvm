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

#ifndef SCI_SOUND_SCI0SOUND_H
#define SCI_SOUND_SCI0SOUND_H

#include "sci/sound/sound.h"

namespace Sci {

enum Sci0PlayStrategy {
	/**
	 * No play strategy is defined.
	 */
	kStrategyNone = -1,

	/**
	 * The caller should synchronously invoke `service` on the driver until
	 * `signal` is kFinished.
	 */
	kStrategySync = 0,

	/**
	 * The caller should invoke `service` once on the driver and then
	 * return.
	 */
	kStrategyAsync = 1,

	/**
	 * Playback did not begin; the caller should clean up immediately.
	 */
	kStrategyAbort = 3
};

enum Sci0PlayState {
	kStateNotReady = 0,
	kStateReady = 1,
	kStateBlocked = 2,
	kStateActive = 3
};

/**
 * A representation of the current state of a sound in SCI0.
 */
struct Sci0Sound : public Common::Serializable {
	/**
	 * The VM sound object used to create this sound.
	 */
	reg_t soundObj;

	/**
	 * The sound resource ID.
	 */
	uint16 resourceNo;

	/**
	 * The Sound resource.
	 *
	 * @note In SSCI this is a raw pointer to the resource data.
	 */
	Resource *resource;

	/**
	 * The number of times the sound should loop.
	 */
	uint16 numLoops;

	/**
	 * The current position of playback in the sound resource, in bytes.
	 */
	uint16 position;

	/**
	 * The priority of the sound. See `Sci1Sound::priority`.
	 */
	int16 priority;

	/**
	 * The playback strategy for the sound.
	 */
	Sci0PlayStrategy strategy;

	uint16 chain; // TODO: May be unused, replaced by priority

	Sci0PlayState state;

	int16 signal;

	/**
	 * The playback volume of the sound, from 0 to 15.
	 */
	int16 volume;

	uint16 effect; // TODO: Not a clue what this is

	Sci0Sound(const reg_t soundObj_ = NULL_REG) :
		soundObj(soundObj_),
		resource(nullptr),
		numLoops(0),
		position(0),
		priority(0),
		strategy(kStrategySync),
		chain(0),
		state(kStateNotReady),
		signal(0),
		volume(0),
		effect(0) {}

	uint8 peek(const uint8 extra = 0) {
		return resource->getUint8At(position + extra);
	}

	uint8 consume() {
		assert(resource);
		const uint8 value = peek();
		advance();
		return value;
	}

	void advance() {
		assert(position < resource->size());
		++position;
	}

	virtual void saveLoadWithSerializer(Common::Serializer &s) override;
};

class Sci0SoundManager final : public SoundManager {
public:
	Sci0SoundManager(ResourceManager &resMan, SegManager &segMan, GameFeatures &features, GuestAdditions &guestAdditions);
	~Sci0SoundManager();

	virtual void reset() override;

private:
	typedef Common::Array<Sci0Sound> SoundsList;

	/**
	 * The list of currently playing & queued sounds.
	 */
	SoundsList _sounds;

#pragma mark -
#pragma mark Save management
public:
	virtual void saveLoadWithSerializer(Common::Serializer &s) override;
	virtual void reconstructPlaylist() override;

private:
	void legacyRestore(Common::Serializer &s);

#pragma mark -
#pragma mark MIDI server
private:
	enum { kHeaderSize = 0x22 };

	struct PlayState {
		uint16 rest;
		Sci0PlayState state;
		uint8 currentFadeVolume;
		uint8 fadeTicksLeftInStep;
		uint8 fadeTicksPerStep;
		bool resetPositionOnPause;
		uint16 loopPosition;
		uint16 cue;

		uint8 lastCommand;

		PlayState() :
			rest(0),
			state(kStateBlocked),
			currentFadeVolume(0),
			fadeTicksLeftInStep(8),
			fadeTicksPerStep(0),
			resetPositionOnPause(false),
			loopPosition(kHeaderSize),
			cue(127),
			lastCommand(0) {}
	};

	static inline void soundServerCallback(void *soundManager) {
		static_cast<Sci0SoundManager *>(soundManager)->soundServer();
	}

	/**
	 * Advances the state of the sound server. Must be called once per tick in
	 * SCI0 for correct sound processing.
	 */
	void soundServer();

	void advancePlayback(Sci0Sound &sound, const bool restoring);

	void processControlChannel(Sci0Sound &sound);

	void processFade(Sci0Sound &sound);

	Sci0PlayStrategy initSound(Sci0Sound &sound);

	void processEndOfTrack(Sci0Sound &sound);

	void sendMessage(Sci0Sound &sound, const uint8 message, const bool restoring);

	PlayState _state;

#pragma mark -
#pragma mark Channel remapping
private:
	enum {
		kNumHardwareChannels = 16
	};

	/**
	 * A physical output channel.
	 */
	struct HardwareChannel {
		enum {
			/**
			 * Special `channelNo` value for an unmapped hardware channel.
			 */
			kUnmapped = 0xff
		};

		/**
		 * The the logical channel currently mapped to this hardware channel.
		 */
		uint8 channelNo;

		/**
		 * The number of voices used by this channel.
		 * TODO: was ChVoice
		 */
		uint8 numVoices;

		HardwareChannel() :
			channelNo(kUnmapped),
			numVoices(0) {}

		inline bool isMapped() const { return channelNo != kUnmapped; }
	};

	typedef Common::FixedArray<HardwareChannel, kNumHardwareChannels> HardwareChannels;

	HardwareChannels _hardwareChannels;

#pragma mark -
#pragma mark Effects
protected:
	virtual uint8 getMasterVolumeImpl() const override { return _masterVolume; }
	virtual void setMasterVolumeImpl(const uint8 volume) override;

private:
	void setSoundVolumes(const uint8 volume);

	uint8 _masterVolume;

#pragma mark -
#pragma mark Playback management
public:
	virtual void pauseAll(const bool pause) override;

private:
	void stopAllSounds();

	void stopAllChannels(const bool pauseOnly);

	void play(Sci0Sound &sound);
	void pause(Sci0Sound &sound);
	void restore(Sci0Sound &sound);
	void stop(Sci0Sound &sound);
	void fade(Sci0Sound &sound);

	Sci0Sound &activate(Sci0Sound &sound);
	Sci0Sound &finishActivation(Sci0Sound &sound);

	/**
	 * The value of `_numServerSuspensions` the last time that sounds were
	 * paused by a call to `pauseAll`.
	 */
	int _lastNumServerSuspensions;

#pragma mark -
#pragma mark Kernel
public:
	void kernelInit(const reg_t soundObj) override;
	void kernelPlay(const reg_t soundObj, const bool) override;
	void kernelDispose(const reg_t soundObj) override;
	void kernelStop(const reg_t soundObj) override;
	bool kernelPause(const bool pause) override;
	void kernelUpdate(const reg_t soundObj) override;
	void kernelFade(const reg_t soundObj) override;

private:
	class ByLowerPriority {
		int16 _priority;
	public:
		inline ByLowerPriority(const int16 priority) : _priority(priority) {}
		inline bool operator()(const Sci0Sound &sound) const {
			return sound.priority <= _priority;
		}
	};

	class ByRegT {
		reg_t _key;
	public:
		inline ByRegT(const reg_t key) : _key(key) {}
		inline bool operator()(const Sci0Sound &sound) const {
			return sound.soundObj == _key;
		}
	};

	class ByState {
		Sci0PlayState _state;
	public:
		inline ByState(const Sci0PlayState state) : _state(state) {}
		inline bool operator()(const Sci0Sound &sound) const {
			return sound.state == _state;
		}
	};

	template <typename Comparator, typename T>
	inline const Sci0Sound *findSound(const T key) const {
		const SoundsList::const_iterator it = Common::find_if(_sounds.begin(), _sounds.end(), Comparator(key));
		if (it == _sounds.end()) {
			return nullptr;
		}
		return &*it;
	}

	template <typename Comparator, typename T>
	inline Sci0Sound *findSound(const T key) {
		const SoundsList::iterator it = Common::find_if(_sounds.begin(), _sounds.end(), Comparator(key));
		if (it == _sounds.end()) {
			return nullptr;
		}
		return &*it;
	}

	template <typename Comparator, typename T>
	inline SoundsList::iterator findSoundIterator(const T key) {
		return Common::find_if(_sounds.begin(), _sounds.end(), Comparator(key));
	}

	template <Sci0PlayState STATE>
	inline Sci0Sound *findSoundByState() {
		const SoundsList::iterator it = Common::find_if(_sounds.begin(), _sounds.end(), ByState(STATE));
		if (it == _sounds.end()) {
			return nullptr;
		}
		return &*it;
	}

#pragma mark -
#pragma mark Debugging
public:
	virtual void debugPrintPlaylist(Console &con) const override;
	virtual void debugPrintSound(Console &con, const uint index) const override;
	virtual void debugPrintChannelMap(Console &con) const override;
	virtual void debugPlaySound(Console &con, const GuiResourceId resourceNo, const bool exclusive) override;
	virtual void debugStopAll() override;

private:
	void debugPrintSound(Console &con, const Sci0Sound &sound) const;
};

} // End of namespace Sci

#endif
