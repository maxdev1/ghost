/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schl√ºssel <lokoxe@gmail.com>                     *
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

#include <stdint.h>
#include <string>

#ifndef SCREEN_HPP_
#define SCREEN_HPP_

#define SCREEN_WIDTH	80
#define SCREEN_HEIGHT	25
#define VIDEO_MEMORY	0xB8000

/**
 *
 */
typedef uint8_t screen_color_t;

#define SC_BLACK		0x0
#define SC_BLUE			0x1
#define SC_GREEN		0x2
#define SC_CYAN			0x3
#define SC_RED			0x4
#define SC_MAGENTA		0x5
#define SC_BROWN		0x6
#define SC_LGRAY		0x7
#define SC_DARKGRAY		0x8
#define SC_LBLUE		0x9
#define SC_LGREEN		0xA
#define SC_LCYAN		0xB
#define SC_LRED			0xC
#define SC_LMAGENTA		0xD
#define SC_YELLOW		0xE
#define SC_WHITE		0xF

#define SC_COLOR(ba, fo)	(fo | (ba << 4))
#define SC_DEFAULT_COLOR	SC_COLOR(SC_BLACK, SC_WHITE)
#define SC_ERROR_COLOR		SC_COLOR(SC_BLACK, SC_RED)

/**
 * OEM-US special characters
 */
#define OEMUS_CHAR_UE	((char) 0x81) /*¸*/

/**
 *
 */
class screen_t {
private:
	uint32_t id;

	uint8_t* output_buffer;
	uint8_t* output_current;

	uint32_t offset;

	uint32_t activeProcessId;
	uint8_t lock;
	void normalize();

public:
	screen_t();

	void clean();
	void deactivate();
	void activate();

	void backspace();
	void write(std::string message, screen_color_t color = SC_DEFAULT_COLOR);
	void writeChar(char c, screen_color_t color = SC_DEFAULT_COLOR);
	void updateCursor();
	void moveCursor(uint16_t x, uint16_t y);
};

#endif
