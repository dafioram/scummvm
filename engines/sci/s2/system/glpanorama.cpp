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
#include "sci/s2/game.h"
#include "sci/s2/system/glpanorama.h"

namespace Sci {

S2Game *GLPanorama::_game = nullptr;

GLPanorama::GLPanorama(const Common::Rect &drawRect) :
	GLObject(),
	_screen(drawRect),
	_plane(drawRect, _screen),
	_image({}),
	_panX(0),
	_panY((512.0 * 1.3 - drawRect.height()) / 2),
	_shiftY(22),
	_aspectRatio(1.3),
	_isDirty(false),
	_isUpdating(false),
	_isFrozen(false) {
	buildWarpTable();
	for (int i = 0; i < 2048; ++i) {
		_panTable[i] = 50 + 50 * sin(i / 326.11);
	}
	for (int i = 0; i < 2048; ++i) {
		_percentTable[i] = MIN<int>(100, 82 + 32 * cos(i / 326.11));
	}
	_width = _screen.getWidth() / 8;
	_height = _screen.getHeight() / 8;
	_deadZone = Common::Rect(
		_plane.getRect().left + _width,
		_plane.getRect().top + _height,
		_plane.getRect().right - _width,
		_plane.getRect().bottom - _height
	);
}

void GLPanorama::drawPic(const uint16 panoramaNo, const bool shouldUpdate) {
	if (panoramaNo != _resourceNo) {
		_image.loadImage(panoramaNo);
	}

	_resourceNo = panoramaNo;
	if (shouldUpdate) {
		_isDirty = true;
	}
}

void GLPanorama::updatePanorama(const bool force) {
	if (_isDirty || force) {
		stretchPanorama();
		if (!_sounds.empty()) {
			panAudio();
		}
		_isDirty = false;
	}
}

void GLPanorama::doIt() {
	if (!_game->getCursor().hasInventory()) {
		if (_game->getUser().getIsHandsOn()) {
			if (_game->getCursor().isHandsOff()) {
				_game->getCursor().goHandsOn();
			}
		} else if (!_game->getCursor().isHandsOff()) {
			_game->getCursor().endHighlight();
			_game->getCursor().goHandsOff();
		}
	}

	if (_game->getUser().getIsHandsOn()) {
		checkMouse();
	}

	if (_image.hasSprites()) {
		updateSprites();
	}

	if (_isDirty) {
		stretchPanorama();
		_plane.updateScreen();
		if (!_sounds.empty()) {
			panAudio();
		}
		_isDirty = false;
	}
}

void GLPanorama::updateSprites() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

void GLPanorama::checkMouse() {
	auto mouse(_game->getUser().getMousePosition());
	bool shouldHighlight = false;
	// There was an off-by-one here in SSCI in the mouse y-axis check
	if (_game->getInterface().getPlane().checkIsOnMe(mouse) && mouse.y < 384) {
		if (!_isFrozen) {
			int deltaX = 0, deltaY = 0, signX = 0, signY = 0;
			if (mouse.x < _deadZone.left) {
				deltaX = _deadZone.left - mouse.x;
				signX = -1;
			} else if (mouse.x >= _deadZone.right) {
				deltaX = mouse.x - _deadZone.right;
				signX = 1;
			}

			if (mouse.y < _deadZone.top) {
				deltaY = _deadZone.top - mouse.y;
				signY = -1;
			} else if (mouse.y >= _deadZone.bottom) {
				deltaY = mouse.y - _deadZone.bottom;
				signY = 1;
			}

			// This original scaling method makes it hard to pan small amounts
			// because it clamps to a very tiny range
#ifdef SCI_USE_ORIGINAL_PANORAMA_SCALING
			const int xRange = 3, yRange = 4;
			deltaX = MIN(xRange * deltaX / _width, xRange - 1);
			deltaY = MIN((yRange - 1) * deltaY / _height, yRange - 1);
			_panX += signX * _game->getPanSpeed() / (xRange - deltaX);
			_panY += signY * _game->getPanSpeed() / (yRange - deltaY);
#else
			const int xRange = _deadZone.left, yRange = _deadZone.top;
			deltaX = MIN(deltaX, xRange);
			deltaY = MIN(deltaY, yRange);
			_panX += signX * _game->getPanSpeed() * deltaX / xRange;
			_panY += signY * _game->getPanSpeed() / 2 * deltaY / yRange;
#endif

			if (signX || signY) {
				_isDirty = true;
			}
		}

		// SSCI did not check if the mouse cursor was within the plane bounds
		// here, which would result in a garbage read to calculate the projected
		// point if the mouse is outside the bounds of the projection
		if (_plane.checkIsOnMe(mouse) && !_game->getCursor().hasInventory()) {
			auto highlightedCel(_game->getCursor().getHighlightedCelRes());
			const auto planeRect(_plane.getRect());

			mouse.x -= planeRect.left;
			mouse.y -= planeRect.top;

			const GLPoint projectedPoint((mouse.x + _panX) % _image.getHeight(),
										 (_xToYInitial[mouse.y] + (_panY + mouse.y) * _xToYDelta[mouse.x]) >> _shiftY);
			for (const auto &exit : _exits) {
				if (exit.contains(projectedPoint)) {
					const auto cursorCel = exit.getCursorCel();

					if (cursorCel == 1 && !_game->getRoomManager().getAutoHighlight()) {
						continue;
					}

					if (cursorCel != 0) {
						highlightedCel.celNo = exit.getCursorCel();
						_game->getCursor().setHighlightedCelRes(highlightedCel);
						shouldHighlight = true;
						break;
					}
				}
			}
		}
	}

	if (!_game->getCursor().hasInventory()) {
		if (shouldHighlight && !_game->getCursor().isHighlighted()) {
			_game->getCursor().beginHighlight();
		} else if (!shouldHighlight && _game->getCursor().isHighlighted()) {
			_game->getCursor().endHighlight();
		}
	}
}

bool GLPanorama::handleEvent(GLEvent &event) {
	if (event.getType() == kSciEventKeyDown) {
		switch (event.getMessage()) {
		case 'w':
		case 'W':
		case kSciKeyUp:
			_panY -= _game->getPanSpeed() / 2;
			_isDirty = true;
			return true;
		case 'a':
		case 'A':
		case kSciKeyLeft:
			_panX -= _game->getPanSpeed();
			_isDirty = true;
			return true;
		case 's':
		case 'S':
		case kSciKeyDown:
			_panY += _game->getPanSpeed() / 2;
			_isDirty = true;
			return true;
		case 'd':
		case 'D':
		case kSciKeyRight:
			_panX += _game->getPanSpeed();
			_isDirty = true;
			return true;
		}
	} else if (event.getType() == kSciEventMousePress) {
		return checkSprites(event);
	} else if (event.getType() == kSciEventMouseRelease) {
		return checkExits(event);
	}
	return false;
}

void GLPanorama::buildWarpTable() {
	const double halfWidth = _screen.getWidth() / 2.0;
	for (int x = 0; x < _screen.getWidth(); ++x) {
		double a = ABS(halfWidth - x) / halfWidth;

		a = a * a * 0.17;
		_xToYDelta[x] = ldexp((0.9999 - a) / _aspectRatio, _shiftY);

		a = a * halfWidth / _aspectRatio;
		_xToYInitial[x] = ldexp(a, _shiftY);
	}
}

void GLPanorama::stretchPanorama() {
	// SSCI re-set plane pixel buffer to screen pixel buffer, but this was
	// already done at construction time of the plane, so is omitted

	while (_panX > _image.getHeight()) {
		_panX -= _image.getHeight();
	}
	while (_panX < 0) {
		_panX += _image.getHeight();
	}

	const int16 yMax = _image.getWidth() * _aspectRatio - _screen.getHeight();
	_panY = CLIP<int16>(_panY, 0, yMax);

	const auto startX = _image.getHeight() - _panX - 1;

	for (auto x = 0; x < _screen.getWidth(); ++x) {
		int16 sourceX = startX - x;
		if (sourceX < 0) {
			sourceX += _image.getHeight();
		}
		const byte *source = _image.getPixels() + sourceX * _image.getWidth();

		const int32 xWarp = _xToYDelta[x];
		int32 xWarp2 = _xToYInitial[x] + _panY * xWarp;

		byte *target = _screen.getPixels() + x;
		for (auto y = 0; y < _screen.getHeight(); ++y) {
			*target = *(source + (xWarp2 >> _shiftY));
			xWarp2 += xWarp;
			target += _screen.getWidth();
		}
	}
}

bool GLPanorama::checkSprites(GLEvent &event) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
	return false;
}

bool GLPanorama::checkExits(GLEvent &event) {
	warning("TODO: %s", __PRETTY_FUNCTION__);
	return false;
}

void GLPanorama::panAudio() {
	warning("TODO: %s", __PRETTY_FUNCTION__);
}

} // End of namespace Sci
