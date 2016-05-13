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

#include <ghostuser/graphics/text/font.hpp>
#include <ghostuser/graphics/text/font_manager.hpp>
#include <ghostuser/io/files/file_utils.hpp>
#include <ghostuser/utils/logger.hpp>
#include <string.h>

/**
 *
 */
g_font::g_font(std::string name, uint8_t* source, uint32_t sourceLength, g_font_style style, bool hint) :
		name(name), data(0), face(0), okay(false), style(style), activeSize(0), hint(hint) {

	data = new uint8_t[sourceLength];
	memcpy(data, source, sourceLength);

	// Check data
	if (data == 0) {
		g_logger::log("failed to allocate memory buffer for font '" + name + "'");
		return;
	}

	// Load font
	FT_Error error = FT_New_Memory_Face(g_font_manager::getInstance()->getLibraryHandle(), data, sourceLength, 0, &face);
	if (error) {
		g_logger::log("freetype2 failed at FT_New_Memory_Face with error code %i", error);

		delete data;
		return;
	}

	// create cairo face
	cairo_face = cairo_ft_font_face_create_for_ft_face(face, 0);

	okay = true;
}

/**
 *
 */
g_font::~g_font() {
	if (okay) {

		// destroy cairo face
		cairo_font_face_destroy(cairo_face);

		// destroy font
		FT_Done_Face(face);

		delete data;
	}

}

/**
 *
 */
g_font* g_font::fromFile(FILE* in, std::string name) {

	g_font* existing = g_font_manager::getInstance()->getFont(name);
	if (existing) {
		return existing;
	}

	int64_t length = g_length(fileno(in));
	if (length == -1) {
		return 0;
	}

	uint8_t* fileContent = new uint8_t[length];
	if (!g_file_utils::tryReadBytes(in, 0, fileContent, length)) {
		delete fileContent;
		return 0;
	}

	bool created = g_font_manager::getInstance()->createFont(name, fileContent, length);
	if (!created) {
		delete fileContent;
		return 0;
	}

	return g_font_manager::getInstance()->getFont(name);
}

/**
 *
 */
bool g_font::isOkay() {
	return okay;
}

