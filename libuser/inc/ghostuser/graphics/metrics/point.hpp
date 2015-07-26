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

#ifndef GHOSTLIBRARY_GRAPHICS_METRICS_POINT
#define GHOSTLIBRARY_GRAPHICS_METRICS_POINT

#include <cstdint>

/**
 *
 */
class g_point {
public:
	int32_t x;
	int32_t y;

	g_point() :
			x(0), y(0) {
	}

	g_point(int32_t _x, int32_t _y) :
			x(_x), y(_y) {
	}

	g_point(const g_point& p) :
			x(p.x), y(p.y) {
	}

	/**
	 *
	 */
	g_point& operator=(const g_point& rhs) {
		x = rhs.x;
		y = rhs.y;
		return *this;
	}

	/**
	 *
	 */
	bool operator==(const g_point& p) const {
		return x == p.x && y == p.y;
	}

	/**
	 *
	 */
	bool operator!=(const g_point& p) const {
		return !(*this == p);
	}

	/**
	 *
	 */
	g_point operator-(const g_point& p) const {
		return g_point(x - p.x, y - p.y);
	}

	/**
	 *
	 */
	g_point operator+(const g_point& p) const {
		return g_point(x + p.x, y + p.y);
	}

};

#endif
