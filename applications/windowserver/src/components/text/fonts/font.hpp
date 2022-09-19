/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __WINDOWSERVER_TEXT_FONTS_FONT__
#define __WINDOWSERVER_TEXT_FONTS_FONT__

#include "components/text/fonts/freetype.hpp"

#include <cairo/cairo-ft.h>
#include <cairo/cairo.h>
#include <map>
#include <string>

enum class g_font_style : uint8_t
{
    NORMAL,
    ITALIC,
    BOLD
};

class g_font
{
  private:
    uint8_t* data;
    std::string name;
    g_font_style style;
    bool hint;

    FT_Face face;
    cairo_font_face_t* cairo_face;

    bool okay;

    int activeSize;

    static bool tryReadBytes(FILE* file, uint32_t offset, uint8_t* buffer, uint32_t len);

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
    std::string getName()
    {
        return name;
    }

    /**
     *
     */
    cairo_font_face_t* getFace()
    {
        return cairo_face;
    }
};

#endif
