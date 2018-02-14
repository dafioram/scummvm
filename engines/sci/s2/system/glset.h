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

#ifndef SCI_S2_SYSTEM_GLSET_H
#define SCI_S2_SYSTEM_GLSET_H

#include "common/algorithm.h"
#include "common/array.h"

namespace Sci {

// TODO: This seems like it should not be accepting null pointers, in which case
// the methods ought to be receiving references instead
template <typename T>
class GLSetAsArray : public Common::Array<T *> {
	using Parent = Common::Array<T *>;

public:
	using Common::Array<T *>::Array;
	using typename Parent::iterator;
	using typename Parent::const_iterator;
	using typename Parent::value_type;
	using typename Parent::size_type;

	void push_front(const value_type value) {
		insert_at(0, value);
	}

	void push_back(const value_type value) {
		assert(!contains(value));
		Parent::push_back(value);
	}

	iterator insert_at(size_type index, value_type value) {
		assert(!contains(value));
		return Parent::insert_at(index, value);
	}

	iterator insert(iterator position, value_type value) {
		assert(!contains(value));
		return Parent::insert(position, value);
	}

	// was IsInList
	bool contains(value_type value) const {
		return Common::find(this->begin(), this->end(), value) != this->end();
	}

	void remove(value_type value) {
		iterator it = Common::find(this->begin(), this->end(), value);
		if (it != this->end()) {
			this->erase(it);
		}
	}

private:
	iterator insert_at(size_type index, const Parent &array);
};

} // End of namespace Sci

#endif
