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

#ifndef __KERNEL_DEBUG_INTERFACE__
#define __KERNEL_DEBUG_INTERFACE__

#include "ghost/stdint.h"
#include "shared/debug/debug_interface.hpp"

#if G_DEBUG_INTERFACE_MODE == G_DEBUG_INTERFACE_MODE_FULL

#define G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED						if (!debugInterfaceInitialized) return;

#define G_DEBUG_INTERFACE_FILESYSTEM_UPDATE_NODE(node)				debugInterfaceFilesystemUpdateNode(node);
#define G_DEBUG_INTERFACE_MEMORY_SET_PAGE_USAGE(pageaddr, usage)	debugInterfaceMemorySetPageUsage(pageaddr, usage);
#define G_DEBUG_INTERFACE_TASK_SET_IDENTIFIER(task, identifier)		debugInterfaceTaskSetIdentifier(task, identifier);
#define G_DEBUG_INTERFACE_TASK_SET_STATUS(task, status)				debugInterfacePublishTaskStatus(task, status);
#define G_DEBUG_INTERFACE_TASK_SET_ROUNDS(task, rounds)				debugInterfacePublishTaskRounds(task, rounds);
#define G_DEBUG_INTERFACE_TASK_SET_SOURCE_PATH(task, source_path)	debugInterfaceTaskSetSourcePath(task, source_path);
#define G_DEBUG_INTERFACE_SYSTEM_INFORMATION(key, value)			debugInterfaceSystemInformation(key, value);

void debugInterfacePublishTaskStatus(int tid, const char* status);

void debugInterfacePublishTaskRounds(int tid, long rounds);

void debugInterfaceTaskSetIdentifier(int tid, const char* identifier);

void debugInterfaceTaskSetSourcePath(int tid, const char* source_path);

void debugInterfaceFilesystemUpdateNode(int id, int physFsId, int parent, char* name);

void debugInterfaceMemorySetPageUsage(uint32_t addr, int usage);

void debugInterfaceSystemInformation(const char* key, uint64_t value);

#else
#define G_DEBUG_INTERFACE_FILESYSTEM_UPDATE_NODE(node)
#define G_DEBUG_INTERFACE_MEMORY_SET_PAGE_USAGE(pageaddr, usage)
#define G_DEBUG_INTERFACE_TASK_SET_IDENTIFIER(task, identifier)
#define G_DEBUG_INTERFACE_TASK_SET_STATUS(task, status)
#define G_DEBUG_INTERFACE_TASK_SET_SOURCE_PATH(task, source_path)
#define G_DEBUG_INTERFACE_SYSTEM_INFORMATION(key, value)
#endif

#endif
