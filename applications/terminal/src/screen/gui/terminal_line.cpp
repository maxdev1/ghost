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
#include <climits>
#include "terminal_line.hpp"

terminal_line_t::~terminal_line_t()
{
	delete[] buffer;
}

void terminal_line_t::insert(int position, char c)
{
	insert(position, &c, 1);
}

void terminal_line_t::insert(int position, const char* data, int count)
{
	if(position < 0 || count <= 0 || count > INT_MAX - position)
		return;
	const int required = position + count;
	if(required > capacity)
	{
		int nextCapacity = capacity ? capacity : 64;
		while(nextCapacity < required)
			nextCapacity = nextCapacity > INT_MAX / 2 ? required : nextCapacity * 2;
		auto next = new char[nextCapacity];
		if(length)
			memcpy(next, buffer, length);
		delete[] buffer;
		buffer = next;
		capacity = nextCapacity;
	}
	if(position > length)
		memset(buffer + length, 0, position - length);
	memcpy(buffer + position, data, count);
	if(required > length)
		length = required;
}

void terminal_line_t::remove(int position)
{
	if(position < 0 || position >= length)
		return;
	memmove(buffer + position, buffer + position + 1, length - position - 1);
	--length;
}
