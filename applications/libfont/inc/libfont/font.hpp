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

#ifndef __LIBFONT_TEXT_FONT__
#define __LIBFONT_TEXT_FONT__

#include "libfont/freetype.hpp"

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
    g_font_style style = g_font_style::NORMAL;
    bool hint = true;

    FT_Face face;
    cairo_font_face_t* cairo_face = nullptr;

    static bool readAllBytes(FILE* file, uint32_t offset, uint8_t* buffer, uint32_t len);

  public:
    /**
     * Creates an empty font with the "name". The "source" data
     * is copied to an internal buffer.
     *
     * @param name			font lookup name
     * @param source		font data
     * @param sourceLength	font data length
     */
    g_font(std::string name, uint8_t* source, uint32_t sourceLength);

    /**
     * Loads the font from the given path.
     *
     * @param path the file path
     * @param name the font name
     * @return either the font, or 0
     */
    static g_font* load(std::string path, std::string name);

    /**
     * Destroys the font, deleting all associated Glyph
     * objects and freeing the freetype face.
     */
    ~g_font();

    /**
     * @return whether the font was successfully initialized.
     */
    bool isValid();

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
