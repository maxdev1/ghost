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

#include "loader/memory/physical.hpp"
#include "loader/setup_information.hpp"
#include "shared/logger/logger.hpp"
#include "shared/multiboot/multiboot.hpp"
#include "shared/panic.hpp"

uint32_t memoryPhysicalReadMemoryMap(g_address startAfter, g_address bitmapArrayStart)
{
	g_multiboot_information* multiboot = setupInformation.multibootInformation;

	if(!(multiboot->flags & G_MULTIBOOT_FLAGS_MMAP))
		panic("%! no memory map available", "mboot");

	// Those are only used when target address is given
	g_bitmap_header* currentBitmap = (g_bitmap_header*) bitmapArrayStart;
	g_bitmap_header* previousBitmap = nullptr;

	uint32_t totalBitmapsSize = 0;

	g_address mapListEnd = ((g_address) multiboot->memoryMap) + multiboot->memoryMapLength;
	for(g_multiboot_mmap* map = multiboot->memoryMap;
		(g_address) map < mapListEnd;
		map = (g_multiboot_mmap*) ((g_address) map + map->size + sizeof(uint32_t)))
	{
		if(map->type != G_MULTIBOOT_MMAP_TYPE_FREE)
			continue;

		// Make sure start and end are in allowed range
		uint64_t start = map->baseAddress;
		uint64_t end = start + map->length;

		if(start > G_ADDRESS_MAX)
			continue;

		if(start < startAfter)
			start = startAfter;

		if(end > G_ADDRESS_MAX)
			end = G_ADDRESS_MAX;

		start = G_PAGE_ALIGN_UP(start);
		end = G_PAGE_ALIGN_DOWN(end);

		if(start >= end)
			continue;

		// Artificially split the current range into multiple bitmaps to improve multi-core performance
		g_address splitStart = start;
		while(splitStart < end)
		{
			g_address splitEnd = splitStart + G_BITMAP_MAX_RANGE;
			if(splitEnd > end)
				splitEnd = end;

			// Calculate required size of current bitmap
			uint32_t entryCount = ((splitEnd - splitStart) / G_PAGE_SIZE) / G_BITMAP_PAGES_PER_ENTRY;
			uint32_t currentBitmapSize = sizeof(g_bitmap_header) + (entryCount * sizeof(g_bitmap_entry));
			totalBitmapsSize += currentBitmapSize;

			// Write bitmap if it's not a dry-run
			if(bitmapArrayStart)
			{
				currentBitmap->baseAddress = splitStart;
				currentBitmap->entryCount = entryCount;
				currentBitmap->hasNext = false;
				g_bitmap_entry* currentBitmapEntries = G_BITMAP_ENTRIES(currentBitmap);
				for(uint32_t i = 0; i < currentBitmap->entryCount; i++)
				{
					currentBitmapEntries[i] = 0;
				}

				if(previousBitmap)
					previousBitmap->hasNext = true;

				logDebug("%! bitmap: %x, base: %x, entries: %i", "physical", currentBitmap, currentBitmap->baseAddress, currentBitmap->entryCount);

				previousBitmap = currentBitmap;
				currentBitmap = G_BITMAP_NEXT_UNCHECKED(currentBitmap);
			}

			splitStart += G_BITMAP_MAX_RANGE;
		}
	}

	return totalBitmapsSize;
}

g_address memoryPhysicalAllocateInitial(g_address startAfter, int pages)
{
	auto multiboot = setupInformation.multibootInformation;

	for(g_address start = startAfter;
		start < G_ADDRESS_MAX;
		start += G_PAGE_SIZE)
	{
		g_address end = start + pages * G_PAGE_SIZE;

		// Make sure none of the pages is within a multiboot module
		bool inModule = false;
		for(g_address page = start; page < end; page += G_PAGE_SIZE)
		{
			for(int i = 0; i < (int) multiboot->modulesCount; i++)
			{
				g_multiboot_module* module = &multiboot->modules[i];
				if(page >= G_PAGE_ALIGN_DOWN(module->moduleStart) &&
				   page < G_PAGE_ALIGN_UP(module->moduleEnd))
				{
					inModule = true;
					start = G_PAGE_ALIGN_UP(module->moduleEnd);
					break;
				}
			}
		}
		if(inModule)
		{
			continue;
		}

		// Make sure the allocated range is free within the memory map
		bool valid = false;
		g_address mapEnd = ((g_address) multiboot->memoryMap) + multiboot->memoryMapLength;
		for(g_multiboot_mmap* map = multiboot->memoryMap;
			(g_address) map < mapEnd;
			map = (g_multiboot_mmap*) ((g_address) map + map->size + sizeof(uint32_t)))
		{
			if(map->type == G_MULTIBOOT_MMAP_TYPE_FREE && start >= map->baseAddress && end < map->baseAddress + map->length)
			{
				valid = true;
				break;
			}
		}
		if(!valid)
		{
			logWarn("%! tried to allocate a physical range at %x which was not within allowed ranges", "physical", start);
			continue;
		}

		return start;
	}

	panic("%! failed to allocate physical memory in early stage", "loader");
}
