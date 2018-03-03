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

#ifndef SCI_S2_INVENTORY_MANAGER_H
#define SCI_S2_INVENTORY_MANAGER_H

#include "common/serializer.h"
#include "common/textconsole.h"
#include "sci/s2/inventory.h"

namespace Sci {

class S2Game;

class S2InventoryManager : public Common::Serializable {
public:
	enum { kMaxHeldItems = 12 };

	S2InventoryManager(S2Game &game);

	void init();

	virtual void saveLoadWithSerializer(Common::Serializer &) override;

	bool hasPrayerStick() const { return !hasPrayerStick(S2PrayerStick::None); }
	bool hasPrayerStick(const S2PrayerStick id) const { return _prayerStick == id; }
	S2PrayerStick getPrayerStickId() const { return _prayerStick; }
	S2Inventory getCurrentItem() const { return _currentItem; }
	void setCurrentItem(const S2Inventory item);

	S2Inventory getShowingItem() const { return _showingItem; }
	bool isItemShowing() const { return _showingItem != S2Inventory::None; }

	void selectItem(const int slotNo);
	void unselectItem(const bool returnToInventory);

	// In SSCI this returned an int, but the return value was never taken
	void addItem(const S2Inventory item);
	S2Inventory removeItem(const int slotNo);
	S2Inventory combineItems();

	const GLCelRes &getShowingItemCel() const {
		return getBigCel(_showingItem);
	}
	void showItem();
	void hideItem();

	S2InventoryState getState(const S2Inventory item) const {
		return _inventory[int(item)].state;
	}
	bool setState(const S2Inventory item, const S2InventoryState state);

	const GLCelRes &getSmallCel(S2Inventory item) const { return _inventory[int(item)].smallCel; }
	const GLCelRes &getBigCel(S2Inventory item) const { return _inventory[int(item)].bigCel; }

	void removeAll(const S2Inventory item) {
		for (int slotNo = 0; slotNo < _itemSlots.size(); ++slotNo) {
			if (_itemSlots[slotNo] == item) {
				removeItem(slotNo);
			}
		}
	}

	bool isPlaced(const S2Inventory item) const {
		return getState(item) == S2InventoryState::Placed;
	}

	bool isTaken(const S2Inventory item) const {
		return getState(item) == S2InventoryState::Taken;
	}

	bool isUsed(const S2Inventory item) const {
		return getState(item) == S2InventoryState::Used;
	}

	bool isInUse(const S2Inventory item) const {
		return getState(item) == S2InventoryState::InUse;
	}

private:
	void refresh();

	S2Game &_game;
	S2Inventory _currentItem;
	S2Inventory _showingItem;

	int _numItemsHeld;
	Common::FixedArray<S2Inventory, kMaxHeldItems> _itemSlots;

	S2PrayerStick _prayerStick;

	Common::FixedArray<S2InventoryItem, int(S2Inventory::kNumInventory)> _inventory;
	Common::FixedArray<S2InventoryItem, int(S2PrayerStick::kNumPrayerSticks)> _prayerSticks;
};

} // End of namespace Sci

#endif
