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

#ifndef __KERNEL_MEMORY__
#define __KERNEL_MEMORY__

#include "kernel/filesystem/filesystem.hpp"
#include "kernel/memory/address_range_pool.hpp"
#include "kernel/memory/paging.hpp"
#include "shared/memory/memory.hpp"
#include "shared/setup_information.hpp"
#include <ghost/types.h>

class g_task;
class g_process;

extern g_address_range_pool* memoryVirtualRangePool;

void memoryInitialize(g_setup_information* setupInformation);

void memoryUnmapSetupMemory();

/**
 * Allocates a physical memory page.
 */
g_physical_address memoryPhysicalAllocate(bool untracked = false);

/**
 * Frees a physical memory page.
 */
void memoryPhysicalFree(g_physical_address page);

/**
 * Allocates and maps a memory range with the given number of pages.
 */
g_virtual_address memoryAllocateKernelRange(int32_t pages);

/**
 * Frees a memory range allocated with <memoryAllocateKernelRange>.
 */
void memoryFreeKernelRange(g_virtual_address address);

/**
 * Creates an on-demand mapping for a file in memory.
 */
void memoryOnDemandMapFile(g_process* process, g_fd file, g_offset fileOffset, g_address fileStart, g_ptrsize fileSize, g_ptrsize memorySize);

/**
 * Searches for an on-demand mapping containing the given address.
 */
g_memory_file_ondemand* memoryOnDemandFindMapping(g_task* task, g_address address);

/**
 * Handles loading of the on-demand mapped file content.
 */
bool memoryOnDemandHandlePageFault(g_task* task, g_address accessed);

#endif
