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

#include "libwindow/text/font_loader.hpp"
#include "libwindow/text/font_manager.hpp"

g_font* g_font_loader::getFont(std::string path, std::string name)
{
	g_font* existing = g_font_manager::getInstance()->getFont(name);
	if(existing)
		return existing;

	g_font* newFont = g_font::load(path, name);
	if(!newFont)
		return nullptr;

	if(g_font_manager::getInstance()->registerFont(name, newFont))
		return newFont;

	delete newFont;
	return nullptr;
}

g_font* g_font_loader::getSystemFont(std::string name)
{
	return getFont("/system/graphics/fonts/" + name + ".ttf", name);
}

g_font* g_font_loader::get(std::string name)
{
	g_font* font = getSystemFont(name);
	if(font)
		return font;
	return getDefault();
}

g_font* g_font_loader::getDefault()
{
	return getFont("/system/graphics/fonts/default.ttf", "default");
}
