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

#include "kernel/memory/chunk_allocator.hpp"
#include "shared/logger/logger.hpp"

void chunkAllocatorInitialize(g_chunk_allocator* allocator, g_virtual_address start, g_virtual_address end)
{
	mutexInitialize(&allocator->lock);
	allocator->first = (g_chunk_header*) start;
	allocator->first->used = false;
	allocator->first->size = end - start - sizeof(g_chunk_header);
	allocator->first->next = 0;
}

void chunkAllocatorExpand(g_chunk_allocator* allocator, uint32_t size)
{
	mutexAcquire(&allocator->lock);

	g_chunk_header* last = allocator->first;
	while(last->next)
		last = last->next;

	g_chunk_header* newChunk = (g_chunk_header*) (((uint32_t) last) + sizeof(g_chunk_header) + last->size);
	newChunk->used = false;
	newChunk->size = size - sizeof(g_chunk_header);
	newChunk->next = 0;
	last->next = newChunk;

	mutexRelease(&allocator->lock);

	chunkAllocatorMerge(allocator);
}

void* chunkAllocatorAllocate(g_chunk_allocator* allocator, uint32_t size)
{
	mutexAcquire(&allocator->lock);
	if(allocator->first == 0)
	{
		logInfo("%! critical: tried to use allocate on uninitialized chunk allocator", "chunkalloc");
		return 0;
	}

	if(size < G_CHUNK_ALLOCATOR_MIN_ALLOC)
		size = G_CHUNK_ALLOCATOR_MIN_ALLOC;

	g_chunk_header* current = allocator->first;
	while(current)
	{
		if(!current->used && (current->size >= (size + sizeof(g_chunk_header) + G_CHUNK_ALLOCATOR_MIN_ALLOC)))
		{
			g_chunk_header* splinter = (g_chunk_header*) ((uint32_t) current + sizeof(g_chunk_header) + size);
			splinter->size = current->size - size - sizeof(g_chunk_header);
			splinter->used = false;
			splinter->next = current->next;

			current->next = splinter;
			current->used = true;
			current->size = size;

			mutexRelease(&allocator->lock);
			return (void*) (((uint32_t) current) + sizeof(g_chunk_header));
		}
		current = current->next;
	}

	mutexRelease(&allocator->lock);
	return 0;
}

uint32_t chunkAllocatorFree(g_chunk_allocator* allocator, void* mem)
{
	if(!allocator->first)
	{
		logWarn("%! critical: tried to use free on uninitialized chunk allocator", "chunkalloc");
		return 0;
	}
	mutexAcquire(&allocator->lock);

	g_chunk_header* blockHeader = (g_chunk_header*) (((uint32_t) mem) - sizeof(g_chunk_header));
	blockHeader->used = false;
	uint32_t size = blockHeader->size;
	chunkAllocatorMerge(allocator);

	mutexRelease(&allocator->lock);
	return size;
}

void chunkAllocatorMerge(g_chunk_allocator* allocator)
{
	mutexAcquire(&allocator->lock);

	g_chunk_header* current = (g_chunk_header*) allocator->first;
	while(current && current->next)
	{
		if(!(current->used) && !(current->next->used))
		{
			current->size += sizeof(g_chunk_header) + current->next->size;
			current->next = current->next->next;
		} else
		{
			current = current->next;
		}
	}

	mutexRelease(&allocator->lock);
}
