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

#include "raster.hpp"

#include <string.h>

void raster_t::scrollBy(int y)
{
	g_atomic_lock(lock);

	// TODO optimize
	int len = width * height * sizeof(raster_entry_t);
	while(y--)
	{
		memcpy(buffer, &buffer[width], len - width * sizeof(raster_entry_t));
		for(uint32_t i = 0; i < width; i++)
		{
			buffer[width * height - width + i] = {c : ' '};
		}
	}

	changed.x = 0;
	changed.y = 0;
	changed.width = width;
	changed.height = height;

	g_atomic_unlock(lock);
}

bool raster_t::resizeTo(int newWidth, int newHeight)
{
	g_atomic_lock(lock);

	if(newWidth == width && newHeight == height)
	{
		g_atomic_unlock(lock);
		return false;
	}

	raster_entry_t* oldBuffer = buffer;
	int oldWidth = width;
	int oldHeight = height;

	buffer = new raster_entry_t[newWidth * newHeight];
	width = newWidth;
	height = newHeight;
	memset(buffer, 0, width * height * sizeof(raster_entry_t));

	if(oldBuffer)
	{
		for(int y = 0; y < (height < oldHeight ? height : oldHeight); y++)
		{
			for(int x = 0; x < (width < oldWidth ? width : oldWidth); x++)
			{
				buffer[y * width + x] = oldBuffer[y * oldWidth + x];
			}
		}

		delete oldBuffer;
	}

	changed.x = 0;
	changed.y = 0;
	changed.width = width;
	changed.height = height;

	g_atomic_unlock(lock);
	return true;
}

raster_entry_t raster_t::getUnlocked(int x, int y)
{
	if(y >= height)
		y = height - 1;
	if(y < 0)
		y = 0;
	if(x >= width)
		x = width - 1;
	if(x < 0)
		x = 0;

	return buffer[y * width + x];
}

void raster_t::lockBuffer()
{
	g_atomic_lock(lock);
}

void raster_t::unlockBuffer()
{
	g_atomic_unlock(lock);
}

void raster_t::clean()
{
	g_atomic_lock(lock);

	memset(buffer, 0, height * width * sizeof(raster_entry_t));

	changed.x = 0;
	changed.y = 0;
	changed.width = width;
	changed.height = height;

	g_atomic_unlock(lock);
}

void raster_t::dirty(int x, int y)
{
	g_atomic_lock(lock);

	if(changed.width == 0)
	{
		changed.x = x;
		changed.y = y;
		changed.width = 1;
		changed.height = 1;
	}
	else
	{
		if(x < changed.x)
		{
			changed.width += changed.x - x;
			changed.x = x;
		}
		else if(x + 1 > changed.x + changed.width)
		{
			changed.width = (x + 1) - changed.x;
		}
		if(y < changed.y)
		{
			changed.height += changed.y - y;
			changed.y = y;
		}
		else if(y + 1 > changed.y + changed.height)
		{
			changed.height = (y + 1) - changed.y;
		}
	}

	g_atomic_unlock(lock);
}

void raster_t::put(int x, int y, uint8_t c, uint8_t foreground, uint8_t background)
{
	g_atomic_lock(lock);

	if(y >= height)
		y = height - 1;
	if(y < 0)
		y = 0;
	if(x >= width)
		x = width - 1;
	if(x < 0)
		x = 0;

	buffer[y * width + x] = {
		c : c,
		foreground : foreground,
		background : background
	};

	g_atomic_unlock(lock);

	dirty(x, y);
}

g_rectangle raster_t::popChanges()
{
	g_rectangle changes = changed;
	changed = g_rectangle(0, 0, 0, 0);
	return changes;
}