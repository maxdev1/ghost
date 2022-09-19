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

#ifndef __GHOST_MEMORY__
#define __GHOST_MEMORY__

#include "ghost/common.h"

__BEGIN_C

/**
 * Page size
 */
#define G_PAGE_SIZE				0x1000
#define G_PAGE_ALIGN_MASK		(G_PAGE_SIZE - 1)

/**
 * Page alignment macros
 */
#define G_PAGE_ALIGN_DOWN(value)				((value) & ~(G_PAGE_SIZE - 1))
#define G_PAGE_ALIGN_UP(value)				(((value) & (G_PAGE_SIZE - 1)) ? (G_PAGE_ALIGN_DOWN((value)) + G_PAGE_SIZE) : (value))

#define G_TABLE_IN_DIRECTORY_INDEX(address)	((uint32_t)((address / G_PAGE_SIZE) / 1024))
#define G_PAGE_IN_TABLE_INDEX(address)		((uint32_t)((address / G_PAGE_SIZE) % 1024))

__END_C

#endif
