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

#include <video/console_video.hpp>
#include <memory/memory.hpp>

static uint8_t* videoMemory = (uint8_t*) g_console_video::VIDEO_MEMORY;
static uint8_t color = g_console_video::DEFAULT_COLOR;
static uint32_t offset = 0;

/**
 * 
 */
void g_console_video::print(char c) {
	if (c == '\n') {
		if (offset % (WIDTH * 2) == 0) {
			print(' ');
		}
		while (offset % (WIDTH * 2) != 0) {
			print(' ');
		}
	} else {
		videoMemory[offset++] = c;
		videoMemory[offset++] = color;

		if (offset >= (WIDTH * 2 * HEIGHT)) {
			scrollUp();
		}
	}
}

/**
 * 
 */
void g_console_video::scrollUp() {
	uint32_t screenBytesWithoutLastLine = SCREEN_BYTES - LINE_BYTES;

	g_memory::copy(videoMemory, videoMemory + LINE_BYTES, screenBytesWithoutLastLine);
	offset -= LINE_BYTES;
	g_memory::setWords(videoMemory + screenBytesWithoutLastLine, ((DEFAULT_COLOR << 8) | ' '), WIDTH);
}

/**
 * 
 */
void g_console_video::clear() {
	for (uint32_t i = 0; i < 25; i++) {
		print('\n');
	}
	offset = 0;
}

/**
 * 
 */
void g_console_video::setColor(uint8_t newColor) {
	color = newColor;
}
