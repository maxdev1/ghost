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

    void initializeEngine();
    void destroyEngine();

  public:
    /**
     * @return the instance of the font manager singleton
     */
    static g_font_manager* getInstance();

    /**
     * Registers the font.
     *
     * @param name			name to which the font shall be registered
     */
    bool registerFont(std::string name, g_font* font);

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
