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

#ifndef GHOSTLIBRARY_GRAPHICS_METRICS_DIMENSION
#define GHOSTLIBRARY_GRAPHICS_METRICS_DIMENSION

#include <cstdint>
#include "point.hpp"

/**
 *
 */
class g_dimension {
public:
	int32_t width;
	int32_t height;

	g_dimension() :
			width(0), height(0) {
	}

	g_dimension(int32_t width, int32_t height) :
			width(width), height(height) {
	}

	g_dimension(const g_dimension& p) :
			width(p.width), height(p.height) {
	}

	/**
	 *
	 */
	g_dimension& operator=(const g_dimension& rhs) {
		width = rhs.width;
		height = rhs.height;
		return *this;
	}

	/**
	 *
	 */
	bool operator==(const g_dimension& p) const {
		return width == p.width && height == p.height;
	}

	/**
	 *
	 */
	bool operator!=(const g_dimension& p) const {
		return !(*this == p);
	}

	/**
	 *
	 */
	g_point operator-(const g_dimension& p) const {
		return g_point(width - p.width, height - p.height);
	}

	/**
	 *
	 */
	g_point operator+(const g_dimension& p) const {
		return g_point(width + p.width, height + p.height);
	}

};

#endif
