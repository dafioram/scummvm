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

#ifndef SCI_S2_SYSTEM_GLOBJECT_H
#define SCI_S2_SYSTEM_GLOBJECT_H

#include "common/textconsole.h"

namespace Sci {

class GLCue;
class GLEvent;

class GLObject {
public:
	enum Flag {
		kIsScreenItem = 1,
		kNeedsDoIt    = 2,
		kNeedsEvent   = 4
	};

	GLObject() :
		_flags(0) {}

	virtual ~GLObject() {}

	inline bool getIsScreenItem() const { return _flags & kIsScreenItem; }
	inline void setIsScreenItem(const bool set) { updateFlags(kIsScreenItem, set); }

	inline bool getNeedsDoIt() const { return _flags & kNeedsDoIt; }
	inline void setNeedsDoIt(const bool set) { updateFlags(kNeedsDoIt, set); }
	virtual void doIt() { error("DoIt not allowed for this class"); }

	inline bool getNeedsEvent() const { return _flags & kNeedsEvent; }
	inline void setNeedsEvent(const bool set) { updateFlags(kNeedsEvent, set); }
	virtual bool handleEvent(GLEvent &event) { error("HandleEvent not allowed for this class"); }

	virtual void cue(GLCue &) {}

private:
	int _flags;

	inline void updateFlags(const Flag value, const bool set) {
		if (set) {
			_flags |= value;
		} else {
			_flags &= ~value;
		}
	}
};

} // End of namespace Sci

#endif
