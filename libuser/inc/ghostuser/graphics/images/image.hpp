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

#ifndef __GHOST_USER_LIBRARY__IMAGES_IMAGE__
#define __GHOST_USER_LIBRARY__IMAGES_IMAGE__

#include <ghostuser/graphics/color_argb.hpp>
#include <stdint.h>
#include <stdio.h>

class g_image_format;

/**
 *
 */
enum class g_image_load_status {
	SUCCESSFUL, FAILED
};

/**
 *
 */
class g_image {
public:
	g_color_argb* buffer;
	uint16_t width;
	uint16_t height;

	/**
	 *
	 */
	g_image() :
			buffer(0), width(0), height(0) {
	}

	/**
	 *
	 */
	g_image_load_status load(FILE* file, g_image_format* format);

	/**
	 *
	 */
	void destroy();

	/**
	 *
	 */
	g_color_argb getARGB(int32_t x, int32_t y);

	/**
	 *
	 */
	g_color_argb* getContent();

	/**
	 *
	 */
	uint16_t getWidth() {
		return width;
	}

	/**
	 *
	 */
	uint16_t getHeight() {
		return height;
	}
};

#endif
