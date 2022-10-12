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

#ifndef __PAGING__
#define __PAGING__

#include "ghost/memory.h"
#include "ghost/types.h"

const uint32_t G_PAGE_TABLE_PRESENT = 1;
const uint32_t G_PAGE_TABLE_READWRITE = 2;
const uint32_t G_PAGE_TABLE_USERSPACE = 4;
const uint32_t G_PAGE_TABLE_WRITETHROUGH = 8;
const uint32_t G_PAGE_TABLE_CACHE_DISABLED = 16;
const uint32_t G_PAGE_TABLE_ACCESSED = 32;
const uint32_t G_PAGE_TABLE_SIZE = 64;

const uint32_t G_PAGE_PRESENT = 1;
const uint32_t G_PAGE_READWRITE = 2;
const uint32_t G_PAGE_USERSPACE = 4;
const uint32_t G_PAGE_WRITETHROUGH = 8;
const uint32_t G_PAGE_CACHE_DISABLED = 16;
const uint32_t G_PAGE_ACCESSED = 32;
const uint32_t G_PAGE_DIRTY = 64;
const uint32_t G_PAGE_GLOBAL = 128;

#define DEFAULT_KERNEL_TABLE_FLAGS (G_PAGE_TABLE_PRESENT | G_PAGE_TABLE_READWRITE)
#define DEFAULT_KERNEL_PAGE_FLAGS (G_PAGE_PRESENT | G_PAGE_READWRITE | G_PAGE_GLOBAL)
#define DEFAULT_USER_TABLE_FLAGS (G_PAGE_TABLE_PRESENT | G_PAGE_TABLE_READWRITE | G_PAGE_TABLE_USERSPACE)
#define DEFAULT_USER_PAGE_FLAGS (G_PAGE_PRESENT | G_PAGE_READWRITE | G_PAGE_USERSPACE)

typedef volatile uint32_t* g_page_directory;
typedef volatile uint32_t* g_page_table;

#define G_INVLPG(addr) __asm__ __volatile__("invlpg (%%eax)" \
											:                \
											: "a"(addr));

/**
 * Switches to the given page directory.
 *
 * @param dir
 * 		the directory to switch to
 */
void pagingSwitchToSpace(g_physical_address dir);

/**
 * Maps a page to the current address space.
 *
 * @param virt
 * 		the virtual address to map to
 * @param phys
 * 		the address of the physical page to map
 * @param tableFlags
 * 		the flags to add on the table entry
 * @param pageFlags
 * 		the flags to add on the page entry
 * @param allowOverride
 * 		whether an existing entry may be overriden
 *
 *
 */
bool pagingMapPage(g_virtual_address virt, g_physical_address phys,
				   uint32_t tableFlags = DEFAULT_KERNEL_TABLE_FLAGS, uint32_t pageFlags = DEFAULT_KERNEL_PAGE_FLAGS, bool allowOverride = false);

/**
 * Unmaps the given virtual page in the current address space.
 *
 * @param virt
 * 		the virtual address to unmap
 */
void pagingUnmapPage(g_virtual_address virt);

/**
 * Returns the currently set page directory.
 *
 * @return the page directory
 */
g_physical_address pagingGetCurrentSpace();

#endif
