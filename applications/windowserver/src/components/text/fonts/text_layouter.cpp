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

#include "components/text/fonts/text_layouter.hpp"

#define BOUNDS_EMPTY 0xFFFFFF
static g_text_layouter* instance = 0;

g_text_layouter* g_text_layouter::getInstance()
{
    if(!instance)
        instance = new g_text_layouter();
    return instance;
}

void rightAlign(g_layouted_text* text, int line, int lineWidth, g_rectangle& bounds)
{
    int diff = bounds.width - bounds.x - lineWidth;
    for(g_positioned_glyph& g : text->positions)
    {
        if(g.line == line)
            g.position.x += diff;
    }
}

void centerAlign(g_layouted_text* text, int line, int lineWidth, g_rectangle& bounds)
{
    int diff = (bounds.width - bounds.x) / 2 - lineWidth / 2;
    for(g_positioned_glyph& g : text->positions)
    {
        if(g.line == line)
            g.position.x += diff;
    }
}

g_layouted_text* g_text_layouter::initializeBuffer()
{
    return new g_layouted_text();
}

void g_text_layouter::layout(cairo_t* cr, const char* text, g_font* font, int size,
                             g_rectangle bounds, g_text_alignment alignment,
                             g_layouted_text* layout, bool breakOnOverflow)
{
    if(!font)
        return;

    size_t text_len = strlen(text);

    // starting coordinates
    int x = bounds.x;
    int y = bounds.y;
    int lineStartX = x;

    // created the scaled font face
    cairo_set_font_face(cr, font->getFace());
    cairo_set_font_size(cr, size);
    auto scaled_face = cairo_get_scaled_font(cr);

    int line = 0;
    int lineHeight = size;

    // create glyphs for the text
    auto previous_glyph_buffer = layout->glyph_buffer;
    auto previous_cluster_buffer = layout->cluster_buffer;

    cairo_text_cluster_flags_t cluster_flags;
    cairo_status_t stat = cairo_scaled_font_text_to_glyphs(scaled_face, 0, 0, text, text_len, &layout->glyph_buffer, &layout->glyph_count,
                                                           &layout->cluster_buffer, &layout->cluster_count, &cluster_flags);

    // free old buffer
    if(previous_glyph_buffer != nullptr && layout->glyph_buffer != previous_glyph_buffer)
    {
        free(previous_glyph_buffer);
    }
    if(previous_cluster_buffer != nullptr && layout->cluster_buffer != previous_cluster_buffer)
    {
        free(previous_cluster_buffer);
    }

    // clear layout entries
    layout->positions.clear();

    // perform layouting
    if(stat == CAIRO_STATUS_SUCCESS)
    {
        // positions in bytes and glyphs
        size_t byte_pos = 0;
        size_t glyph_pos = 0;

        // text extents
        cairo_text_extents_t extents;

        for(int i = 0; i < layout->cluster_count; i++)
        {
            cairo_text_cluster_t* cluster = &layout->cluster_buffer[i];
            cairo_glyph_t* glyphs = &layout->glyph_buffer[glyph_pos];

            // create new position
            g_positioned_glyph positioned;
            positioned.glyph = glyphs;
            positioned.glyph_count = cluster->num_glyphs;
            cairo_scaled_font_glyph_extents(scaled_face, positioned.glyph, positioned.glyph_count, &extents);

            positioned.advance.x = extents.x_advance;
            positioned.advance.y = extents.y_advance;
            positioned.size.width = extents.width;
            positioned.size.height = extents.height;

            // check if newline
            bool isNewline = false;
            if(cluster->num_bytes == 1 && text[byte_pos] == '\n')
            {
                isNewline = true;
            }
            bool invisible = false;

            // Wouldn't match in line or is break character? Start next line
            if(isNewline || (breakOnOverflow && (x + positioned.size.width > bounds.width)))
            {
                if(isNewline)
                {
                    invisible = true;
                }

                if(alignment == g_text_alignment::RIGHT)
                {
                    rightAlign(layout, line, x - lineStartX, bounds);
                }
                else if(alignment == g_text_alignment::CENTER)
                {
                    centerAlign(layout, line, x - lineStartX, bounds);
                }

                ++line;
                x = bounds.x;
                lineStartX = x;
                y += lineHeight;
            }

            if(!invisible)
            {
                // Position
                positioned.line = line;
                positioned.position.x = x;
                positioned.position.y = y + lineHeight;

                // Add position
                layout->positions.push_back(positioned);

                // Jump to next
                x += positioned.advance.x;
            }

            // increase positions
            glyph_pos += cluster->num_glyphs;
            byte_pos += cluster->num_bytes;
        }
    }

    if(alignment == g_text_alignment::RIGHT)
    {
        rightAlign(layout, line, x - lineStartX, bounds);
    }
    else if(alignment == g_text_alignment::CENTER)
    {
        centerAlign(layout, line, x - lineStartX, bounds);
    }

    // Set text bounds
    int tbTop = BOUNDS_EMPTY;
    int tbLeft = BOUNDS_EMPTY;
    int tbRight = 0;
    int tbBottom = 0;

    for(g_positioned_glyph& p : layout->positions)
    {
        if(p.position.x < tbLeft)
        {
            tbLeft = p.position.x;
        }
        if(p.position.y < tbTop)
        {
            tbTop = p.position.y;
        }

        // get extents again
        int r = p.position.x + p.size.width;
        if(r > tbRight)
        {
            tbRight = r;
        }
        int b = p.position.y + p.size.height;
        if(b > tbBottom)
        {
            tbBottom = b;
        }
    }

    if(tbTop != BOUNDS_EMPTY && tbLeft != BOUNDS_EMPTY)
    {
        layout->textBounds.x = tbLeft;
        layout->textBounds.y = tbTop;
        layout->textBounds.width = tbRight - tbLeft;
        layout->textBounds.height = tbBottom - tbTop;
    }
}

void g_text_layouter::destroyLayout(g_layouted_text* layout)
{
    if(layout->glyph_buffer)
        free(layout->glyph_buffer);
    if(layout->cluster_buffer)
        free(layout->cluster_buffer);
    delete layout;
}
