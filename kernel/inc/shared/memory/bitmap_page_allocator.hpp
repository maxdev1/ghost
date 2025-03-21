/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __BITMAP_PAGE_ALLOCATOR__
#define __BITMAP_PAGE_ALLOCATOR__

#include "shared/memory/paging.hpp"
#include "shared/system/mutex.hpp"
#include <limine.h>

#include <ghost/stdint.h>

#define G_BITMAP_ALLOCATOR_FASTBUFFER_SIZE 128

#define G_BITMAP_ENTRY_TYPE         uint64_t
#define G_BITMAP_BITS_PER_ENTRY     (sizeof(G_BITMAP_ENTRY_TYPE) * 8)

struct g_bitmap_header;

/**
 * Header of each bitmap index page. The bitmap index page is the top-level
 * structure that keeps track of a list of bitmaps.
 */
struct g_bitmap_index_page_header
{
    g_bitmap_index_page_header* next;
    g_bitmap_header* entries[];
}__attribute__((packed));

#define G_BITMAP_INDEX_MAX_ENTRIES     ((G_PAGE_SIZE - offsetof(g_bitmap_index_page_header, entries)) / sizeof(g_bitmap_index_page_header))

/**
 * Header of a single bitmap. The entries are the actual bitmap and each address
 * is calculated by the base plus total bit index multiplied by page size.
 */
struct g_bitmap_header
{
    g_physical_address base;
    g_physical_address end;
    g_mutex lock;
    G_BITMAP_ENTRY_TYPE entries[];
}__attribute__((packed));

#define G_BITMAP_MAX_ENTRIES                ((G_PAGE_SIZE - offsetof(g_bitmap_header, entries)) / sizeof(G_BITMAP_ENTRY_TYPE))
#define G_BITMAP_TOTAL_BITS                 (G_BITMAP_MAX_ENTRIES * 8)

/**
 * Allocator structure
 */
struct g_bitmap_page_allocator
{
    g_mutex lock;
    uint32_t freePageCount;
    g_bitmap_index_page_header* indexPage;

    struct
    {
        g_mutex lock;
        g_physical_address buffer[G_BITMAP_ALLOCATOR_FASTBUFFER_SIZE];
        int size;
    } fastBuffer;
};

/**
 * Initializes the bitmap page allocator.
 */
void bitmapPageAllocatorInitialize(g_bitmap_page_allocator* allocator, limine_memmap_response* memoryMap);

void bitmapPageAllocatorMarkFree(g_bitmap_page_allocator* allocator, g_physical_address address);

g_physical_address bitmapPageAllocatorAllocate(g_bitmap_page_allocator* allocator);


#endif
