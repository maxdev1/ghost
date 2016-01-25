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

#ifndef __GHOST_STANDARD_IO__
#define __GHOST_STANDARD_IO__

#include <ghost.h>
#include <stdint.h>

class terminal_t;
class screen_t;

/**
 *
 */
struct standard_out_thread_data_t {
	bool stop;
	bool err;
	g_fd stdout_read_end;
	screen_t* screen;
};

/**
 *
 */
struct standard_in_thread_data_t {
	bool continue_input;
	g_fd stdin_write_end;
	g_pid int_pid = -1; // process to interrupt on Ctrl+C
	terminal_t* terminal;
};

#endif
