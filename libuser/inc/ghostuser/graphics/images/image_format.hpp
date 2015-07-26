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

#ifndef __GHOST_USER_LIBRARY__IMAGES_IMAGEFORMAT__
#define __GHOST_USER_LIBRARY__IMAGES_IMAGEFORMAT__

#include <string>

class g_image;

/**
 *
 */
enum class g_image_encoding_status {
	SUCCESSFUL, FAILED
};

/**
 *
 */
enum class g_image_decoding_status {
	SUCCESSFUL, FAILED
};

/**
 *
 */
class g_image_format {
public:
	virtual ~g_image_format() {
	}

	virtual std::string getName() = 0;

	virtual g_image_decoding_status decode(FILE* input, g_image& image) = 0;
	virtual g_image_encoding_status encode(g_image& image, FILE* output) = 0;
};

#endif
