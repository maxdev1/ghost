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

#include <memory/physical/pp_allocator.hpp>
#include <memory/bitmap/bitmap.hpp>
#include <memory/bitmap/bitmap_page_allocator.hpp>
#include <logger/logger.hpp>
#include <kernel.hpp>
#include "debug/debug_interface_kernel.hpp"

static uint32_t freePageCount = 0;

static g_bitmap_entry bitmap[G_BITMAP_LENGTH];
static g_bitmap_page_allocator physicalAllocator;

/**
 *
 */
uint32_t g_pp_allocator::getFreePageCount() {
	return freePageCount;
}

/**
 *
 */
void g_pp_allocator::initializeFromBitmap(g_physical_address bitmapStart, g_physical_address bitmapEnd) {

	// Check bitmap length
	g_log_debug("%! checking bitmap", "ppa");
	uint32_t bitmapBytes = bitmapEnd - bitmapStart;
	uint32_t bitmapExpectedBytes = G_BITMAP_LENGTH * sizeof(g_bitmap_entry);

	if (bitmapBytes == bitmapExpectedBytes) {
		g_log_debug("%! bitmap check successful", "ppa");
	} else {
		g_kernel::panic("%! bitmap has wrong length", "ppa");
	}

	// Copy the bitmap and count the pages
	g_log_debug("%! copying bitmap", "ppa");

	physicalAllocator.initialize(bitmap);
	g_memory::copy(bitmap, (void*) bitmapStart, G_BITMAP_SIZE);

	// Count free pages
	for (uint32_t i = 0; i < G_BITMAP_LENGTH; i++) {
		for (uint8_t b = 0; b < G_BITMAP_BITS_PER_ENTRY; b++) {
			if (G_BITMAP_IS_SET(bitmap, i, b)) {
				++freePageCount;
			}
		}
	}

	G_DEBUG_INTERFACE_SYSTEM_INFORMATION("memory.freePageCount", freePageCount);

	g_log_debug("%! bitmap analyzed, got %i free pages", "ppa", freePageCount);
}

/**
 *
 */
void g_pp_allocator::free(g_physical_address page) {
	physicalAllocator.markFree(page);

	++freePageCount;

	G_DEBUG_INTERFACE_MEMORY_SET_PAGE_USAGE(page, 0);
}

/**
 *
 */
g_physical_address g_pp_allocator::allocate() {
	g_physical_address page = physicalAllocator.allocate();

	if (page == 0) {
		g_log_info("%! critical: physical page allocator has no pages left", "ppa");
		g_kernel::panic("%! out of physical memory", "ppa");
	}

	--freePageCount;

	G_DEBUG_INTERFACE_MEMORY_SET_PAGE_USAGE(page, 1);
	return page;
}
