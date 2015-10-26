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

#ifndef GHOST_MULTITASKING_PROCESS
#define GHOST_MULTITASKING_PROCESS

#include "ghost/types.h"
#include <tasking/thread.hpp>
#include <utils/list_entry.hpp>
#include <memory/collections/address_range_pool.hpp>
#include <system/smp/global_lock.hpp>

/**
 * Constants used as flags on virtual ranges of processes
 */
#define G_PROC_VIRTUAL_RANGE_FLAG_NONE						0
#define G_PROC_VIRTUAL_RANGE_FLAG_PHYSICAL_OWNER			1

/**
 *
 */
struct g_signal_handler {
public:
	uintptr_t handler = 0;
	uintptr_t callback = 0;
	g_tid thread_id = 0;
};

/**
 *
 */
class g_process {
public:
	g_process* parent;
	g_thread* main;

	// Make sure to adjust forking when changing this.
	g_virtual_address imageStart;
	g_virtual_address imageEnd;    // TODO
	g_virtual_address heapStart;   // Maybe use some kind of "SectionList" here
	g_virtual_address heapBreak;
	uint32_t heapPages;

	// Taken from virtual ranges:
	g_virtual_address tls_master_in_proc_location;
	g_virtual_address tls_master_copysize;
	g_virtual_address tls_master_totalsize;
	g_virtual_address tls_master_alignment;

	g_page_directory pageDirectory;
	char* cliArguments;
	char* workingDirectory;
	char* source_path;

	g_address_range_pool virtualRanges;

	g_security_level securityLevel;

	g_signal_handler signal_handlers[SIG_COUNT];

	/**
	 *
	 */
	g_process(g_security_level securityLevel);
	~g_process();

};

#endif
