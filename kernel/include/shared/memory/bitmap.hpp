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

/**
 * Bitmap entry type
 */
typedef uint8_t g_bitmap_entry;

/**
 * Constants for bitmap measuring
 */
#define G_BITMAP_SIZE			(1024 * 1024)
#define G_BITMAP_BITS_PER_ENTRY	(8)

/**
 * Macros for bitmap calculation
 */
#define G_ADDRESS_TO_BITMAP_INDEX(address)	((address / G_PAGE_SIZE) / G_BITMAP_BITS_PER_ENTRY)
#define G_ADDRESS_TO_BITMAP_BIT(address)	((address / G_PAGE_SIZE) % G_BITMAP_BITS_PER_ENTRY)
#define G_BITMAP_TO_ADDRESS(index, bit)		((index * G_BITMAP_BITS_PER_ENTRY * G_PAGE_SIZE) + (bit * G_PAGE_SIZE))

#define G_BITMAP_IS_SET(bitmap, index, bit)	(bitmap[index] & (1 << bit))
#define G_BITMAP_SET(bitmap, index, bit)	(bitmap[index] |= (1 << bit))
#define G_BITMAP_UNSET(bitmap, index, bit)	(bitmap[index] &= ~(1 << bit))

#endif
