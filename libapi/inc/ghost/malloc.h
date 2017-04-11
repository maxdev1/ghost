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

#ifndef __GHOST_LIBAPI_MALLOC__
#define __GHOST_LIBAPI_MALLOC__

#include "ghost/common.h"

__BEGIN_C

/**
 * Deallocates the given memory (which must have previously been allocated
 * with one of the allocation functions). (N1548-7.22.3.3)
 *
 * @param ptr
 * 		pointer to memory to free, or NULL
 */
void free(void* ptr);

/**
 * Allocates <size> bytes of memory. (N1548-7.22.3.4)
 *
 * @param size
 * 		size in bytes to allocate
 * @return
 * 		allocated memory
 */
void* malloc(size_t size);

__END_C

#endif
