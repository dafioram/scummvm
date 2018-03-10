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

#include "sci/s2/interface.h"
#include "sci/s2/movie_captions.h"
#include "sci/s2/movie_manager.h"

namespace Sci {
namespace S2Captions {

#define put(frameNo, nextFrameNo, textNo) \
case frameNo: \
	interface.putText(textNo, false, true); \
	movie.setFrameNo(nextFrameNo); \
	break

void caption1020(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 520, 0);
	put(520, 570, 41123);
	put(570, 625, 41125);
	put(625, 690, 41127);
	put(690, 750, 41129);
	put(750, 880, 41130);
	put(880, 915, 0);
	put(915, 985, 0);
	put(985, 9999, 0);
	}
}

void caption2000(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 178, 0);
	put(178, 248, 11001);
	put(248, 318, 11002);
	put(318, 386, 11003);
	put(386, 460, 11004);
	put(460, 596, 11005);
	put(596, 667, 11006);
	put(667, 735, 11007);
	put(735, 808, 11008);
	put(808, 9999, 11009);
	}
}

void caption2001(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 48, 0);
	put(48, 112, 11011);
	put(112, 180, 11012);
	put(180, 249, 11013);
	put(249, 377, 11014);
	put(377, 515, 11015);
	put(515, 9999, 11016);
	}
}

void caption2002(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 47, 0);
	put(47, 102, 11062);
	put(102, 225, 11063);
	put(225, 323, 11064);
	put(323, 418, 11065);
	put(418, 475, 11066);
	put(475, 540, 11067);
	put(540, 9999, 11068);
	}
}

void caption2003(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 20, 0);
	put(20, 48, 11050);
	put(48, 72, 11051);
	put(72, 94, 11052);
	put(94, 9999, 11053);
	}
}

void caption2004(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 122, 0);
	put(122, 175, 11055);
	put(175, 224, 11056);
	put(224, 273, 11057);
	put(273, 335, 11058);
	put(335, 385, 11059);
	put(385, 9999, 11060);
	}
}

void caption2005(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 34, 0);
	put(34, 88, 11041);
	put(88, 150, 11042);
	put(150, 195, 11043);
	put(195, 260, 11044);
	put(260, 311, 11045);
	put(311, 390, 11046);
	put(390, 508, 11047);
	put(508, 9999, 11048);
	}
}

void caption2006(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 66, 0);
	put(66, 112, 11018);
	put(112, 204, 11019);
	put(204, 300, 11020);
	put(300, 351, 11021);
	put(351, 398, 11022);
	put(398, 437, 11023);
	put(437, 550, 11024);
	put(550, 642, 11025);
	put(642, 9999, 11026);
	}
}

void caption2007(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 168, 0);
	put(168, 240, 11028);
	put(240, 325, 11029);
	put(325, 394, 11030);
	put(394, 497, 11031);
	put(497, 566, 11032);
	put(566, 9999, 11033);
	}
}

void caption2008(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 50, 0);
	put(50, 111, 11035);
	put(111, 228, 11036);
	put(228, 320, 11037);
	put(320, 416, 11038);
	put(416, 9999, 11039);
	}
}

void caption2009(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 55050);
	}
}

void caption2010(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 55051);
	}
}

void caption2011(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 55052);
	}
}

void caption2012(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 11072);
	}
}

void caption3001(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 11074);
	}
}

void caption3002(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 10172);
	}
}

void caption3003(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 11070);
	}
}

void caption4000(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 11076);
	}
}

void caption4010(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 40, 0);
	put(40, 88, 41201);
	put(88, 135, 41202);
	put(135, 164, 41203);
	put(164, 236, 41204);
	put(236, 368, 41205);
	put(368, 410, 41206);
	put(410, 495, 41209);
	put(495, 516, 41210);
	put(516, 596, 41211);
	put(596, 619, 41212);
	put(619, 669, 41213);
	put(669, 681, 41214);
	put(681, 727, 41215);
	put(727, 820, 41216);
	put(820, 923, 41217);
	put(923, 9999, 41218);
	}
}

void caption4020(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 50, 0);
	put(50, 165, 51650);
	put(165, 235, 51651);
	put(235, 9999, 51652);
	}
}

void caption5010(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 118, 52623);
	put(118, 9999, 52626);
	}
}

void caption5020(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 55017);
	}
}

void caption5030(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 55018);
	}
}

void caption5040(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 52001);
	}
}

void caption5050(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 55019);
	}
}

void caption5060(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 55020);
	}
}

void caption5070(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 52031);
	}
}

void caption5080(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 52501);
	}
}

void caption5090(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 55022);
	}
}

void caption5100(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 186, 51101);
	put(186, 9999, 51102);
	}
}

void caption5110(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 55028);
	}
}

void caption5120(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 55029);
	}
}

void caption53431(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 11072);
	}
}

void caption53511(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 9999, 42648);
	}
}

void caption53861(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 9999, 52611);
	}
}

void caption54171(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 190, 42652);
	put(190, 9999, 52664);
	}
}

void caption54172(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 60, 52664);
	put(60, 9999, 0);
	}
}

void caption54371(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 28, 0);
	put(28, 50, 11078);
	put(50, 172, 11079);
	put(172, 264, 11080);
	put(264, 354, 11081);
	put(354, 9999, 11082);
	}
}

void caption54771(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 40, 52670);
	put(40, 195, 0);
	put(195, 230, 42653);
	put(230, 9999, 52672);
	}
}

void caption54971(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 11, 0);
	put(11, 200, 42654);
	put(200, 9999, 42655);
	}
}

void caption55171(S2MovieManager &movie, S2Interface &interface) {
	switch (movie.getFrameNo()) {
	put(5, 100, 0);
	put(100, 172, 52673);
	put(172, 212, 52689);
	put(212, 270, 52700);
	put(270, 340, 52701);
	put(340, 395, 52702);
	put(395, 410, 52703);
	put(410, 9999, 0);
	}
}

Captioner get(const uint16 movieNo) {
	switch (movieNo) {
	case 1020:
		return caption1020;
	case 2000:
		return caption2000;
	case 2001:
		return caption2001;
	case 2002:
		return caption2002;
	case 2003:
		return caption2003;
	case 2004:
		return caption2004;
	case 2005:
		return caption2005;
	case 2006:
		return caption2006;
	case 2007:
		return caption2007;
	case 2008:
		return caption2008;
	case 2009:
		return caption2009;
	case 2010:
		return caption2010;
	case 2011:
		return caption2011;
	case 2012:
		return caption2012;
	case 3001:
		return caption3001;
	case 3002:
		return caption3002;
	case 3003:
		return caption3003;
	case 4000:
		return caption4000;
	case 4010:
		return caption4010;
	case 4020:
		return caption4020;
	case 5010:
		return caption5010;
	case 5020:
		return caption5020;
	case 5030:
		return caption5030;
	case 5040:
		return caption5040;
	case 5050:
		return caption5050;
	case 5060:
		return caption5060;
	case 5070:
		return caption5070;
	case 5080:
		return caption5080;
	case 5090:
		return caption5090;
	case 5100:
		return caption5100;
	case 5110:
		return caption5110;
	case 5120:
		return caption5120;
	case 53431:
		return caption53431;
	case 53511:
		return caption53511;
	case 53861:
		return caption53861;
	case 54171:
		return caption54171;
	case 54172:
		return caption54172;
	case 54371:
		return caption54371;
	case 54771:
		return caption54771;
	case 54971:
		return caption54971;
	case 55171:
		return caption55171;
	default:
		return nullptr;
	}
}

} // End of namespace S2Captions
} // End of namespace Sci
