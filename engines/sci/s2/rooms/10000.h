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

#ifndef SCI_S2_ROOMS_10000_H
#define SCI_S2_ROOMS_10000_H

#include "sci/s2/room.h"
#include "sci/s2/rooms/phone.h"
#include "sci/s2/system/glcycler.h"

namespace Sci {

class S2Room10000 : public S2Room, PhoneRoom {
public:
	using S2Room::S2Room;
	virtual void init(const int roomNo) override;
	virtual void dispose(const int roomNo) override;
	virtual void doIt() override {}
	virtual bool handleEvent(GLEvent &event) override;

private:
	Common::ScopedPtr<GLCycler> _cycler;
	Common::ScopedPtr<GLCel> _cel;

#pragma mark -
#pragma mark Cancel action
private:
	void cancelScript(GLScript &, const int state);
	int _cancelSoundNo = 0;
	int _lastRoomNo = 0;

#pragma mark -
#pragma mark TV
private:
	int _videoNo = 0;

#pragma mark -
#pragma mark Suitcase
private:
	void suitcaseScript(GLScript &, const int state);
	void takeTapeFromSuitcase(GLEvent &, GLTarget &);
	void putTapeInSuitcase(GLEvent &, GLTarget &);

	bool _suitcaseIsOpen = false;

#pragma mark -
#pragma mark Dresser
private:
	void openDrawer(GLEvent &, GLTarget &);
	void drawerScript(GLScript &, const int state);
	void addDresserHotspots();

	S2Hotspot *_topDrawer = nullptr;
	S2Hotspot *_bottomDrawer = nullptr;
	S2Hotspot *_prayerStickHotspot = nullptr;
	bool _topDrawerIsOpen = false;
	bool _bottomDrawerIsOpen = false;

#pragma mark -
#pragma mark Radio
private:
	void playRadio(const uint16 soundNo);

	bool _radioIsOn = false;
	int _radioSoundNo = 0;
};

} // End of namespace Sci

#endif
