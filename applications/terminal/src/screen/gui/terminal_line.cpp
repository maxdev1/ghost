/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2024, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include <cstring>
#include "terminal_line.hpp"

terminal_line_t::~terminal_line_t()
{
	if (buffer) delete[] buffer;
}

void terminal_line_t::insert(int position, char c)
{
	if (position >= length)
	{
		// TODO improve performance by keeping wider buffer
		char *new_buffer = new char[position + 1];
		memcpy(new_buffer, buffer, length);
		delete[] buffer;
		memset(&new_buffer[length], 0, position - length);
		buffer = new_buffer;
		length = position + 1;
	}

	buffer[position] = c;
}

void terminal_line_t::remove(int position)
{
	if (length > 0)
	{
		memcpy(&buffer[position], &buffer[position + 1], length - position - 1);
		length--;
	}
}
