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

#ifndef __TERMINAL_DOCUMENT__
#define __TERMINAL_DOCUMENT__

#include "terminal_line.hpp"

#include <ghost.h>

struct terminal_row_t
{
	char* start;
	int length;
};

/**
 * Document of the terminal that manages character insertion and removal. It performs the translation
 * from lines to "rows" by breaking lines based on parameters.
 */
class terminal_document_t
{
	g_user_mutex linesLock = g_mutex_initialize_r(true);
	terminal_line_t* lines = nullptr;
	int cachedColumns = 0;
	int cachedRows = 1;
	static int rowsForLine(const terminal_line_t* line, int columns);
	terminal_line_t* findLine(int offset);
	void insertNewline(terminal_line_t*& current);

public:
	terminal_document_t();
	~terminal_document_t();

	void insert(int x, int offsetY, char c);
	int write(int& x, int offsetY, const char* data, int length);
	// Copy a viewport under one lock; offsetY is the top row, counted from the bottom.
	void copyRows(int offsetY, int columns, int rows, char* target);
	void remove(int x, int offsetY);
	terminal_line_t* getLine(int offset);

	terminal_row_t acquireRow(int offsetY, int columns);
	void releaseRow() const;

	int getRowCount(int columns);
	void clear();
};


#endif
