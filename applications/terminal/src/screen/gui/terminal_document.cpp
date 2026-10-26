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
#include <cstring>
#include <climits>
#include <algorithm>

terminal_document_t::terminal_document_t()
{
	lines = new terminal_line_t();
}

terminal_document_t::~terminal_document_t()
{
	while(lines)
	{
		auto previous = lines->previous;
		delete lines;
		lines = previous;
	}
	g_mutex_destroy(linesLock);
}

int terminal_document_t::rowsForLine(const terminal_line_t* line, int columns)
{
	return line->length ? (line->length - 1) / columns + 1 : 1;
}

terminal_line_t* terminal_document_t::findLine(int offset)
{
	if(offset < 0)
		return nullptr;
	auto current = lines;
	while(offset-- && current)
		current = current->previous;
	return current;
}

void terminal_document_t::insertNewline(terminal_line_t*& current)
{
	auto next = new terminal_line_t();
	next->previous = current;
	next->next = current->next;
	if(current->next)
		current->next->previous = next;
	else
		lines = next;
	current->next = next;
	current = next;
	if(cachedColumns > 0)
		++cachedRows;
}

int terminal_document_t::write(int& x, int offsetY, const char* data, int length)
{
	g_mutex_acquire(linesLock);
	if(x < 0 || length <= 0)
	{
		const int rows = cachedRows;
		g_mutex_release(linesLock);
		return rows;
	}
	auto current = findLine(offsetY);
	for(int offset = 0; current && offset < length;)
	{
		if(data[offset] == '\n')
		{
			insertNewline(current);
			x = 0;
			++offset;
			continue;
		}
		int end = offset;
		while(end < length && data[end] != '\n')
			++end;
		if(end - offset > INT_MAX - x)
			break;
		int oldRows = cachedColumns > 0 ? rowsForLine(current, cachedColumns) : 0;
		current->insert(x, data + offset, end - offset);
		x += end - offset;
		if(cachedColumns > 0)
			cachedRows += rowsForLine(current, cachedColumns) - oldRows;
		offset = end;
	}
	const int rows = cachedRows;
	g_mutex_release(linesLock);
	return rows;
}

void terminal_document_t::insert(int x, int offsetY, char c)
{
	write(x, offsetY, &c, 1);
}

void terminal_document_t::remove(int x, int offsetY)
{
	g_mutex_acquire(linesLock);
	auto current = findLine(offsetY);
	if(current)
	{
		int oldRows = cachedColumns > 0 ? rowsForLine(current, cachedColumns) : 0;
		current->remove(x);
		if(cachedColumns > 0)
			cachedRows += rowsForLine(current, cachedColumns) - oldRows;
	}
	g_mutex_release(linesLock);
}

terminal_line_t* terminal_document_t::getLine(int offset)
{
	g_mutex_acquire(linesLock);
	auto result = findLine(offset);
	g_mutex_release(linesLock);
	return result;
}

terminal_row_t terminal_document_t::acquireRow(int offsetY, int columns)
{
	g_mutex_acquire(linesLock);
	if(offsetY < 0 || columns <= 0)
		return {nullptr, 0};
	auto line = lines;
	while(line && offsetY >= rowsForLine(line, columns))
	{
		offsetY -= rowsForLine(line, columns);
		line = line->previous;
	}
	if(!line || !line->length)
		return {nullptr, 0};
	const int last = (line->length - 1) % columns + 1;
	const int end = line->length - (offsetY ? last + (offsetY - 1) * columns : 0);
	const int count = offsetY ? columns : last;
	return {line->buffer + end - count, count};
}

void terminal_document_t::releaseRow() const
{
	g_mutex_release(linesLock);
}

void terminal_document_t::copyRows(int offsetY, int columns, int rows, char* target)
{
	if(columns <= 0 || rows <= 0)
		return;
	memset(target, 0, (size_t) columns * rows);
	if(offsetY < 0)
		return;
	g_mutex_acquire(linesLock);
	const int first = std::max(0, offsetY - rows + 1);
	int offset = 0;
	auto line = lines;
	while(line && offset + rowsForLine(line, columns) <= first)
	{
		offset += rowsForLine(line, columns);
		line = line->previous;
	}
	while(line && offset <= offsetY)
	{
		int lineRows = rowsForLine(line, columns);
		const int skip = std::max(0, first - offset);
		const int last = line->length ? (line->length - 1) % columns + 1 : 0;
		int end = line->length - (skip ? last + (skip - 1) * columns : 0);
		offset += skip;
		for(int part = skip; part < lineRows && offset <= offsetY; ++part, ++offset)
		{
			const int count = part ? columns : last;
			if(count)
				memcpy(target + (size_t) (offsetY - offset) * columns, line->buffer + end - count, count);
			end -= count;
		}
		line = line->previous;
	}
	g_mutex_release(linesLock);
}

int terminal_document_t::getRowCount(int columns)
{
	if(columns <= 0)
		return 0;
	g_mutex_acquire(linesLock);
	if(columns != cachedColumns)
	{
		cachedColumns = columns;
		cachedRows = 0;
		for(auto current = lines; current; current = current->previous)
			cachedRows += rowsForLine(current, columns);
	}
	const int result = cachedRows;
	g_mutex_release(linesLock);
	return result;
}

void terminal_document_t::clear()
{
	g_mutex_acquire(linesLock);
	auto old = lines;
	lines = new terminal_line_t();
	cachedRows = 1;
	g_mutex_release(linesLock);
	while(old)
	{
		auto previous = old->previous;
		delete old;
		old = previous;
	}
}
