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

#ifndef __KERNEL_ADDRESS_RANGE_POOL__
#define __KERNEL_ADDRESS_RANGE_POOL__

#include "ghost/types.h"
#include "shared/system/mutex.hpp"

struct g_address_range
{
	g_address_range* next;
	bool used;
	g_address base;
	uint32_t pages;
	uint8_t flags;
};

struct g_address_range_pool
{
	g_address_range* first;
	g_mutex lock;
};

void addressRangePoolInitialize(g_address_range_pool* pool);

void addressRangePoolAddRange(g_address_range_pool* pool, g_address start, g_address end);

void addressRangePoolCloneRanges(g_address_range_pool* pool, g_address_range_pool* other);

void addressRangePoolReleaseRanges(g_address_range_pool* pool);

void addressRangePoolMerge(g_address_range_pool* pool);

g_address addressRangePoolAllocate(g_address_range_pool* pool, uint32_t pages, uint8_t flags = 0);

int32_t addressRangePoolFree(g_address_range_pool* pool, g_address base);

g_address_range* addressRangePoolGetRanges(g_address_range_pool* pool);

g_address_range* addressRangePoolFind(g_address_range_pool* pool, g_address base);

void addressRangePoolDump(g_address_range_pool* pool, bool onlyFree = false);

#endif
