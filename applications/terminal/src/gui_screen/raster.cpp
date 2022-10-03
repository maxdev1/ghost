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
	int len = width * height;
	while(y--)
	{
		memcpy(buffer, &buffer[width], len - width);
		for(uint32_t i = 0; i < width; i++)
		{
			buffer[len - width + i] = ' ';
		}
	}

	g_atomic_unlock(lock);
}

void raster_t::resizeTo(int newWidth, int newHeight)
{
	g_atomic_lock(lock);

	uint8_t* oldBuffer = buffer;
	int oldWidth = width;
	int oldHeight = height;

	buffer = new uint8_t[newWidth * newHeight];
	width = newWidth;
	height = newHeight;
	memset(buffer, 0, width * height);

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

	g_atomic_unlock(lock);
}

uint8_t raster_t::getUnlocked(int x, int y)
{
	uint8_t val;

	if(y >= height)
		y = height - 1;
	if(y < 0)
		y = 0;
	if(x >= width)
		x = width - 1;
	if(x < 0)
		x = 0;

	val = buffer[y * width + x];

	return val;
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
	memset(buffer, 0, height * width);
	g_atomic_unlock(lock);
}

void raster_t::put(int x, int y, uint8_t c)
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

	buffer[y * width + x] = c;

	g_atomic_unlock(lock);
}