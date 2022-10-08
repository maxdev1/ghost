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

#ifndef __KERNEL_MEMORY__
#define __KERNEL_MEMORY__

#include "ghost/types.h"

#include "shared/setup_information.hpp"
#include "shared/memory/memory.hpp"
#include "shared/memory/bitmap_page_allocator.hpp"

#include "kernel/memory/paging.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/memory/address_range_pool.hpp"

extern g_bitmap_page_allocator memoryPhysicalAllocator;
extern g_address_range_pool* memoryVirtualRangePool;

void memoryInitialize(g_setup_information* setupInformation);

void memoryInitializePhysicalAllocator(g_setup_information* setupInformation);

void memoryUnmapSetupMemory();

#endif
