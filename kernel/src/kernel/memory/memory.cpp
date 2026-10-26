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
#include "kernel/system/processor/processor.hpp"
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

bool memorySizeToPageCount(g_size size, uint32_t* outPages)
{
	*outPages = 0;
	const uint64_t maximumSize = (uint64_t) UINT32_MAX * G_PAGE_SIZE;
	if(!size || size > maximumSize)
		return false;
	*outPages = (size - 1) / G_PAGE_SIZE + 1;
	return true;
}

bool memoryPhysicalIsRangeAddressable(g_physical_address address, g_size size)
{
	if(size == 0)
		return false;

	// Long mode requires PAE; older CPUs without this leaf use 36 physical bits.
	uint32_t eax, ebx, ecx, edx;
	uint32_t physicalAddressBits = 36;
	processorCpuid(0x80000000, &eax, &ebx, &ecx, &edx);
	if(eax >= 0x80000008)
	{
		processorCpuid(0x80000008, &eax, &ebx, &ecx, &edx);
		physicalAddressBits = eax & 0xff;
	}

	if(physicalAddressBits < 32 || physicalAddressBits > 52)
		return false;

	const uint64_t physicalLimit = 1ULL << physicalAddressBits;
	return address < physicalLimit && size <= physicalLimit - address;
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

g_physical_address memoryPhysicalTryAllocate()
{
	auto page = bitmapPageAllocatorTryAllocate(&memoryPhysicalAllocator, 1024);
	if(page)
		pageReferenceTrackerIncrement(page);
	return page;
}

g_virtual_address memoryAllocateKernel(int32_t pages)
{
	if(pages <= 0)
		return 0;
	g_virtual_address virt = addressRangePoolAllocate(memoryVirtualRangePool, pages);
	if(!virt)
		return 0;
	int32_t installed = 0;
	for(; installed < pages; ++installed)
	{
		auto phys = memoryPhysicalTryAllocate();
		if(!phys)
			break;
		if(!pagingMapPage(virt + installed * G_PAGE_SIZE, phys, G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_DEFAULT))
		{
			memoryPhysicalFree(phys);
			break;
		}
	}
	if(installed != pages)
	{
		while(installed)
		{
			auto address = virt + --installed * G_PAGE_SIZE;
			auto physical = pagingVirtualToPhysical(address);
			pagingUnmapPage(address);
			memoryPhysicalFree(physical);
		}
		addressRangePoolFree(memoryVirtualRangePool, virt);
		return 0;
	}
	return virt;
}

void memoryFreeKernelRange(g_virtual_address address)
{
	uint32_t pages;
	if(!addressRangePoolClaimForFree(memoryVirtualRangePool, address, &pages))
	{
		logWarn("%! tried to free unallocated kernel range at %x", "memory", address);
		return;
	}

	for(uint32_t i = 0; i < pages; i++)
	{
		g_virtual_address virt = address + i * G_PAGE_SIZE;
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
	if(pagingVirtualToPhysical(accessedLeft))
	{
		// Another core may have resolved this fault before we got here.
		pagingInvalidatePage(accessedLeft);
		return true;
	}

	// Fill the page through the direct map before publishing a user mapping.
	// Other threads must never execute a partially loaded library/code page.
	auto physical = memoryPhysicalTryAllocate();
	if(!physical)
		return false;
	auto page = (uint8_t*) G_MEM_PHYS_TO_VIRT(physical);
	memorySetBytes(page, 0, G_PAGE_SIZE);

	auto fileEnd = mapping->fileStart + mapping->fileSize;
	auto copyLeft = mapping->fileStart > accessedLeft ? mapping->fileStart : accessedLeft;
	auto copyRight = fileEnd > accessedRight ? accessedRight : fileEnd;
	if(copyLeft < copyRight)
	{
		g_offset fileOffset = mapping->fileOffset + (copyLeft - mapping->fileStart);
		if(!filesystemReadToMemory(mapping->fd, fileOffset, page + (copyLeft - accessedLeft), copyRight - copyLeft))
		{
			memoryPhysicalFree(physical);
			return false;
		}
	}

	if(!pagingMapPage(accessedLeft, physical, G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT))
	{
		// A concurrent fault won publication; discard our private copy.
		memoryPhysicalFree(physical);
		pagingInvalidatePage(accessedLeft);
		return (pagingVirtualToPageEntry(accessedLeft) & G_PAGE_PRESENT) != 0;
	}
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
