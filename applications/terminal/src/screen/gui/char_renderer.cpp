/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "char_renderer.hpp"
#include <libwindow/font/font_loader.hpp>

char_renderer_t::char_renderer_t()
{
	font = g_font_loader::get(CHAR_RENDERER_DEFAULT_FONT);

	fontOptions = cairo_font_options_create();
	cairo_font_options_set_antialias(fontOptions, CAIRO_ANTIALIAS_NONE);
}


void char_renderer_t::printChar(cairo_t* cr, int x, int y, char c)
{
	char_layout_t* charLayout = layoutChar(scaledFont, c);
	if(charLayout == nullptr)
		return;

	cairo_text_extents_t charExtents;
	cairo_scaled_font_glyph_extents(scaledFont, charLayout->glyph_buffer, 1, &charExtents);

	cairo_save(cr);
	cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1);
	cairo_translate(cr, x, y);
	cairo_glyph_path(cr, charLayout->glyph_buffer, charLayout->cluster_buffer[0].num_glyphs);
	cairo_fill(cr);
	cairo_restore(cr);
}

char_layout_t* char_renderer_t::layoutChar(cairo_scaled_font_t* scaledFont, char c)
{
	auto entry = charLayoutCache.find(c);
	if(entry != charLayoutCache.end())
		return (*entry).second;

	char buffer[2];
	buffer[0] = c;
	buffer[1] = 0;

	auto layout = new char_layout_t();
	cairo_text_cluster_flags_t cluster_flags;
	const cairo_status_t stat = cairo_scaled_font_text_to_glyphs(scaledFont, 0, 0, buffer, 1, &layout->glyph_buffer,
	                                                             &layout->glyph_count, &layout->cluster_buffer,
	                                                             &layout->cluster_count, &cluster_flags);
	if(stat == CAIRO_STATUS_SUCCESS)
	{
		charLayoutCache[c] = layout;
		return layout;
	}

	delete layout;
	return nullptr;
}


void char_renderer_t::prepareContext(cairo_t* cr, double fontSize)
{
	cairo_set_font_face(cr, font->getFace());
	cairo_set_font_size(cr, fontSize);
	scaledFont = cairo_get_scaled_font(cr);
	cairo_set_font_options(cr, fontOptions);
}
