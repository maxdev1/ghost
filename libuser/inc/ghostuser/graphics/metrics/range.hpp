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

#ifndef GHOSTLIBRARY_GRAPHICS_METRICS_RANGE
#define GHOSTLIBRARY_GRAPHICS_METRICS_RANGE

#include <cstdint>

/**
 *
 */
class g_range {
private:
	int32_t first;
	int32_t last;

public:

	g_range() :
			first(0), last(0) {
	}

	g_range(int32_t _a, int32_t _b) {
		set(_a, _b);
	}

	g_range(const g_range& p) :
			first(p.first), last(p.last) {
	}

	/**
	 *
	 */
	void set(int32_t a, int32_t b) {
		first = a < b ? a : b;
		last = a > b ? a : b;
	}

	/**
	 *
	 */
	int32_t getFirst() {
		return first;
	}

	/**
	 *
	 */
	int32_t getLast() {
		return last;
	}

	/**
	 *
	 */
	int32_t getLength() {
		return last - first;
	}

	/**
	 *
	 */
	g_range& operator=(const g_range& rhs) {
		first = rhs.first;
		last = rhs.last;
		return *this;
	}

	/**
	 *
	 */
	bool operator==(const g_range& p) const {
		return (first == p.first) && (last == p.last);
	}

	/**
	 *
	 */
	bool operator!=(const g_range& p) const {
		return !(*this == p);
	}

};

#endif
