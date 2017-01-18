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

#ifndef GHOST_SHARED_VIDEO_CONSOLEVIDEO
#define GHOST_SHARED_VIDEO_CONSOLEVIDEO

#include <stdarg.h>
#include "ghost/stdint.h"

/**
 * The ConsoleVideo class provides simple methods for printing messages
 * to the VBE video memory
 */
class g_console_video {
public:
	static const uint32_t VIDEO_MEMORY = 0xB8000;
	static const uint32_t WIDTH = 80;
	static const uint32_t HEIGHT = 25;
	static const uint32_t LINE_BYTES = WIDTH * 2;
	static const uint32_t SCREEN_BYTES = HEIGHT * LINE_BYTES;

	static const uint32_t DEFAULT_COLOR = 0x07;
	static const uint32_t HEADER_COLOR = 0x0F;

	/**
	 * Prints the given character in the current color.
	 *
	 * @param c the character
	 */
	static void print(char c);

	/**
	 *
	 */
	static void putChar(uint16_t x, uint16_t y, char c, uint8_t color);

	/**
	 *
	 */
	static void putString(uint16_t x, uint16_t y, const char* c, uint8_t color);

	/**
	 * Sets the output color.
	 *
	 * @param color the color
	 */
	static void setColor(uint8_t color);

	/**
	 * Clears the screen.
	 */
	static void clear();

	/**
	 *
	 */
	static int getWidth() {
		return WIDTH;
	}

	/**
	 *
	 */
	static int getHeight() {
		return HEIGHT;
	}

	/**
	 *
	 */
	static void setVisualCursor(int x, int y);

private:

	/**
	 * Scrolls the screen content up by one line.
	 */
	static void scrollUp();

};

#endif
