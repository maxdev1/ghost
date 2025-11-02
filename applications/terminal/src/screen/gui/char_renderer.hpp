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

#ifndef __TERMINAL_CHAR_RENDERER__
#define __TERMINAL_CHAR_RENDERER__

#include <cairo/cairo.h>
#include <unordered_map>
#include <libfenster/font/font.hpp>

#define CHAR_RENDERER_DEFAULT_FONT "consolas"

struct char_layout_t
{
    cairo_glyph_t* glyph_buffer = nullptr;
    int glyph_count = 0;
    cairo_text_cluster_t* cluster_buffer = nullptr;
    int cluster_count = 0;
};


class char_renderer_t
{
    fenster::Font* font;
    cairo_scaled_font_t* scaledFont = nullptr;
    cairo_font_options_t* fontOptions = nullptr;

    std::unordered_map<char, char_layout_t*> charLayoutCache{};
    char_layout_t* layoutChar(cairo_scaled_font_t* scaledFont, char c);

public:
    char_renderer_t();

    void prepareContext(cairo_t* cr, double fontSize);
    void printChar(cairo_t* cr, int x, int y, char c);
};


#endif //CHAR_RENDERER_HPP
