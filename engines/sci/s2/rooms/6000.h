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

#ifndef SCI_S2_ROOMS_6000_H
#define SCI_S2_ROOMS_6000_H

#include "common/ptr.h"
#include "sci/s2/system/glscript.h"
#include "sci/s2/room.h"

namespace Sci {

class S2Room6000 : public S2Room {
public:
	using S2Room::S2Room;

	virtual void init(const int roomNo) override;
	virtual void dispose(const int roomNo) override;

private:
	void initBakery();
	void initBank();
	void initBarber();
	void initWarehouse();
	void initToes();
	void initMausoleum();
	void initBook();

	void enter(const int roomNo, const uint16 enterSound, const uint16 exitSound, const bool addExit);
	void enterScript(GLScript &, const int);
	void cancelAction(GLScript &, const int);
	uint16 _enterSoundNo = 0;
	uint16 _exitSoundNo = 0;
	Common::ScopedPtr<GLCel> _cel;
	Common::ScopedPtr<GLCycler> _cycler;

	void goToEthereal(GLScript &, const int);
	Common::ScopedPtr<GLScript> _ethereal;
	bool _choose = false;

	void animateFan(GLScript &, const int);
	void animateSign(GLScript &, const int);
	void animateFlag(GLScript &, const int);
	void animateBirds(GLScript &, const int);
	void animateCafeLight(GLScript &, const int);
	void animatePole(GLScript &, const int);
	void animateMotor(GLScript &, const int);
	void animateShadow(GLScript &, const int);
	Common::ScopedPtr<GLScript> _fan;
	Common::ScopedPtr<GLScript> _sign;
	Common::ScopedPtr<GLScript> _flag;
	Common::ScopedPtr<GLScript> _birds;
	Common::ScopedPtr<GLScript> _cafeLight;
	int _lightState;
	Common::ScopedPtr<GLScript> _pole;
	Common::ScopedPtr<GLScript> _pole2;
	Common::ScopedPtr<GLScript> _motor;
	Common::ScopedPtr<GLScript> _shadow;
	S2PanoramaSprite *_panoramaSprite0;
	S2PanoramaSprite *_panoramaSprite1;
	S2PanoramaSprite *_panoramaSprite2;
	Common::ScopedPtr<S2PanoramaCycler> _panoramaCycler;
	Common::ScopedPtr<S2PanoramaCycler> _panoramaCycler1;
	Common::ScopedPtr<GLCycler> _cycler2;

	void drawPole(const int roomNo);
	void showPoleNote(GLScript &, const int);

	void showShadow(GLScript &, const int);
	void addShadowRoomExits();

	void useGasPump(GLScript &, const int);

	void prySiding(GLScript &, const int);

	void openKeyBox(GLScript &, const int);

	void showMotelSign(GLScript &, const int);

	void openStatue(GLScript &, const int);

	void openRock(GLScript &, const int);

	void fillGas(GLScript &, const int);
	void startMotor(GLScript &, const int);

	void openGate(GLScript &, const int);

	void showNorah(GLScript &, const int);
	uint16 _norahDuration;

	void chooseEnemy(GLScript &, const int);
	Common::ScopedPtr<GLScript> _norah;
};

} // End of namespace Sci

#endif
