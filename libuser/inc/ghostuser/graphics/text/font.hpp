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

#ifndef GHOSTLIBRARY_GRAPHICS_TEXT_FONT
#define GHOSTLIBRARY_GRAPHICS_TEXT_FONT

#include <ghostuser/graphics/text/freetype.hpp>
#include <ghostuser/graphics/text/glyph.hpp>
#include <ghostuser/io/streams/input_stream.hpp>
#include <string>
#include <map>

/**
 *
 */
enum class g_font_style
	: uint8_t {
		NORMAL, ITALIC, BOLD
};

typedef std::map<char, g_glyph*> g_font_glyph_map;
typedef std::map<int, g_font_glyph_map> g_font_size_map;

/**
 *
 */
class g_font {
private:
	uint8_t* data;
	std::string name;
	FT_Face face;
	g_font_style style;
	bool hint;

	bool okay;

	/**
	 * Glyph cache
	 */
	g_font_size_map glyphCache;

	/**
	 * Freetype sizes
	 */
	std::map<int, FT_Size> sizes;
	int activeSize;

	/**
	 * Deletes all glyphs in the cache
	 */
	void destroyGlyphs();

	/**
	 * Looks for the size in the size map, otherwise creates it
	 */
	FT_Size getSize(int size);

public:
	/**
	 * Creates an empty font with the "name". The "source" data
	 * is copied to an internal buffer.
	 *
	 * @param name			font lookup name
	 * @param source		font data
	 * @param sourceLength	font data length
	 * @param style			font style
	 * @param hint	whether to render with hinting (antialias) or not
	 */
	g_font(std::string name, uint8_t* source, uint32_t sourceLength, g_font_style style, bool hint = true);

	/**
	 * Loads a font from the file "in". If there is already a font with
	 * the "name", that font is returned.
	 *
	 * @param in	the font file
	 * @param name	the name to register the font to
	 * @return either the font, or 0
	 */
	static g_font* fromFile(FILE* in, std::string name);

	/**
	 * Destroys the font, deleting all associated Glyph
	 * objects and freeing the freetype face.
	 */
	~g_font();

	/**
	 * @return whether the font was successfully initialized.
	 */
	bool isOkay();

	/**
	 * @return the name of the font
	 */
	std::string getName() {
		return name;
	}

	/**
	 * Looks for character "c" with "size" in the glyph
	 * cache, renders it if not existant.
	 *
	 * @param size	the size to render
	 * @param c		the searched character
	 * @return the matching glyph object
	 */
	g_glyph* getGlyph(int size, char c);

	/**
	 *
	 */
	int getLineHeight(int size);

	/**
	 *
	 */
	g_point getKerning(int size, g_glyph* left, g_glyph* right);

};

#endif
