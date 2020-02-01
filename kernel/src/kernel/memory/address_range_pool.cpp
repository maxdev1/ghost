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

#include "kernel/memory/address_range_pool.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/kernel.hpp"

#include "shared/memory/paging.hpp"
#include "shared/logger/logger.hpp"

void addressRangePoolInitialize(g_address_range_pool* pool)
{
	pool->first = 0;
	mutexInitialize(&pool->lock);
}

void addressRangePoolAddRange(g_address_range_pool* pool, g_address start, g_address end)
{
	mutexAcquire(&pool->lock);

	g_address_range* newRange = (g_address_range*) heapAllocate(sizeof(g_address_range));
	newRange->base = start;
	newRange->used = false;
	newRange->next = 0;
	newRange->pages = (end - start) / G_PAGE_SIZE;
	newRange->flags = 0;

	if(pool->first == 0)
	{
		// If the pool is empty, add it as first
		newRange->next = 0;
		pool->first = newRange;

	} else
	{
		// Find the last range that has a lower base than this one
		g_address_range* lastBelow = 0;
		g_address_range* range = pool->first;
		while(range)
		{
			if(range->base < newRange->base)
			{
				lastBelow = range;
			}
			range = range->next;
		}

		if(lastBelow)
		{
			// Place the new range between the lastBelow and the one after it
			newRange->next = lastBelow->next;
			lastBelow->next = newRange;

		} else
		{
			// There was no range found that has a lower base than this,
			// so add it as first
			newRange->next = pool->first;
			pool->first = newRange;
		}
	}
	mutexRelease(&pool->lock);

	addressRangePoolMerge(pool);
}

void addressRangePoolCloneRanges(g_address_range_pool* pool, g_address_range_pool* other)
{
	mutexAcquire(&pool->lock);

	if(pool->first)
		addressRangePoolReleaseRanges(pool);

	g_address_range* otherCurrent = (g_address_range*) other->first;
	g_address_range* last = 0;
	while(otherCurrent)
	{
		g_address_range* newRange = (g_address_range*) heapAllocate(sizeof(g_address_range));
		*newRange = *otherCurrent;
		newRange->next = 0;

		if(last)
		{
			last->next = newRange;
			last = newRange;
		} else
		{
			pool->first = newRange;
			last = newRange;
		}
		otherCurrent = otherCurrent->next;
	}

	mutexRelease(&pool->lock);
}

g_address_range* addressRangePoolGetRanges(g_address_range_pool* pool)
{
	return pool->first;
}

g_address addressRangePoolAllocate(g_address_range_pool* pool, uint32_t requestedPages, uint8_t flags)
{
	if(pool == 0)
		kernelPanic("%! tried to access null pool", "addrpool");

	mutexAcquire(&pool->lock);

	if(requestedPages == 0)
	{
		requestedPages = 1;
	}

	// Find an unused range that has more/equal requested pages
	g_address_range* range = pool->first;
	while(range)
	{
		if(!range->used && range->pages >= requestedPages)
			break;

		range = range->next;
	}

	if(range)
	{
		range->used = true;
		range->flags = flags;

		int32_t remainingPages = range->pages - requestedPages;
		if(remainingPages > 0)
		{
			g_address_range* splinter = (g_address_range*) heapAllocate(sizeof(g_address_range));
			splinter->used = false;
			splinter->pages = remainingPages;
			splinter->base = range->base + requestedPages * G_PAGE_SIZE;
			splinter->flags = 0;

			splinter->next = range->next;
			range->next = splinter;
			range->pages = requestedPages;
		}

		mutexRelease(&pool->lock);
		return range->base;
	}

	logInfo("%! critical, no free range of size %i pages", "addrpool", requestedPages);
	addressRangePoolDump(pool);
	mutexRelease(&pool->lock);
	return 0;
}

int32_t addressRangePoolFree(g_address_range_pool* pool, g_address base)
{
	mutexAcquire(&pool->lock);

	int32_t freedPages = -1;

	g_address_range* range = pool->first;
	while(range)
	{
		if(range->base == base)
		{
			break;
		}
		range = range->next;
	}

	if(!range)
	{
		logInfo("%! bug: tried to free a range (%h) that doesn't exist", "addrpool", base);
		mutexRelease(&pool->lock);
		return freedPages;
	}
	if(!range->used)
	{
		logInfo("%! bug: multiple free calls for range %h (pages: %i)", "addrpool", range->base, range->pages);
		mutexRelease(&pool->lock);
		return freedPages;
	}

	range->used = false;
	freedPages = range->pages;
	addressRangePoolMerge(pool);

	mutexRelease(&pool->lock);
	return freedPages;
}

void addressRangePoolMerge(g_address_range_pool* pool)
{
	g_address_range* current = (g_address_range*) pool->first;

	while(current && current->next)
	{
		if(!current->used && !current->next->used && ((current->base + current->pages * G_PAGE_SIZE) == current->next->base))
		{
			current->pages += current->next->pages;

			g_address_range* nextnext = current->next->next;
			delete current->next;
			current->next = nextnext;

		} else
		{
			current = current->next;
		}
	}
}

void addressRangePoolDump(g_address_range_pool* pool, bool onlyFree)
{
	logDebug("%! range structure:", "vra");
	if(pool->first == 0)
	{
		logDebug("%#  cannot dump, no first entry");
		return;
	}

	g_address_range* current = (g_address_range*) pool->first;
	while(current)
	{
		if(!onlyFree || !current->used)
		{
			logDebug("%#  used: %b, base: %h, pages: %i (- %h)", current->used, current->base, current->pages, current->base + current->pages * G_PAGE_SIZE);
		}
		current = current->next;
	}
}

void addressRangePoolReleaseRanges(g_address_range_pool* pool)
{
	g_address_range* range = pool->first;
	while(range)
	{
		g_address_range* next = range->next;
		heapFree(range);
		range = next;
	}
	pool->first = 0;
}

g_address_range* addressRangePoolFind(g_address_range_pool* pool, g_address base)
{
	mutexAcquire(&pool->lock);

	g_address_range* range = pool->first;
	while(range)
	{
		if(range->base == base)
		{
			break;
		}
		range = range->next;
	}

	mutexRelease(&pool->lock);

	return range;
}
