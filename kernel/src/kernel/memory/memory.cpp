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

#include "kernel/memory/memory.hpp"
#include "kernel/debug/debug_interface.hpp"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/kernel.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/memory/constants.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/tasking/task.hpp"
#include "kernel/logger/logger.hpp"

g_address_range_pool* memoryVirtualRangePool = nullptr;
g_bitmap_page_allocator memoryPhysicalAllocator;

void memoryInitialize(limine_memmap_response* memoryMap)
{
	logInfo("%! initializing kernel memory with map at %x", "mem", memoryMap);

	bitmapPageAllocatorInitialize(&memoryPhysicalAllocator, memoryMap);
	logInfo("%! available: %i MiB", "memory", (memoryPhysicalAllocator.freePageCount * G_PAGE_SIZE) / 1024 / 1024);

	heapInitialize();

	memoryVirtualRangePool = (g_address_range_pool*) heapAllocate(sizeof(g_address_range_pool));
	addressRangePoolInitialize(memoryVirtualRangePool);
	addressRangePoolAddRange(memoryVirtualRangePool, G_MEM_KERN_VIRT_RANGES_START, G_MEM_KERN_VIRT_RANGES_END);

	pageReferenceTrackerInitialize();
}

g_physical_address memoryPhysicalAllocate(bool untracked)
{
	g_physical_address page = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
	if(!untracked && page)
		pageReferenceTrackerIncrement(page);
	return page;
}

void memoryPhysicalFree(g_physical_address page)
{
	if(!page)
		return;
	if(pageReferenceTrackerDecrement(page) == 0)
		bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, page);
}

g_virtual_address memoryAllocateKernel(int32_t pages)
{
	g_virtual_address virt = addressRangePoolAllocate(memoryVirtualRangePool, pages);
	for(int32_t i = 0; i < pages; i++)
	{
		g_physical_address phys = memoryPhysicalAllocate();
		pagingMapPage(virt + (i * G_PAGE_SIZE), phys, G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_DEFAULT);
	}
	return virt;
}

void memoryFreeKernelRange(g_virtual_address address)
{
	auto range = addressRangePoolFind(memoryVirtualRangePool, address);
	if(!range)
	{
		logWarn("%! tried to free unallocated kernel range at %x", "memory", address);
		return;
	}

	for(int32_t i = 0; i < range->pages; i++)
	{
		g_virtual_address virt = range->base + (i * G_PAGE_SIZE);
		g_physical_address phys = pagingVirtualToPhysical(virt);
		pagingUnmapPage(virt);
		memoryPhysicalFree(phys);
	}

	addressRangePoolFree(memoryVirtualRangePool, address);
}

void memoryOnDemandMapFile(g_process* process, g_fd file, g_offset fileOffset, g_address fileStart, g_ptrsize fileSize,
                           g_ptrsize memorySize)
{
	g_memory_file_ondemand* mapping = (g_memory_file_ondemand*) heapAllocate(sizeof(g_memory_file_ondemand));
	mapping->fd = file;
	mapping->fileStart = fileStart;
	mapping->fileOffset = fileOffset;
	mapping->fileSize = fileSize;
	mapping->memSize = memorySize;

	mapping->next = process->onDemandMappings;
	process->onDemandMappings = mapping;
}

g_memory_file_ondemand* memoryOnDemandFindMapping(g_task* task, g_address address)
{
	auto mapping = task->process->onDemandMappings;
	while(mapping)
	{
		if(address >= G_PAGE_ALIGN_DOWN(mapping->fileStart) &&
		   address < G_PAGE_ALIGN_UP(mapping->fileStart + mapping->memSize))
		{
			return mapping;
		}
		mapping = mapping->next;
	}

	return nullptr;
}

bool memoryOnDemandHandlePageFault(g_task* task, g_address accessed)
{
	auto mapping = memoryOnDemandFindMapping(task, accessed);
	if(!mapping)
		return false;

	auto accessedLeft = G_PAGE_ALIGN_DOWN(accessed);
	auto accessedRight = accessedLeft + G_PAGE_SIZE;
	auto fileEnd = mapping->fileStart + mapping->fileSize;

	// Allocate requested page
	pagingMapPage(accessedLeft, memoryPhysicalAllocate(), G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT);

	// Zero everything before content
	g_address zeroLeft = accessedLeft;
	g_address zeroRight = mapping->fileStart > accessedRight ? accessedRight : mapping->fileStart;
	if(zeroLeft < zeroRight)
		memorySetBytes((void*) zeroLeft, 0, zeroRight - zeroLeft);

	// Read data to memory
	g_address copyLeft = mapping->fileStart > accessedLeft ? mapping->fileStart : accessedLeft;
	g_address copyRight = fileEnd > accessedRight ? accessedRight : fileEnd;
	if(copyLeft < copyRight)
	{
		g_offset fileOffset = mapping->fileOffset + (copyLeft - mapping->fileStart);
		if(!filesystemReadToMemory(mapping->fd, fileOffset, (uint8_t*) copyLeft, copyRight - copyLeft))
			return false;
	}

	// Zero everything after content
	g_address rzeroLeft = fileEnd > accessedLeft ? fileEnd : accessedLeft;
	g_address rzeroRight = accessedRight;
	if(rzeroLeft < rzeroRight)
		memorySetBytes((void*) rzeroLeft, 0, rzeroRight - rzeroLeft);

	return true;
}

void* memorySetBytes(void* target, uint8_t value, int32_t length)
{
	auto pos = (uint8_t*) target;

	while(length--)
		*pos++ = value;

	return target;
}

void* memorySetWords(void* target, uint16_t value, int32_t length)
{
	auto pos = (uint16_t*) target;

	while(length--)
		*pos++ =  value;

	return target;
}

void* memoryCopy(void* target, const void* source, int32_t size)
{
	auto targetPtr = (uint8_t*) target;
	auto sourcePtr = (const uint8_t*) source;

	// TODO qword copying

	while(size >= 4)
	{
		*(uint32_t*) targetPtr = *(const uint32_t*) sourcePtr;
		targetPtr += 4;
		sourcePtr += 4;
		size -= 4;
	}

	while(size >= 2)
	{
		*(uint16_t*) targetPtr = *(const uint16_t*) sourcePtr;
		targetPtr += 2;
		sourcePtr += 2;
		size -= 2;
	}

	while(size--)
		*targetPtr++ = *sourcePtr++;

	return target;
}
