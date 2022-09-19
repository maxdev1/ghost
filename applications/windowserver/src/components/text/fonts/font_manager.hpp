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

#ifndef __WINDOWSERVER_TEXT_FONTS_FONTMANAGER__
#define __WINDOWSERVER_TEXT_FONTS_FONTMANAGER__

#include "components/text/fonts/font.hpp"
#include "components/text/fonts/freetype.hpp"
#include <map>
#include <string>

class g_font_manager
{
  private:
    FT_Library library;
    std::map<std::string, g_font*> fontRegistry;

    g_font_manager();
    ~g_font_manager();

    /**
     * Initializes a freetype library instance
     */
    void initializeEngine();

    /**
     * Destroys the freetype library instance
     */
    void destroyEngine();

  public:
    /**
     * @return the instance of the font manager singleton
     */
    static g_font_manager* getInstance();

    /**
     * Creates a font with the "name", reading the font data "source".
     * The data within "source" is copied to the {Font} objects buffer.
     *
     * @param name			name to which the font shall be registered
     * @param source		font data
     * @param sourceLength	length of the font data
     * @param style			font style
     * @param hint			whether to hint the font
     */
    bool createFont(std::string name, uint8_t* source, uint32_t sourceLength, g_font_style style = g_font_style::NORMAL, bool hint = true);

    /**
     * Looks for an existing font with the "name".
     *
     * @param name	the name to which the font is registered
     */
    g_font* getFont(std::string name);

    /**
     * Deletes the font and removes it from the font registry.
     *
     * @param font	the font to destroy
     */
    void destroyFont(g_font* font);

    /**
     * @return the freetype library handle
     */
    FT_Library getLibraryHandle();
};

#endif
