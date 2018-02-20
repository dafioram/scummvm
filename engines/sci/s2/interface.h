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

#ifndef SCI_S2_INTERFACE_H
#define SCI_S2_INTERFACE_H

#include "common/ptr.h"
#include "common/rect.h"
#include "sci/s2/bitmap.h"
#include "sci/s2/button.h"
#include "sci/s2/inventory.h"
#include "sci/s2/inventory_manager.h"
#include "sci/s2/inventory_object.h"
#include "sci/s2/system/glplane.h"
#include "sci/s2/system/glscreen_item.h"
#include "sci/s2/system/gltarget.h"

namespace Sci {

class GLEvent;
class GLScript;
class S2Game;
class S2Kernel;

// In SSCI this was of type GLObject and then got treated like a GLTarget using
// unsafe casts
class S2Interface : public GLTarget {
public:
	S2Interface(S2Kernel &kernel, S2Game &game);

	void init();

	virtual void doIt() override;
	virtual bool handleEvent(GLEvent &event) override;

	GLTransparentPlane &getPlane() const { return *_main; }

	void show();
	void hide();
	void putText(const uint16 messageNo, const bool append = false, const bool showImmediately = false);
	bool getIsCaptioningOn() const { return _isCaptioningOn; }

	void disableButtons();
	void resetButtons();

	void changeLife(const int amount);

	void drawInventoryItem(const int slotNo, const S2Inventory item);
	void eraseInventoryItem(const int slotNo);

private:
	S2Button *makeButton(const int16 loopNo, const GLButton::EventHandler &handler, const bool shouldEnable = true) const;

	void captionScript(GLScript &script, const int state);
	Common::String _activeCaptionText;
	Common::String::iterator _nextCaptionPosition;
	Common::String::iterator _currentCaptionPosition;

	void displayText(const Common::String &text);
	void clearText();
	void stopText();

	// In SSCI this was inverted
	bool _isVisible;

	int _healthRemaining;

	S2Kernel &_kernel;
	S2Game &_game;
	Common::ScopedPtr<GLColoredPlane> _background;
	Common::ScopedPtr<GLTransparentPlane> _main;
	Common::ScopedPtr<GLTransparentPlane> _captions;
	Common::ScopedPtr<GLScreenItem> _toolbar;
	Common::ScopedPtr<GLScreenItem> _health;
	Common::ScopedPtr<GLScreenItem> _healthMask;

	Common::ScopedPtr<S2Button> _internet;
	Common::ScopedPtr<S2Button> _flashback;
	Common::ScopedPtr<S2Button> _options;
	Common::ScopedPtr<S2Button> _map;
	Common::ScopedPtr<S2Button> _eye;

	Common::String _caption;
	Common::Rect _captionRect;
	Common::ScopedPtr<S2Bitmap> _captionText;
	Common::ScopedPtr<GLScreenItem> _captionUi;
	Common::ScopedPtr<GLScript> _captionScript;
	bool _isCaptioningOn;
	bool _hasCaptioningFinished;

	Common::FixedArray<Common::ScopedPtr<S2InventoryObject>, S2InventoryManager::kMaxHeldItems> _inventory;
};

} // End of namespace Sci

#endif
