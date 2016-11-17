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

#ifndef __TERMINAL_SCREEN_HEADLESS__
#define __TERMINAL_SCREEN_HEADLESS__

#include "screen.hpp"

#define SCREEN_WIDTH	80
#define SCREEN_HEIGHT	25
#define VIDEO_MEMORY	0xB8000

/**
 *
 */
class headless_screen_t: public screen_t {
private:
	uint32_t id;
	uint8_t* output_current;

	uint32_t offset;

	uint32_t activeProcessId;
	g_atom lock = 0;
	void normalize();
	void updateVisualCursor();

public:
	headless_screen_t();

	g_key_info readInput();
	void clean();
	void backspace();
	void writeChar(char c);
	void moveCursor(int x, int y);
	int getCursorX();
	int getCursorY();
};

#endif
