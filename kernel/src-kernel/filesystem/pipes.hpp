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

#ifndef GHOST_FILESYSTEM_PIPES
#define GHOST_FILESYSTEM_PIPES

#include "ghost/stdint.h"
#include "ghost/kernel.h"
#include "utils/list_entry.hpp"
#include "tasking/process.hpp"
#include "filesystem/pipes.hpp"

/**
 *
 */
typedef int g_pipe_id;

/**
 *
 */
struct g_pipe {
	uint8_t* buffer;
	uint8_t* write;
	uint8_t* read;
	uint32_t size;
	uint32_t capacity;

	g_list_entry<g_pid>* references;
};

/**
 *
 */
class g_pipes {
public:

	/**
	 *
	 */
	static void initialize();

	/**
	 *
	 */
	static g_pipe* get(g_pipe_id id);

	/**
	 *
	 */
	static g_pipe_id create();

	/**
	 *
	 */
	static void add_reference(g_pipe_id pipe, g_pid pid);

	/**
	 *
	 */
	static void remove_reference(g_pipe_id pipe, g_pid pid);

	/**
	 *
	 */
	static bool has_reference_from_other_process(g_pipe* pipe, g_pid);

};

#endif
