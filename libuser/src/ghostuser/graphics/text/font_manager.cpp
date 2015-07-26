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

#include <ghostuser/graphics/text/font_manager.hpp>
#include <ghostuser/utils/logger.hpp>

static g_font_manager* instance = 0;

/**
 * @see header
 */
g_font_manager* g_font_manager::getInstance() {
	if (instance == 0) {
		instance = new g_font_manager();
	}
	return instance;
}

/**
 * @see {initializeEngine}
 */
g_font_manager::g_font_manager() {
	initializeEngine();
}

/**
 * @see {destroyEngine}
 */
g_font_manager::~g_font_manager() {
	destroyEngine();
}

/**
 * @see header
 */
void g_font_manager::initializeEngine() {
	FT_Error error = FT_Init_FreeType(&library);
	if (error) {
		g_logger::log("freetype2 failed at FT_Init_FreeType with error code %i", error);
	}
}

/**
 * @see header
 */
void g_font_manager::destroyEngine() {
	FT_Error error = FT_Done_Library(library);
	if (error) {
		g_logger::log("freetype2 failed at FT_Done_Library with error code %i", error);
	}
}

/**
 * @see header
 */
g_font* g_font_manager::getFont(std::string name) {
	if (fontRegistry.count(name) > 0) {
		return fontRegistry[name];
	}
	return 0;
}

/**
 * @see header
 */
bool g_font_manager::createFont(std::string name, uint8_t* source, uint32_t sourceLength, g_font_style style, bool hint) {

	if (fontRegistry.count(name) > 0) {
		g_logger::log("tried to create font '" + name + "' that already exists");
		return false;
	}

	// Create font object
	g_font* font = new g_font(name, source, sourceLength, style, hint);
	if (!font->isOkay()) {
		delete font;
	}

	// Register font
	fontRegistry[name] = font;

	g_logger::log("created font '" + name + "'");
	return true;
}

/**
 * @see header
 */
void g_font_manager::destroyFont(g_font* font) {
	fontRegistry.erase(font->getName());
	delete font;
}

/**
 * @see header
 */
FT_Library g_font_manager::getLibraryHandle() {
	return library;
}
