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

#include "shared/video/console_video.hpp"
#include "shared/logger/logger.hpp"
#include "shared/video/bitmap_font.hpp"

static volatile uint32_t* videoMemory = nullptr;
static int videoWidth = 0;
static int videoHeight = 0;
static uint64_t videoPitch = 0;

static uint32_t color = G_CONSOLE_VIDEO_DEFAULT_COLOR;
static uint32_t offset = 0;

void consoleVideoInitialize(limine_framebuffer* framebuffer)
{
	videoMemory = static_cast<volatile uint32_t*>(framebuffer->address);
	videoWidth = framebuffer->width;
	videoHeight = framebuffer->height;
	videoPitch = framebuffer->pitch / (framebuffer->bpp / 8);
}

void consoleVideoPrint(char c)
{
	int charColumns = videoWidth / bitmapFontCharWidth;
	int charRows = videoHeight / bitmapFontCharHeight;

	if(c == '\n')
	{
		offset += charColumns - (offset % charColumns);
	}
	else
	{
		if(offset >= charColumns * charRows)
			consoleVideoScrollUp();

		int x = offset % charColumns;
		int y = (offset / charColumns) % charRows;

		consoleVideoPutChar(x, y, c, color);

		offset++;
	}
}

void consoleVideoPutChar(uint16_t x, uint16_t y, char c, uint32_t color)
{
	uint8_t* fontChar = bitmapFontGetChar(c);

	int onScreenX = x * bitmapFontCharWidth;
	if(onScreenX > videoWidth - bitmapFontCharWidth)
		return;

	int onScreenY = y * bitmapFontCharHeight;;
	if(onScreenY > videoHeight - bitmapFontCharHeight)
		return;

	for(int cy = 0; cy < bitmapFontCharHeight; cy++)
	{
		for(int cx = 0; cx < bitmapFontCharWidth; cx++)
		{
			videoMemory[(onScreenY + cy) * videoPitch + (onScreenX + cx)] =
					(fontChar && fontChar[cy * bitmapFontCharWidth + cx] > 0) ? color : 0;
		}
	}
}

void consoleVideoPutString(uint16_t x, uint16_t y, const char* c, uint32_t color)
{
	int charColumns = videoWidth / bitmapFontCharWidth;
	int charRows = videoHeight / bitmapFontCharHeight;
	while(*c)
	{
		consoleVideoPutChar(x++, y, *c, color);
		if(x > charColumns)
		{
			x = 0;
			y++;
		}
		if(y > charRows)
		{
			y = 0;
		}
		++c;
	}
}

void consoleVideoScrollUp()
{
	for(int y = 0; y < videoHeight; y++)
	{
		for(int x = 0; x < videoWidth; x++)
		{
			videoMemory[y * videoPitch + x] = 0;
		}
	}
	offset = 0;

	// TODO this is quite slow:
	// int charColumns = videoWidth / bitmapFontCharWidth;
	// int charRows = videoHeight / bitmapFontCharHeight;
	//
	// for(int y = 0; y < (charRows - 1) * bitmapFontCharHeight; y++)
	// {
	// 	for(int x = 0; x < videoWidth; x++)
	// 	{
	// 		videoMemory[y * videoPitch + x] = videoMemory[(y + bitmapFontCharHeight) * videoPitch + x];
	// 	}
	// }
	//
	// for(int y = (charRows - 1) * bitmapFontCharHeight; y < videoHeight; y++)
	// {
	// 	for(int x = 0; x < videoWidth; x++)
	// 	{
	// 		videoMemory[y * videoPitch + x] = 0;
	// 	}
	// }
	//
	// offset -= charColumns;
	// if(offset < 0)
	// 	offset = 0;
}

void consoleVideoClear()
{
	for(int y = 0; y < videoHeight; y++)
	{
		for(int x = 0; x < videoWidth; x++)
		{
			videoMemory[y * videoPitch + x] = 0;
		}
	}
	offset = 0;
}

void consoleVideoSetColor(uint32_t newColor)
{
	color = newColor;
}

uint32_t consoleVideoGetColor()
{
	return color;
}

void consoleVideoSetVisualCursor(int x, int y)
{
	// TODO: Do we need it?
}
