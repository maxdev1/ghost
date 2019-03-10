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
#include "shared/memory/memory.hpp"
#include "shared/system/io_port.hpp"

static uint8_t* videoMemory = (uint8_t*) G_CONSOLE_VIDEO_MEMORY;
static uint8_t color = G_CONSOLE_VIDEO_DEFAULT_COLOR;
static uint32_t offset = 0;

void consoleVideoPrint(char c)
{
	if(c == '\n')
	{
		if(offset % (G_CONSOLE_VIDEO_WIDTH * 2) == 0)
			consoleVideoPrint(' ');

		while(offset % (G_CONSOLE_VIDEO_WIDTH * 2) != 0)
			consoleVideoPrint(' ');

	} else
	{
		videoMemory[offset++] = c;
		videoMemory[offset++] = color;

		if(offset >= (G_CONSOLE_VIDEO_WIDTH * 2 * G_CONSOLE_VIDEO_HEIGHT))
		{
			consoleVideoScrollUp();
		}
	}
}

void consoleVideoPutChar(uint16_t x, uint16_t y, char c, uint8_t color)
{
	videoMemory[y * (G_CONSOLE_VIDEO_WIDTH * 2) + x * 2] = c;
	videoMemory[y * (G_CONSOLE_VIDEO_WIDTH * 2) + x * 2 + 1] = color;
}

void consoleVideoPutString(uint16_t x, uint16_t y, const char* c, uint8_t color)
{
	while(*c)
	{
		consoleVideoPutChar(x++, y, *c, color);
		if(x > G_CONSOLE_VIDEO_WIDTH)
		{
			x = 0;
			y++;
		}
		if(y > G_CONSOLE_VIDEO_HEIGHT)
		{
			y = 0;
		}
		++c;
	}
}

void consoleVideoScrollUp()
{
	uint32_t screenBytesWithoutLastLine = G_CONSOLE_VIDEO_SCREEN_BYTES - G_CONSOLE_VIDEO_LINE_BYTES;
	memoryCopy(videoMemory, videoMemory + G_CONSOLE_VIDEO_LINE_BYTES, screenBytesWithoutLastLine);
	offset -= G_CONSOLE_VIDEO_LINE_BYTES;
	memorySetWords(videoMemory + screenBytesWithoutLastLine, ((G_CONSOLE_VIDEO_DEFAULT_COLOR << 8) | ' '), G_CONSOLE_VIDEO_WIDTH);
}

void consoleVideoClear()
{
	for(uint32_t i = 0; i < 25; i++)
	{
		consoleVideoPrint('\n');
	}
	offset = 0;
}

void consoleVideoSetColor(uint8_t newColor)
{
	color = newColor;
}

void consoleVideoSetVisualCursor(int x, int y)
{

	uint16_t position = (y * G_CONSOLE_VIDEO_WIDTH) + x;
	ioPortWriteByte(0x3D4, 0x0F);
	ioPortWriteByte(0x3D5, (uint8_t) (position & 0xFF));
	ioPortWriteByte(0x3D4, 0x0E);
	ioPortWriteByte(0x3D5, (uint8_t) ((position >> 8) & 0xFF));
}
