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

#ifndef SCI_DEBUGGER_H
#define SCI_DEBUGGER_H

#include "gui/debugger.h"
#include "sci/resource/resource.h"

namespace Sci {

class Audio32;
struct EngineState;
class GfxFrameout;
class ResourceManager;
struct reg_t;

class Debugger : public GUI::Debugger {
public:
	Debugger(ResourceManager *resMan, GfxFrameout *frameout, Audio32 *audio, EngineState *state);

protected:
	ResourceManager *_resMan;
	EngineState *_gameState;

#pragma mark -
#pragma mark Command argument parsers
protected:
	// Refer to the "addresses" command on how to pass address parameters
	int parse_reg_t(EngineState *s, const char *str, reg_t *dest, bool mayBeValue) const;

	bool parseInteger(const char *argument, int &result) const;

	ResourceType parseResourceType(const char *resourceId) const;

	bool parseResourceNumber36(const char *userParameter, uint16 &resourceNumber, uint32 &resourceTuple) const;

#pragma mark -
#pragma mark General
protected:
	virtual bool cmdHelp(int argc, const char **argv) = 0;

#pragma mark -
#pragma mark Resources
protected:
	void printResourcesHelp() const;

private:
	bool cmdDiskDump(int argc, const char **argv);
	void cmdDiskDumpWorker(ResourceType resourceType, int resourceNumber, uint32 resourceTuple);
	bool cmdHexDump(int argc, const char **argv);
	bool cmdResourceId(int argc, const char **argv);
	bool cmdResourceInfo(int argc, const char **argv);
	bool cmdResourceTypes(int argc, const char **argv);
	bool cmdList(int argc, const char **argv);
	bool cmdResourceIntegrityDump(int argc, const char **argv);
	bool cmdAllocList(int argc, const char **argv);
	bool cmdHexgrep(int argc, const char **argv);

	void writeIntegrityDumpLine(const Common::String &statusName, const Common::String &resourceName, Common::WriteStream &out, Common::ReadStream *const data, const int size, const bool writeHash) const;

#pragma mark -
#pragma mark Graphics
protected:
	void printGraphicsHelp() const;

	GfxFrameout *_gfxFrameout;

private:
	bool cmdPlaneList(int argc, const char **argv);
	bool cmdVisiblePlaneList(int argc, const char **argv);
	bool cmdPlaneItemList(int argc, const char **argv);
	bool cmdVisiblePlaneItemList(int argc, const char **argv);

#pragma mark -
#pragma mark Music/SFX
protected:
	void printAudioHelp() const;

	Audio32 *_audio32;

private:
	bool cmdAudioList(int argc, const char **argv);
	bool cmdAudioDump(int argc, const char **argv);
};

} // End of namespace Sci

#endif
