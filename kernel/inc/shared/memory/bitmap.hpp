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

#ifndef __BITMAP__
#define __BITMAP__

#include "shared/memory/paging.hpp"
#include "shared/system/mutex.hpp"

/**
 * When creating the bitmap array, this controls how large the maximum physical
 * memory area can be that a single bitmap maintains.
 */
#define G_BITMAP_MAX_RANGE	0x05000000

/**
 * One bitmap entry, each bit representing one page.
 */
typedef uint8_t g_bitmap_entry;

/**
 * A bitmap stores information about a number of free pages, starting from the
 * base address. In memory, there are multiple bitmaps following each other,
 * the "hasNext" flag indicating whether there is another one.
 *
 * ...[g_bitmap_header][g_bitmap_entry][g_bitmap_entry]...[g_bitmap_header]][g_bitmap_entry]...
 *
 * There can be no pointers within these structures, since the kernel maps the
 * bitmap to its own area in the virtual space before unmapping the setup memory.
 */
typedef struct
{
	g_physical_address baseAddress;
	uint32_t entryCount;
	bool hasNext;
	g_mutex lock;
	uint32_t firstFree;
} __attribute__((packed)) g_bitmap_header;

/**
 * Number of pages each entry contains.
 */
#define G_BITMAP_PAGES_PER_ENTRY (sizeof(g_bitmap_entry) * 8)

/**
 * @returns the next bitmap of the given bitmap, if the "hasNext" flag indicates
 * 		the presence of this bitmap. Offset is calculated by the amount of
 * 		following <g_bitmap_entry>s
 */
#define G_BITMAP_NEXT_UNCHECKED(bitmap) ((g_bitmap_header*) (((g_address) (bitmap)) + sizeof(g_bitmap_header) + (bitmap->entryCount * sizeof(g_bitmap_entry))))
#define G_BITMAP_NEXT(bitmap) (bitmap->hasNext ? G_BITMAP_NEXT_UNCHECKED(bitmap) : nullptr)

/**
 * @returns a pointer to the entries of this bitmap
 */
#define G_BITMAP_ENTRIES(bitmap) ((g_bitmap_entry*) (((g_address) bitmap) + sizeof(g_bitmap_header)))

/**
 * Calculates the index relative to an offset and vice-versa.
 */
#define G_OFFSET_TO_BITMAP_INDEX(address) ((address / G_PAGE_SIZE) / G_BITMAP_PAGES_PER_ENTRY)
#define G_OFFSET_TO_BITMAP_BIT(address) ((address / G_PAGE_SIZE) % G_BITMAP_PAGES_PER_ENTRY)
#define G_BITMAP_TO_OFFSET(index, bit) ((index * G_BITMAP_PAGES_PER_ENTRY * G_PAGE_SIZE) + (bit * G_PAGE_SIZE))

/**
 * Checks whether a bit is set or sets/unsets it.
 */
#define G_BITMAP_IS_SET(bitmap, index, bit) (G_BITMAP_ENTRIES(bitmap)[index] & (1 << bit))
#define G_BITMAP_SET(bitmap, index, bit) (G_BITMAP_ENTRIES(bitmap)[index] |= (1 << bit))
#define G_BITMAP_UNSET(bitmap, index, bit) (G_BITMAP_ENTRIES(bitmap)[index] &= ~(1 << bit))
#define G_BITMAP_ENTRY_FULL 0xFFFFFFFF

#endif
