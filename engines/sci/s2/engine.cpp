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

#include <ctime>
#include "common/config-manager.h"
#include "common/error.h"
#include "common/savefile.h"
#include "common/system.h"
#include "engines/savestate.h"
#include "graphics/thumbnail.h"
#include "sci/s2/debugger.h"
#include "sci/s2/engine.h"
#include "sci/s2/game.h"
#include "sci/s2/kernel.h"

namespace Sci {

// TODO: Make this some common code somewhere and/or replace it with something
// less critically dumb so we can consistently have correct universal dates in
// save games
static std::time_t getLocalEpoch() {
	static std::time_t localEpoch = 0;
	static bool defined = false;
	if (!defined) {
		std::tm epoch = { 0, 0, 0, 1, 0, 70, 0, 0, -1};
		localEpoch = std::mktime(&epoch);
		auto utcEpoch = *std::gmtime(&localEpoch);
		auto utcTime = std::mktime(&utcEpoch);
		localEpoch -= (utcTime - localEpoch);
	}
	return localEpoch;
}

static std::tm timestampToLocal(const uint64 timestamp) {
	std::time_t localTimestamp(getLocalEpoch() + timestamp);
	return *localtime(&localTimestamp);
}

static uint64 getCurrentTimestamp() {
	std::time_t now = std::time(nullptr);
	return now - getLocalEpoch();
}

S2Engine::S2Engine(OSystem &system, const char *gameId, const GameMetadata &metadata) :
	Engine(&system),
	_system(system),
	_gameId(gameId),
	_metadata(metadata) {
	g_SciBE = false;
	g_Sci11BE = false;
	g_Sci32BE = false;
}

bool S2Engine::detectSaveGame(Common::InSaveFile &in) {
	const uint16 magic = in.readUint16BE();
	in.seek(-2, SEEK_CUR);
	return magic == kSaveMagic;
}

bool S2Engine::readSaveGameMetadata(Common::InSaveFile &in, Sci::S2SaveGameMetadata &outMetadata) {
	if (in.readUint16BE() != kSaveMagic) {
		return false;
	}

	Common::Serializer serializer(&in, nullptr);
	if (!serializer.syncVersion(kSaveVersion)) {
		return false;
	}

	outMetadata.saveLoadWithSerializer(serializer);
	return true;
}

bool S2Engine::fillSaveGameDescriptor(Common::InSaveFile &in, const int slotNr, SaveStateDescriptor &descriptor, const bool setAllProperties) {

	S2SaveGameMetadata metadata;
	if (!readSaveGameMetadata(in, metadata)) {
		return false;
	}

	descriptor = SaveStateDescriptor(slotNr, metadata.name);

	if (setAllProperties) {
		Graphics::Surface *const thumbnail = Graphics::loadThumbnail(in);
		descriptor.setThumbnail(thumbnail);

		auto date(timestampToLocal(metadata.timestamp));

		descriptor.setSaveDate(date.tm_year + 1900, date.tm_mon + 1, date.tm_mday);
		descriptor.setSaveTime(date.tm_hour, date.tm_min);
		descriptor.setPlayTime(metadata.numTicksElapsed * 1000 / 60);
	}

	return true;
}

void S2Engine::initializePath(const Common::FSNode &gamePath) {
	Engine::initializePath(gamePath);
	SearchMan.addSubDirectoryMatching(gamePath, "data");
	SearchMan.addSubDirectoryMatching(gamePath, "movies");
	SearchMan.addSubDirectoryMatching(gamePath, "patches");
	SearchMan.addSubDirectoryMatching(gamePath, "robots");
}

Common::Error S2Engine::run() {
	_kernel.reset(new S2Kernel(_system, *this, _metadata));
	_game.reset(new S2Game(*this, *_kernel));
	_debugger.reset(new S2Debugger(*_kernel, *_game));
	_game->run();
	return Common::kNoError;
}

GUI::Debugger *S2Engine::getDebugger() {
	return _debugger.get();
}

bool S2Engine::hasFeature(const EngineFeature f) const {
	switch (f) {
	case Engine::kSupportsRTL:
	case Engine::kSupportsSubtitleOptions:
	case Engine::kSupportsSavingDuringRuntime:
	case Engine::kSupportsLoadingDuringRuntime:
		return true;
	default:
		return false;
	}
}

bool S2Engine::canSaveGameStateCurrently() {
	return _game->canSaveNow();
}

bool S2Engine::canLoadGameStateCurrently() {
	return _game->canLoadNow();
}

Common::StringArray S2Engine::listSaves() {
	return getSaveFileManager()->listSavefiles(_gameId + ".###");
}

Common::Error S2Engine::saveGameState(const int slotNo, const Common::String &description) {
	using S = Common::String;
	Common::ScopedPtr<Common::OutSaveFile> file(getSaveFileManager()->openForSaving(S::format("%s.%03d", _gameId.c_str(), slotNo)));
	if (!file) {
		return Common::kWritingFailed;
	}

	file->writeUint16BE(kSaveMagic);
	Common::Serializer serializer(nullptr, file.get());
	serializer.syncVersion(kSaveVersion);

	S2SaveGameMetadata metadata;
	metadata.name = description;
	metadata.timestamp = getCurrentTimestamp();
	metadata.numTicksElapsed = _kernel->timeManager.getTickCount();
	metadata.saveLoadWithSerializer(serializer);

	// TODO: Should save a thumbnail generated from before the global room
	Graphics::saveThumbnail(*file);

	_game->saveLoadWithSerializer(serializer);

	return Common::kNoError;
}

Common::Error S2Engine::loadGameState(const int slotNo) {
	using S = Common::String;
	Common::ScopedPtr<Common::InSaveFile> file(getSaveFileManager()->openForLoading(S::format("%s.%03d", _gameId.c_str(), slotNo)));
	if (!file) {
		return Common::kReadingFailed;
	}

	if (file->readUint16BE() != kSaveMagic) {
		return Common::kReadingFailed;
	}

	Common::Serializer serializer(file.get(), nullptr);
	if (!serializer.syncVersion(kSaveVersion)) {
		return Common::kReadingFailed;
	}

	S2SaveGameMetadata metadata;
	metadata.saveLoadWithSerializer(serializer);
	_game->setSaveGameName(metadata.name);

	Graphics::skipThumbnail(*file);

	_game->saveLoadWithSerializer(serializer);

	return Common::kNoError;
}

int S2Engine::getInitialLoadSlot() const {
	return ConfMan.getInt("save_slot");
}

} // End of namespace Sci
