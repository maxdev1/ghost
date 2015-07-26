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
class screen_t {
private:
	uint32_t id;

	uint8_t* output_buffer;
	uint8_t* output_current;

	uint32_t offset;

	uint32_t activeProcessId;
	uint8_t lock;
	void writeChar(char c);
	void normalize();

public:
	screen_t();

	void clean();
	void deactivate();
	void activate();

	void backspace();
	void write(std::string message);
	void updateCursor();
	void moveCursor(uint16_t x, uint16_t y);
};

#endif
