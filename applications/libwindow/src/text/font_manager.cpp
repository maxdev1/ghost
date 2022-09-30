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

#include "libwindow/text/font_manager.hpp"

static g_font_manager* instance = 0;

g_font_manager* g_font_manager::getInstance()
{
	if(instance == 0)
	{
		instance = new g_font_manager();
	}
	return instance;
}

g_font_manager::g_font_manager()
{
	initializeEngine();
}

g_font_manager::~g_font_manager()
{
	destroyEngine();
}

void g_font_manager::initializeEngine()
{
	FT_Error error = FT_Init_FreeType(&library);
	if(error)
		klog("freetype2 failed at FT_Init_FreeType with error code %i", error);
}

void g_font_manager::destroyEngine()
{
	FT_Error error = FT_Done_Library(library);
	if(error)
		klog("freetype2 failed at FT_Done_Library with error code %i", error);
}

g_font* g_font_manager::getFont(std::string name)
{
	if(fontRegistry.count(name) > 0)
		return fontRegistry[name];
	return nullptr;
}

bool g_font_manager::registerFont(std::string name, g_font* font)
{
	if(fontRegistry.count(name) > 0)
	{
		klog("tried to create font '%s' that already exists", name.c_str());
		return false;
	}

	fontRegistry[name] = font;
	return true;
}

void g_font_manager::destroyFont(g_font* font)
{
	fontRegistry.erase(font->getName());
	delete font;
}

FT_Library g_font_manager::getLibraryHandle()
{
	return library;
}
