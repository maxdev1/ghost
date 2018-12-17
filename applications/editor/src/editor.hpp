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

#ifndef __GHOST_EDITOR__
#define __GHOST_EDITOR__

#include <string>
#include <ghostuser/graphics/metrics/point.hpp>
#include <ghostuser/io/terminal.hpp>

void load_file();
void extend_buffer();
void display_current_lines();
void quit_with_error(std::string message);
void insert_into_buffer(char* c, uint32_t len);
void move_caret(int x, int y);
void do_backspace();
uint8_t* find_line(int line);

int get_line_visual_length(int line);
g_term_cursor_position to_visual(int line, int col);

#endif
