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

#include "video/graphics.hpp"
#include <malloc.h>
#include <string.h>

#include <stdio.h>

g_graphics::g_graphics(uint16_t width, uint16_t height) : width(width), height(height)
{
	resize(width, height);
}

void g_graphics::resize(int newWidth, int newHeight, bool averaged, bool force)
{
	if(newWidth <= 0 || newHeight <= 0)
		return;

	if(averaged)
	{
		newWidth = newWidth + (averageFactor - newWidth % averageFactor);
		newHeight = newHeight + (averageFactor - newHeight % averageFactor);
	}

	// TODO: Like this, buffers never downscale. Check if we want this:
	if(newWidth <= width && newHeight <= height && !force)
		return;

	if(surface)
		cairo_surface_destroy(surface);
	if(context)
		cairo_destroy(context);

	width = newWidth;
	height = newHeight;
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	context = cairo_create(surface);
}

void g_graphics::blitTo(g_graphics* graphics, g_rectangle absoluteClip, g_point position)
{
	auto cr = graphics->context;
	cairo_save(cr);
	cairo_set_source_surface(cr, this->surface, position.x, position.y);
	cairo_rectangle(cr, absoluteClip.x, absoluteClip.y, absoluteClip.width, absoluteClip.height);
	cairo_clip(cr);
	cairo_paint(cr);
	cairo_restore(cr);
}
