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

#ifndef __GHOST_DEBUG_INTERFACE_KERNEL__
#define __GHOST_DEBUG_INTERFACE_KERNEL__

#include "ghost/stdint.h"
#include "debug/debug_interface.hpp"
#include "filesystem/filesystem.hpp"

#define G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED						if (!g_debug_interface_initialized) return;

// shortcuts
#if G_DEBUG_INTERFACE_MODE == G_DEBUG_INTERFACE_MODE_FULL
#define G_DEBUG_INTERFACE_FILESYSTEM_UPDATE_NODE(node)				g_debug_interface_kernel::filesystemUpdateNode(node);
#define G_DEBUG_INTERFACE_MEMORY_SET_PAGE_USAGE(pageaddr, usage)	g_debug_interface_kernel::memorySetPageUsage(pageaddr, usage);
#define G_DEBUG_INTERFACE_TASK_SET_IDENTIFIER(task, identifier)		g_debug_interface_kernel::taskSetIdentifier(task, identifier);
#define G_DEBUG_INTERFACE_TASK_SET_STATUS(task, status)				g_debug_interface_kernel::publishTaskStatus(task, status);
#define G_DEBUG_INTERFACE_TASK_SET_ROUNDS(task, rounds)				g_debug_interface_kernel::publishTaskRounds(task, rounds);
#define G_DEBUG_INTERFACE_TASK_SET_SOURCE_PATH(task, source_path)	g_debug_interface_kernel::taskSetSourcePath(task, source_path);
#define G_DEBUG_INTERFACE_SYSTEM_INFORMATION(key, value)			g_debug_interface_kernel::systemInformation(key, value);

#else
#define G_DEBUG_INTERFACE_FILESYSTEM_UPDATE_NODE(node)
#define G_DEBUG_INTERFACE_MEMORY_SET_PAGE_USAGE(pageaddr, usage)
#define G_DEBUG_INTERFACE_TASK_SET_IDENTIFIER(task, identifier)
#define G_DEBUG_INTERFACE_TASK_SET_STATUS(task, status)
#define G_DEBUG_INTERFACE_TASK_SET_SOURCE_PATH(task, source_path)
#define G_DEBUG_INTERFACE_SYSTEM_INFORMATION(key, value)

#endif

/**
 *
 */
class g_debug_interface_kernel {
public:
	/**
	 *
	 */
	static void publishTaskStatus(int tid, const char* status);
	/**
	 *
	 */
	static void publishTaskRounds(int tid, long rounds);

	/**
	 *
	 */
	static void taskSetIdentifier(int tid, const char* identifier);

	/**
	 *
	 */
	static void taskSetSourcePath(int tid, const char* source_path);

	/**
	 *
	 */
	static void filesystemUpdateNode(g_fs_node* node);

	/**
	 *
	 */
	static void memorySetPageUsage(uint32_t addr, int usage);

	/**
	 *
	 */
	static void systemInformation(const char* key, uint64_t value);

};

#endif
