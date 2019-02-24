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

#ifndef __CONSOLE_VIDEO__
#define __CONSOLE_VIDEO__

#include <stdarg.h>
#include "ghost/stdint.h"

#define G_CONSOLE_VIDEO_MEMORY			0xB8000
#define G_CONSOLE_VIDEO_WIDTH			80
#define G_CONSOLE_VIDEO_HEIGHT			25
#define G_CONSOLE_VIDEO_LINE_BYTES		(G_CONSOLE_VIDEO_WIDTH * 2)
#define G_CONSOLE_VIDEO_SCREEN_BYTES	(G_CONSOLE_VIDEO_HEIGHT * G_CONSOLE_VIDEO_LINE_BYTES)

#define G_CONSOLE_VIDEO_DEFAULT_COLOR	0x07
#define G_CONSOLE_VIDEO_HEADER_COLOR	0x0F

void consoleVideoPrint(char c);

void consoleVideoPutChar(uint16_t x, uint16_t y, char c, uint8_t color);

void consoleVideoPutString(uint16_t x, uint16_t y, const char *c, uint8_t color);

void consoleVideoSetColor(uint8_t color);

void consoleVideoClear();

void consoleVideoSetVisualCursor(int x, int y);

void consoleVideoScrollUp();

#endif
