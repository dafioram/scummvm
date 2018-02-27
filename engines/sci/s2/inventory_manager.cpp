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

#include "sci/s2/game.h"
#include "sci/s2/inventory_manager.h"
#include "sci/s2/scoring_manager.h"

namespace Sci {

S2InventoryManager::S2InventoryManager(S2Game &game) :
	_game(game),
	_currentItem(S2Inventory::None),
	_showingItem(S2Inventory::None),
	_numItemsHeld(0),
	_itemSlots(),
	_prayerStick(S2PrayerStick::None) {}

void S2InventoryManager::init() {
	for (int itemNo = 0; itemNo < int(S2Inventory::kNumInventory); ++itemNo) {
		S2InventoryState state;
		if (itemNo == 1 || itemNo == 2) {
			state = S2InventoryState::Taken;
		} else if (itemNo ==  3 || itemNo ==  5 || itemNo ==  6 || itemNo ==  7 ||
				   itemNo ==  9 || itemNo == 11 || itemNo == 13 || itemNo == 14 ||
				   itemNo == 15 ||
				   (itemNo >= 17 && itemNo <= 24) ||
				   (itemNo >= 26 && itemNo <= 32) ||
				   itemNo == 34 || itemNo == 35 || itemNo == 37 || itemNo == 38 ||
				   itemNo == 40 || itemNo == 41 || itemNo == 48 || itemNo == 49) {
			state = S2InventoryState::Placed;
		} else {
			state = S2InventoryState::Normal;
		}

		_inventory[itemNo] = S2InventoryItem(state, GLCelRes(3000, itemNo, 0), GLCelRes(3000 + itemNo, 0, 0));

		if (state == S2InventoryState::Taken) {
			addItem(S2Inventory(itemNo));
		}
	}

	for (int itemNo = 0; itemNo < int(S2PrayerStick::kNumPrayerSticks); ++itemNo) {
		const GLCelRes cel(910, 0, itemNo);
		_prayerSticks[itemNo] = S2InventoryItem(S2InventoryState::Normal, cel, cel);
	}
}

static void syncInventoryItem(Common::Serializer &s, S2InventoryItem &item) {
	s.syncAsByte(reinterpret_cast<uint8 &>(item.state));
}

void S2InventoryManager::saveLoadWithSerializer(Common::Serializer &s) {
	s.syncAsByte(reinterpret_cast<uint8 &>(_currentItem));
	s.syncAsByte(reinterpret_cast<uint8 &>(_prayerStick));
	s.syncArray(_inventory.data(), _inventory.size(), syncInventoryItem);
	s.syncArray(_prayerSticks.data(), _prayerSticks.size(), syncInventoryItem);
	if (s.isLoading()) {
		refresh();
	}
}

void S2InventoryManager::setCurrentItem(const S2Inventory item) {
	if (_currentItem == item) {
		return;
	}

	unselectItem(true);
	for (int slotNo = 0; slotNo < _itemSlots.size(); ++slotNo) {
		if (_itemSlots[slotNo] == item) {
			removeItem(slotNo);
		}
	}

	_game.getCursor().getItem(getSmallCel(_currentItem));
	_game.getInterface().disableButtons();
	setState(item, S2InventoryState::InUse);
	_currentItem = item;
}

void S2InventoryManager::addItem(const S2Inventory item) {
	if (_numItemsHeld >= kMaxHeldItems) {
		_game.getSoundManager().play(10005, false, Audio32::kMaxVolume);
		return;
	}

	for (const auto &heldItem : _itemSlots) {
		if (item == heldItem) {
			return;
		}
	}

	int slotNo = 0;
	while (_itemSlots[slotNo] != S2Inventory::None) {
		++slotNo;
	}
	setState(item, S2InventoryState::Taken);
	++_numItemsHeld;
	_itemSlots[slotNo] = item;
	_game.getInterface().drawInventoryItem(slotNo, item);
}

S2Inventory S2InventoryManager::removeItem(const int slotNo) {
	const S2Inventory oldItem = _itemSlots[slotNo];
	if (oldItem != S2Inventory::None) {
		_game.getInterface().eraseInventoryItem(slotNo);
		setState(oldItem, S2InventoryState::InUse);
		_itemSlots[slotNo] = S2Inventory::None;
		--_numItemsHeld;
	}
	return oldItem;
}

S2Inventory S2InventoryManager::combineItems() {
	auto itemA = _game.getInventoryManager()._currentItem;
	auto itemB = _game.getInventoryManager()._showingItem;
	if (itemB < itemA) {
		SWAP(itemA, itemB);
	}

	auto result = S2Inventory::None;
	switch (itemA) {
	case S2Inventory::Batteries:
		// In SSCI the success sound was played even when the combination failed
		if (itemB == S2Inventory::TapePlayer) {
			result = S2Inventory::TapePlayerWithBatteries;
			_game.getSoundManager().play(12415, false, 100);
		} else if (itemB == S2Inventory::TapePlayerWithTape) {
			result = S2Inventory::CompleteTapePlayer;
			_game.getSoundManager().play(12415, false, 100);
		}
		break;

	case S2Inventory::Quarter:
		if (itemB == S2Inventory::Change2_50) {
			result = S2Inventory::Change2_75;
			_game.getSoundManager().play(10007, false, 100);
		}
		break;

	case S2Inventory::CassetteTape:
		// In SSCI the success sound was played even when the combination failed
		if (itemB == S2Inventory::TapePlayer) {
			result = S2Inventory::TapePlayerWithTape;
			_game.getSoundManager().play(12416, false, 100);
		} else if (itemB == S2Inventory::TapePlayerWithBatteries) {
			result = S2Inventory::CompleteTapePlayer;
			_game.getSoundManager().play(12416, false, 100);
		}
		break;

	case S2Inventory::kInv35:
		if (itemB == S2Inventory::kInv41) {
			addItem(S2Inventory::kInv36);
			result = S2Inventory::kInv47;
			_game.getSoundManager().play(12320, false, 100);
		}
		break;

	case S2Inventory::kInv36:
		if (itemB == S2Inventory::kInv41) {
			result = S2Inventory::kInv47;
			_game.getSoundManager().play(12320, false, 100);
		}
		break;

	default:
		break;
	}

	if (result != S2Inventory::None) {
		setState(_currentItem, S2InventoryState::Used);
		setState(_showingItem, S2InventoryState::Used);
		_showingItem = result;
		unselectItem(false);
	} else {
		unselectItem(false);
	}

	return result;
}

void S2InventoryManager::showItem() {
	_showingItem = _currentItem;
	unselectItem(false);
	if (_showingItem == S2Inventory::kInv12) {
		_game.getRoomManager().loadGlobalRoom(4301);
	} else if (_showingItem == S2Inventory::kInv19) {
		_game.getRoomManager().loadGlobalRoom(4302);
	} else {
		_game.getRoomManager().loadGlobalRoom(4300);
	}
}

void S2InventoryManager::hideItem() {
	if (_numItemsHeld == _itemSlots.size()) {
		_game.getCursor().getItem(getSmallCel(_showingItem));
		setCurrentItem(_showingItem);
	} else {
		addItem(_showingItem);
	}
	_showingItem = S2Inventory::None;
	_game.getRoomManager().unloadGlobalRoom();
	_game.getInterface().resetButtons();
}

bool S2InventoryManager::setState(const S2Inventory item, const S2InventoryState state) {
	if (state == S2InventoryState::Taken &&
		(_numItemsHeld == kMaxHeldItems || getCurrentItem() != S2Inventory::None)) {

		_game.getSoundManager().play(10005, false, Audio32::kMaxVolume);
		return false;
	}

	_game.getRoomManager().setIsSaved(false);
	if (getState(item) == S2InventoryState::InUse && state == S2InventoryState::Used) {
		_game.getCursor().dropItem();
		_currentItem = S2Inventory::None;
		if (!isItemShowing()) {
			_game.getInterface().resetButtons();
		}
		if (item != S2Inventory::kInv10 && item != S2Inventory::kInv16 &&
			item != S2Inventory::kInv25 && item != S2Inventory::kInv39) {
			_game.getScoringManager().doEvent(S2Score::kScore1);
		}
	}

	if (getState(item) < S2InventoryState::Taken && state == S2InventoryState::Taken &&
		item > S2Inventory::kInv2) {
		_game.getScoringManager().doEvent(S2Score::kScore0);
	}

	_inventory[int(item)].state = state;
	return true;
}

void S2InventoryManager::selectItem(const int slotNo) {
	if (_currentItem != S2Inventory::None) {
		unselectItem(true);
	}
	_currentItem = removeItem(slotNo);
	_game.getSoundManager().play(10008, false, 100);
	_game.getCursor().getItem(_inventory[int(_currentItem)].smallCel);
	_game.getInterface().disableButtons();
}

void S2InventoryManager::unselectItem(const bool returnToInventory) {
	if (_currentItem != S2Inventory::None) {
		const S2Inventory oldItem = _currentItem;
		_currentItem = S2Inventory::None;
		_game.getCursor().dropItem();
		if (returnToInventory) {
			addItem(oldItem);
			_game.getSoundManager().play(10009, false, 100);
			if (!isItemShowing()) {
				_game.getInterface().resetButtons();
			}
		} else {
			setState(oldItem, S2InventoryState::Used);
		}
	}
}

void S2InventoryManager::refresh() {
	for (auto slotNo = 0; slotNo < kMaxHeldItems; ++slotNo) {
		_game.getInterface().eraseInventoryItem(slotNo);
		_itemSlots[slotNo] = S2Inventory::None;
	}
	_numItemsHeld = 0;
	for (auto itemNo = 0; itemNo < _inventory.size(); ++itemNo) {
		if (_inventory[itemNo].state == S2InventoryState::Taken) {
			addItem(S2Inventory(itemNo));
		}
	}
	_game.getCursor().dropItem();
	if (_currentItem != S2Inventory::None) {
		_game.getCursor().getItem(_inventory[int(_currentItem)].smallCel);
	}
	_game.getCursor().dropPrayerStick();
	if (_prayerStick != S2PrayerStick::None) {
		_game.getCursor().getPrayerStick(_prayerSticks[int(_prayerStick)].smallCel);
	}
}

} // End of namespace Sci
