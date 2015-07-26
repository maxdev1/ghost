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

#include <memory/bitmap/bitmap_page_allocator.hpp>

#include <multiboot/multiboot.hpp>
#include <memory/paging.hpp>

#include <logger/logger.hpp>

/**
 * 
 */
void g_bitmap_page_allocator::initialize(g_bitmap_entry* theBitmap) {

	bitmap = theBitmap;

	g_log_debug("%! initializing to bitmap %h", "bitmap", bitmap);
	for (uint32_t i = 0; i < G_BITMAP_LENGTH; i++) {
		bitmap[i] = 0;
	}
	g_log_debug("%! map zeroed", "physical");
}

/**
 * 
 */
void g_bitmap_page_allocator::markFree(uint32_t address) {
	uint32_t index = G_ADDRESS_TO_BITMAP_INDEX(address);
	uint32_t bit = G_ADDRESS_TO_BITMAP_BIT(address);

	G_BITMAP_SET(bitmap, index, bit);
}

/**
 * 
 */
uint32_t g_bitmap_page_allocator::allocate() {

	for (uint32_t i = 0; i < G_BITMAP_LENGTH; i++) {
		// If the entry is more than zero there is at least one free
		if (bitmap[i] > 0) {

			for (uint32_t b = 0; b < G_BITMAP_BITS_PER_ENTRY; b++) {
				// If this bit is set, this page is free
				if (G_BITMAP_IS_SET(bitmap, i, b)) {
					G_BITMAP_UNSET(bitmap, i, b);

					// Page found, clear if necessary and return
					return G_BITMAP_TO_ADDRESS(i, b);
				}
			}
		}
	}

	return 0;
}
