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

terminal_document_t::terminal_document_t()
{
	this->lines = new terminal_line_t();
}

void terminal_document_t::insert(int x, int offsetY, char c)
{
	g_atomic_lock(this->linesLock);

	auto current = this->lines;
	while(offsetY-- && current)
	{
		current = current->previous;
	}

	if(current)
	{
		if(c == '\n')
		{
			auto new_line = new terminal_line_t();

			new_line->previous = current;
			if(current->next)
			{
				current->next->previous = new_line;
				new_line->next = current->next;
			}
			else
			{
				this->lines = new_line;
			}
			current->next = new_line;
		}
		else
		{
			current->insert(x, c);
		}
	}

	g_atomic_unlock(this->linesLock);
}

void terminal_document_t::remove(int x, int offsetY)
{
	g_atomic_lock(this->linesLock);

	auto current = this->lines;
	while(offsetY-- && current)
	{
		current = current->previous;
	}

	if(current)
	{
		current->remove(x);
	}

	g_atomic_unlock(this->linesLock);
}


terminal_line_t* terminal_document_t::getLine(int offset)
{
	g_atomic_lock(this->linesLock);

	auto current = this->lines;
	while(offset-- && current)
	{
		current = current->previous;
	}

	g_atomic_unlock(this->linesLock);
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

 * Keeps the document locked until it is explicitly unlocked.
 */
terminal_row_t terminal_document_t::acquireRow(int offsetY, int columns)
{
	g_atomic_lock(this->linesLock);

	auto line = this->lines;
	int rem = line->length;

	while(offsetY-- && line)
	{
		if(rem == 0)
		{
			rem = line->length;
		}

		if(rem > columns)
		{
			if(rem % columns == 0)
			{
				rem -= columns;
			}
			else
			{
				rem -= rem % columns;
			}
		}
		else
		{
			line = line->previous;
			if(line)
				rem = line->length;
		}
	}

	if(!line)
	{
		return {
				nullptr,
				0
		};
	}

	int start = 0;
	int len = 0;
	if(rem > 0)
	{
		if(rem % columns == 0)
		{
			start = rem - columns;
			len = columns;
		}
		else
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

void terminal_document_t::releaseRow() const
{
	g_atomic_unlock(this->linesLock);
}

int terminal_document_t::getRowCount(int columns)
{
	if(columns == 0)
		return 0;

	int rows = 0;
	g_atomic_lock(this->linesLock);
	auto current = this->lines;
	while(current)
	{
		rows += current->length / columns + (current->length != 0 && current->length % columns == 0 ? 0 : 1);
		current = current->previous;
	}
	g_atomic_unlock(this->linesLock);
	return rows;
}

void terminal_document_t::clear()
{
	g_atomic_lock(this->linesLock);
	auto old = this->lines;
	this->lines = new terminal_line_t();
	g_atomic_unlock(this->linesLock);

	auto current = old;
	while(current)
	{
		auto prev = current->previous;
		delete current;
		current = prev;
	}
}
