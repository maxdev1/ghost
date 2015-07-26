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

#include <ghostuser/graphics/images/image.hpp>
#include <ghostuser/graphics/images/image_format.hpp>
#include <ghostuser/utils/logger.hpp>
#include <string.h>

/**
 *
 */
g_image_load_status g_image::load(FILE* file, g_image_format* format) {

	destroy();

	if (format == 0) {
		return g_image_load_status::FAILED;
	}

	g_image_decoding_status decodingStatus = format->decode(file, *this);
	if (decodingStatus == g_image_decoding_status::SUCCESSFUL) {
		return g_image_load_status::SUCCESSFUL;
	}

	return g_image_load_status::FAILED;
}

/**
 *
 */
void g_image::destroy() {

	if (buffer) {
		delete buffer;
		buffer = 0;
	}
}

/**
 *
 */
g_color_argb g_image::getARGB(int32_t x, int32_t y) {
	if (buffer == 0) {
		return 0;
	}

	if (x < 0 || y < 0 || x >= width || y >= height) {
		return 0;
	}

	return buffer[x + y * width];
}

/**
 *
 */
g_color_argb* g_image::getContent() {
	return buffer;
}
