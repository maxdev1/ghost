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

#include "libwindow/text/font.hpp"
#include "libwindow/text/font_manager.hpp"
#include <string.h>

g_font::g_font(std::string name, uint8_t* source, uint32_t sourceLength) : name(name)
{
	data = new uint8_t[sourceLength];
	if(!data)
	{
		klog("failed to allocate memory buffer for font '%s'", name);
		return;
	}
	memcpy(data, source, sourceLength);

	FT_Error error = FT_New_Memory_Face(g_font_manager::getInstance()->getLibraryHandle(), data, sourceLength, 0, &face);
	if(error)
	{
		klog("freetype2 failed at FT_New_Memory_Face with error code %i", error);
		delete data;
		return;
	}

	cairo_face = cairo_ft_font_face_create_for_ft_face(face, 0);
	cairo_status_t status = cairo_font_face_status(cairo_face);
	if(status != CAIRO_STATUS_SUCCESS)
	{
        FT_Done_Face(face);
		klog("cairo failed at cairo_ft_font_face_create_for_ft_face with error %i", status);
		cairo_face = 0;
		delete data;
		return;
	}
}

g_font::~g_font()
{
	if(cairo_face)
	{
		cairo_font_face_destroy(cairo_face);
		FT_Done_Face(face);
		delete data;
	}
}

bool g_font::readAllBytes(FILE* file, uint32_t offset, uint8_t* buffer, uint32_t len)
{
	uint32_t remain = len;
	fseek(file, offset, SEEK_SET);

	while(remain)
	{
		size_t read = fread(&buffer[len - remain], 1, remain, file);
		if(read <= 0)
			return false;
		remain -= read;
	}

	return true;
}

g_font* g_font::load(std::string path, std::string name)
{
	FILE* file = fopen(path.c_str(), "r");
	if(!file)
		return nullptr;

	int64_t length = g_length(fileno(file));
	if(length == -1)
	{
		fclose(file);
		return nullptr;
	}

	uint8_t* content = new uint8_t[length];
	if(!readAllBytes(file, 0, content, length))
	{
		fclose(file);
		delete content;
		return nullptr;
	}
	fclose(file);

	g_font* font = new g_font(name, content, length);
	if(!font->isValid())
	{
		delete font;
		return nullptr;
	}
	return font;
}

bool g_font::isValid()
{
	return cairo_face != nullptr;
}
