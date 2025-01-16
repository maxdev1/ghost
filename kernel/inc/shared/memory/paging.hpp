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

#define G_PAGE_TABLE_PRESENT        (1)
#define G_PAGE_TABLE_READWRITE      (1 << 1)
#define G_PAGE_TABLE_USERSPACE      (1 << 2)
#define G_PAGE_TABLE_WRITETHROUGH   (1 << 3)
#define G_PAGE_TABLE_CACHE_DISABLED (1 << 4)
#define G_PAGE_TABLE_ACCESSED       (1 << 5)
#define G_PAGE_TABLE_SIZE           (1 << 6)

#define G_PAGE_PRESENT              (1)
#define G_PAGE_READWRITE            (1 << 1)
#define G_PAGE_USERSPACE            (1 << 2)
#define G_PAGE_WRITETHROUGH         (1 << 3)
#define G_PAGE_CACHE_DISABLED       (1 << 4)
#define G_PAGE_ACCESSED             (1 << 5)
#define G_PAGE_DIRTY                (1 << 6)
#define G_PAGE_GLOBAL               (1 << 7)

/**
 * Default flag definitions
 */
#define G_PAGE_TABLE_KERNEL_DEFAULT (G_PAGE_TABLE_PRESENT | G_PAGE_TABLE_READWRITE)
#define G_PAGE_TABLE_USER_DEFAULT   (G_PAGE_TABLE_PRESENT | G_PAGE_TABLE_READWRITE | G_PAGE_TABLE_USERSPACE)

#define G_PAGE_KERNEL_DEFAULT       (G_PAGE_PRESENT | G_PAGE_READWRITE | G_PAGE_GLOBAL)
#define G_PAGE_KERNEL_UNCACHED      (G_PAGE_KERNEL_DEFAULT | G_PAGE_CACHE_DISABLED)
#define G_PAGE_USER_DEFAULT         (G_PAGE_PRESENT | G_PAGE_READWRITE | G_PAGE_USERSPACE)

/**
 * Type definitions for pointers to a directory or table
 */
typedef volatile uint32_t* g_page_directory;
typedef volatile uint32_t* g_page_table;

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
                   uint32_t tableFlags = G_PAGE_TABLE_KERNEL_DEFAULT,
                   uint32_t pageFlags = G_PAGE_KERNEL_DEFAULT, bool allowOverride = false);

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

/**
 * Invalidates the translation lookaside buffer (TLB) entries for a given page.
 */
static inline void pagingInvalidatePage(uint32_t addr)
{
    __asm__ __volatile__("invlpg (%0)" : : "r"(addr) : "memory");
}

#endif
