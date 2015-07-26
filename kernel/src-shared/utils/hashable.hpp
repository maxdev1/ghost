/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef GHOST_SHARED_UTILS_HASHCODE
#define GHOST_SHARED_UTILS_HASHCODE

#include "ghost/stdint.h"

/**
 *
 */
class g_hashable {
public:
	virtual ~g_hashable() {
	}

	/**
	 *
	 */
	template<typename T>
	uint32_t hashcode(const T* p) {
		return (uint64_t) p;
	}

	/**
	 *
	 */
	static uint32_t hashcode(const g_hashable& value) {
		return value.hashcode();
	}

	/**
	 *
	 */
	static uint32_t hashcode(uint64_t value) {
		return value;
	}

	/**
	 *
	 */
	virtual uint32_t hashcode() const = 0;
};

#endif
