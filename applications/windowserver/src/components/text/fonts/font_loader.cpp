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

#include "components/text/fonts/font_loader.hpp"

g_font* g_font_loader::getFontAtPath(std::string path, std::string name)
{
    FILE* file = fopen(path.c_str(), "r");
    if(file != NULL)
    {
        g_font* font = g_font::fromFile(file, name);
        fclose(file);
        return font;
    }
    return 0;
}

g_font* g_font_loader::getSystemFont(std::string name)
{
    return getFontAtPath("/system/graphics/fonts/" + name + ".ttf", name);
}

g_font* g_font_loader::get(std::string name)
{
    g_font* font = getSystemFont(name);

    if(font == 0)
    {
        font = getDefault();
    }

    return font;
}

g_font* g_font_loader::getDefault()
{
    return getFontAtPath("/system/graphics/fonts/default.ttf", "default");
}
