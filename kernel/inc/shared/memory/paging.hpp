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

#include <ghost/memory/types.h>
#include <ghost/stdint.h>

#define G_PAGE_PRESENT          (1ULL << 0)  // Page is present
#define G_PAGE_WRITABLE_FLAG    (1ULL << 1)  // Page is writable
#define G_PAGE_USER_FLAG        (1ULL << 2)  // Page is accessible from user mode
#define G_PAGE_WRITE_THROUGH    (1ULL << 3)  // Write-through caching
#define G_PAGE_CACHE_DISABLE    (1ULL << 4)  // Disable caching
#define G_PAGE_ACCESSED_FLAG    (1ULL << 5)  // Page has been accessed
#define G_PAGE_DIRTY_FLAG       (1ULL << 6)  // Page has been written to (only for PT entries)
#define G_PAGE_LARGE_PAGE_FLAG  (1ULL << 7)  // Page is a large page (2MB or 1GB)
#define G_PAGE_GLOBAL_FLAG      (1ULL << 8)  // Page is global (only for PT entries)
#define G_PAGE_NX_FLAG          (1ULL << 63) // No-execute flag (if supported)

/**
 * Default flag definitions
 */
#define G_PAGE_TABLE_KERNEL_DEFAULT (G_PAGE_PRESENT | G_PAGE_WRITABLE_FLAG)
#define G_PAGE_TABLE_USER_DEFAULT   (G_PAGE_PRESENT | G_PAGE_WRITABLE_FLAG | G_PAGE_USER_FLAG)

#define G_PAGE_KERNEL_DEFAULT       (G_PAGE_PRESENT | G_PAGE_WRITABLE_FLAG | G_PAGE_GLOBAL_FLAG)
#define G_PAGE_KERNEL_UNCACHED      (G_PAGE_KERNEL_DEFAULT | G_PAGE_CACHE_DISABLE)
#define G_PAGE_USER_DEFAULT         (G_PAGE_PRESENT | G_PAGE_WRITABLE_FLAG | G_PAGE_USER_FLAG)

#define G_PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define G_PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define G_PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define G_PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)
#define G_PML4_VIRT_ADDRESS(pml4, pdpt, pd, pt) \
    ((((uint64_t)(pml4) << 39) | ((uint64_t)(pdpt) << 30) | ((uint64_t)(pd) << 21) | ((uint64_t)(pt) << 12)) | \
    ((((uint64_t)(pml4) & 0x100) ? 0xFFFF000000000000ULL : 0)))

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
                   uint64_t tableFlags, uint64_t ptFlags,
                   bool allowOverride = false);

bool pagingMapPage(g_virtual_address virt, g_physical_address phys,
                   uint64_t pdptFlags, uint64_t pdFlags,
                   uint64_t ptFlags, uint64_t pageFlags,
                   bool allowOverride = false);

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
static inline void pagingInvalidatePage(g_address addr)
{
    __asm__ __volatile__("invlpg (%0)" : : "r"(addr) : "memory");
}

#endif
