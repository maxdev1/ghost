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

#include "terminal_document.hpp"

terminal_document::terminal_document()
{
	this->line = new terminal_line();
}

void terminal_document::insert(int x, int offsetY, char c)
{
	auto current = this->line;
	while (offsetY-- && current)
	{
		current = current->previous;
	}

	if (current)
	{
		if (c == '\n')
		{
			auto new_line = new terminal_line();

			new_line->previous = current;
			if (current->next)
			{
				current->next->previous = new_line;
				new_line->next = current->next;
			} else
			{
				this->line = new_line;
			}
			current->next = new_line;
		} else
		{
			current->insert(x, c);
		}
	}
}

void terminal_document::remove(int x, int offsetY)
{
	auto current = this->line;
	while (offsetY-- && current)
	{
		current = current->previous;
	}

	if (current)
	{
		current->remove(x);
	}
}


terminal_line *terminal_document::get_line(int offset)
{
	auto current = this->line;
	while (offset-- && current)
	{
		current = current->previous;
	}
	return current;
}

/**
 * Translates lines in the document to rows in the screen. Imagine the
 * below structure during calculation, the remainder being used to
 * calculate the row span.

    [row] 	[10 cols ]		[rem]
	5		3333333			7
	4		22222			5
	3		1111111111		10
	2		1111111111		20
	1		1111			25
	0		000				3

 */
terminal_row terminal_document::get_row(int offsetY, int columns)
{
	auto line = this->line;
	int rem = line->length;

	while (offsetY-- && line)
	{
		if (rem == 0)
		{
			rem = line->length;
		}

		if (rem > columns)
		{
			if (rem % columns == 0)
			{
				rem -= columns;
			} else
			{
				rem -= rem % columns;
			}
		} else
		{
			line = line->previous;
			if (line) rem = line->length;
		}
	}

	if (!line)
		return {
			nullptr,
			0
		};

	int start = 0;
	int len = 0;
	if (rem > 0)
	{
		if (rem % columns == 0)
		{
			start = rem - columns;
			len = columns;
		} else
		{
			start = rem - rem % columns;
			len = rem % columns;
		}
	}
	return {
		&line->buffer[start],
		len
	};
}


int terminal_document::get_total_rows(int columns)
{
	if (columns == 0) return 0;

	int rows = 0;
	auto current = this->line;
	while (current)
	{
		rows += current->length / columns + (current->length != 0 && current->length % columns == 0 ? 0 : 1);
		current = current->previous;
	}
	return rows;
}

void terminal_document::clear()
{
	auto old = this->line;
	this->line = new terminal_line();

	auto current = old;
	while (current)
	{
		auto prev = current->previous;
		delete current;
		current = prev;
	}
}
