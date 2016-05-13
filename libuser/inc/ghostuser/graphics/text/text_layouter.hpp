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

#ifndef GHOSTLIBRARY_GRAPHICS_TEXT_TEXTLAYOUTER
#define GHOSTLIBRARY_GRAPHICS_TEXT_TEXTLAYOUTER

#include <ghostuser/graphics/metrics/point.hpp>
#include <ghostuser/graphics/metrics/dimension.hpp>
#include <ghostuser/graphics/metrics/rectangle.hpp>
#include <ghostuser/graphics/text/font.hpp>
#include <ghostuser/graphics/text/text_alignment.hpp>
#include <vector>

/**
 *
 */
struct g_positioned_glyph {
	g_positioned_glyph() :
			line(-1), glyph(0), glyph_count(0) {
	}

	int line;
	g_point position;

	g_dimension size;
	g_point advance;

	cairo_glyph_t* glyph;
	int glyph_count;
};

/**
 *
 */
struct g_layouted_text {

	// List of glyphs with their positions
	std::vector<g_positioned_glyph> positions;

	// Bounds of the entire layouted text
	g_rectangle textBounds;

	// Buffers
	cairo_glyph_t* glyph_buffer = nullptr;
	int glyph_count;
	cairo_text_cluster_t* cluster_buffer = nullptr;
	int cluster_count;
};

/**
 *
 */
class g_text_layouter {
private:
	/**
	 *
	 */
	g_text_layouter() {
	}

public:
	/**
	 * @return the instance of the font manager singleton
	 */
	static g_text_layouter* getInstance();

	/**
	 *
	 */
	g_layouted_text* initializeBuffer();

	/**
	 *
	 */
	void layout(cairo_t* cr, const char* text, g_font* font, int size, g_rectangle bounds, g_text_alignment alignment, g_layouted_text* layout,
			bool breakOnOverflow = true);

	/**
	 *
	 */
	void destroyLayout(g_layouted_text* layout);

};

#endif
