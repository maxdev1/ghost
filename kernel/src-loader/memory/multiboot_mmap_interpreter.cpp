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

#include <memory/multiboot_mmap_interpreter.hpp>
#include <multiboot/multiboot.hpp>
#include <logger/logger.hpp>
#include <loader.hpp>

/**
 * 
 */
void g_multiboot_mmap_interpreter::load(g_bitmap_page_allocator* allocator, uint32_t reservedAreaEnd) {

	g_multiboot_information* mbInfo = g_loader::getSetupInformation()->multibootInformation;

	g_multiboot_mmap* map = (g_multiboot_mmap*) mbInfo->memoryMapAddress;
	uint32_t mapListEnd = mbInfo->memoryMapAddress + mbInfo->memoryMapLength;

	// Iterate over the list of memory maps from GRUB
	g_log_info("%! memory regions:", "memmap");
	while (((uint32_t) map) < mapListEnd) {

		// Check if the map is usable memory
		if (map->type == 1) {
			uint64_t areaStart = (uint64_t) map->baseAddressLower | ((uint64_t) map->baseAddressHigher << 32);
			uint64_t areaEnd = areaStart + ((uint64_t) map->lengthLower | ((uint64_t) map->lengthHigher << 32));

			// If this ranges is out of 32bit bounds, ignore it
			if (areaStart > 0xFFFFFFFF) {
				g_log_info("%# > 0xFFFFFFFF             : not usable");

			} else {
				g_log_infon("%#   %h - %h", (uint32_t ) areaStart, (uint32_t ) areaEnd);

				// Make sure that the mapped area lays behind the kernel
				if (areaStart < reservedAreaEnd) {
					areaStart = reservedAreaEnd;
				}

				// End of area above 32bit? Cut off
				if (areaEnd > 0xFFFFFFFF) {
					areaEnd = 0xFFFFFFFF;
				}

				// Page-align
				areaStart = G_PAGE_ALIGN_UP(areaStart);
				areaEnd = G_PAGE_ALIGN_DOWN(areaEnd);

				// Mark as free
				uint32_t chunkCount = 0;
				uint32_t inModule = 0;

				if (areaEnd > areaStart) {
					// Split into page sized chunks
					while (areaStart < areaEnd - G_PAGE_SIZE) {

						// Exclude memory within modules
						bool isInModule = false;
						for (uint32_t i = 0; i < mbInfo->modulesCount; i++) {
							g_multiboot_module* module = (g_multiboot_module*) (mbInfo->modulesAddress + sizeof(g_multiboot_module) * i);

							if ((areaStart >= G_PAGE_ALIGN_DOWN(module->moduleStart)) && (areaStart < G_PAGE_ALIGN_UP(module->moduleEnd))) {
								isInModule = true;
								break;
							}
						}

						// If its not inside a module, mark as free
						if (isInModule) {
							++inModule;
						} else {
							allocator->markFree(areaStart);
							++chunkCount;
						}

						areaStart += G_PAGE_SIZE;
					}
				}

				g_log_info(": %i available (%i blocked)", chunkCount, inModule);
			}
		}

		// Skip to the next map (the sizeof in the end is something GRUB-specific, look up the docs)
		map = (g_multiboot_mmap*) ((uint32_t) map + map->size + sizeof(uint32_t));
	}
}
