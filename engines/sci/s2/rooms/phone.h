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

#ifndef SCI_S2_ROOMS_PHONE_H
#define SCI_S2_ROOMS_PHONE_H

#include "common/str.h"
#include "sci/s2/room.h"

namespace Sci {

class S2PhoneRoom : public S2Room {
public:
	using S2Room::S2Room;

protected:
	void setUpPhone(const int roomNo);

	virtual bool handleEvent(GLEvent &event) override;

private:
	void emplaceDigit(const int digit, const int16 x1, const int16 y1, const int16 x2, const int16 y2);
	void pushButton(const int digit);

	Common::String _number;
};

} // End of namespace Sci

#endif
