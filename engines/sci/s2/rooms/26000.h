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

#ifndef SCI_S2_ROOMS_26000_H
#define SCI_S2_ROOMS_26000_H

#include "sci/s2/room.h"

namespace Sci {

class S2Room26000 : public S2Room {
public:
	using S2Room::S2Room;

	virtual void init(const int) override;
	virtual void dispose(const int) override;
	virtual void doIt() override;

	void setFinalSequence(const int state) {
		setScript(this, &S2Room26000::finalSequence, state);
	}

private:
	void enterFromRelative(const int roomNo, const int16 panXDelta, const int16 elseDelta) {
		if (room().getPreviousRoomNo() == roomNo) {
			room().getPanorama().panX() += panXDelta;
		} else {
			room().getPanorama().panX() += elseDelta;
		}
	}

	void cancelScript(GLScript &, const int);

	void useTapePlayer(GLTarget &);
	void tapePlayer(GLScript &, const int);

	void drawPrayerSticks();
	void setPrayerStickFlags();

	void losePrayerStick(GLScript &, const int);
	Common::ScopedPtr<S2PanoramaSprite> _prayerStick;
	Common::ScopedPtr<GLScript> _prayerScript;
	Common::ScopedPtr<S2PanoramaJump> _mover;
	Common::FixedArray<bool, kGameFlag126 - kGameFlag115 + 1> _sticks = {};

	void takeIanyi(GLScript &, const int);
	void amuletSandMismatch(GLScript &, const int);
	void lookAtSand(GLScript &, const int);

	void summonSpirit(GLScript &, const int);
	uint16 _spiritSoundNo;

	GameFlag _flag;
	bool _gaveFakeAmulet = false;
	bool _fakeIsInAltar = false;

	void endgame(GLScript &, const int);
	Common::FixedArray<Common::ScopedPtr<GLEndCycler>, 5> _endCyclers;
	Common::FixedArray<GLCel *, 5> _endCels;
	int _globalRoomNo;

	void finalSequence(GLScript &, const int);
};

} // End of namespace Sci

#endif
