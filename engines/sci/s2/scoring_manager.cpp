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

#include "common/textconsole.h"
#include "sci/s2/scoring_manager.h"

namespace Sci {

S2ScoringManager::S2ScoringManager() :
	_currentScore(0) {
	_events[kScore0] = 2050;
	_events[kScore1] = 2500;
	_events[kScore2] = 2000;
	_events[kScore3] = 3500;
	_events[kScore4] = 7500;
	_events[kScore5] = -2000;
	_events[kScore6] = -500;

	for (int i = kScore7; i <= kScore41; ++i) {
		_events[i] = 250;
	}

	for (int i = kScore42; i <= kScore165; ++i) {
		_events[i] = 500;
	}

	for (int i = kScore166; i <= kScore174; ++i) {
		_events[i] = 650;
	}

	for (int i = kScore175; i <= kScore180; ++i) {
		_events[i] = 750;
	}

	for (int i = kScore181; i <= kScore190; ++i) {
		_events[i] = 1750;
	}

	for (int i = kScore191; i <= kScore202; ++i) {
		_events[i] = 3500;
	}

	_events[kScore203] = 17500;

	for (int i = kScore204; i <= kScore227; ++i) {
		_events[i] = 7500;
	}

	_events[kScore228] = _events[kScore229] = 18300;

	for (int i = kScore230; i <= kScore241; ++i) {
		_events[i] = 15000;
	}
}

void S2ScoringManager::doEvent(S2Score event) {
	int score = _events[event];
	if (score < 0 && ABS(score) >= _currentScore) {
		_currentScore = 0;
	} else {
		_currentScore += score;
	}

	if (event > kScore6) {
		_events[event] = 0;
	}
}

} // End of namespace Sci
