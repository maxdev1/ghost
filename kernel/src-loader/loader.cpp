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

#include <loader.hpp>

#include <multiboot/multiboot_module_analyzer.hpp>
#include <memory/gdt/gdt_manager.hpp>
#include <memory/memory.hpp>
#include <memory/paging_initializer.hpp>
#include <memory/multiboot_mmap_interpreter.hpp>
#include <memory/bitmap/bitmap_page_allocator.hpp>
#include <kernelloader/kernel_loader.hpp>

#include <logger/logger.hpp>
#include <video/console_video.hpp>
#include <video/pretty_boot.hpp>
#include <multiboot/multiboot_util.hpp>
#include <stdarg.h>

/**
 * Linker symbols defined in the linker script
 */
extern "C" {
void* endAddress;
}

/**
 * Setup information struct containing important information that the
 * kernel needs for further setup.
 */
static g_setup_information setupInformation;

/**
 * Returns the loaders area struct
 */
g_setup_information* g_loader::getSetupInformation() {
	return &setupInformation;
}

/**
 *
 */
static g_bitmap_page_allocator physicalAllocator;

/**
 *
 */
g_bitmap_page_allocator* g_loader::getPhysicalAllocator() {
	return &physicalAllocator;
}

/**
 * 
 */
uint32_t g_loader::findFreeMemory(g_multiboot_information* info, uint32_t start, int count) {

	g_log_info("%! searching for %i free pages (starting at %h)", "loader", count, start);
	g_physical_address location = start;

	while (location < 0xFFFFFFFF) {

		bool notWithinModule = true;

		// For each of the required pages, check if it is within a module
		for (int i = 0; i < count; i++) {
			uint32_t pos = location + i * G_PAGE_SIZE;

			// Check one of the modules contains this position
			for (uint32_t i = 0; i < info->modulesCount; i++) {
				g_multiboot_module* module = (g_multiboot_module*) (info->modulesAddress + sizeof(g_multiboot_module) * i);

				uint32_t moduleStart = G_PAGE_ALIGN_DOWN(module->moduleStart);
				uint32_t moduleEnd = G_PAGE_ALIGN_UP(module->moduleEnd);

				if (pos >= moduleStart && pos < moduleEnd) {
					notWithinModule = false;
					location = moduleEnd;
					break;
				}
			}
		}

		if (notWithinModule) {
			g_log_info("%# found: %h", location);
			return location;
		}

		location += G_PAGE_SIZE;
	}

	panic("%! could not find free memory chunk", "loader");
	return 0;
}

/**
 * 
 */
void g_loader::initialize(g_multiboot_information* multibootInformation) {

	// Store multiboot structure
	setupInformation.multibootInformation = multibootInformation;

	// Begin initialization
	g_log_info("%! loader initializing", "loader");

	// End of the loader binary in memory
	uint32_t loaderEndAddress = G_PAGE_ALIGN_UP((uint32_t ) &endAddress);

	// Find free spaces to place the GDT and the bitmap
	G_PRETTY_BOOT_STATUS("Preparing memory", 3);
	uint32_t gdtAreaStart = findFreeMemory(multibootInformation, loaderEndAddress, 1);
	uint32_t gdtAreaEnd = gdtAreaStart + G_PAGE_SIZE;

	uint32_t bitmapStart = findFreeMemory(multibootInformation, gdtAreaEnd, G_PAGE_ALIGN_UP(G_BITMAP_SIZE) / G_PAGE_SIZE);
	uint32_t bitmapEnd = G_PAGE_ALIGN_UP(bitmapStart + G_BITMAP_SIZE);

	// The "reservedAreaEnd" is the end of the memory (somewhere above 1MiB)
	// that is not occupied by the loader binary or the pages that we split
	// of for use as bitmap and GDT.
	uint32_t reservedAreaEnd = bitmapEnd;

#if G_LOGGING_DEBUG
	// Information output
	g_log_debug("%! available modules:", "mmodule");
	for (uint32_t i = 0; i < multibootInformation->modulesCount; i++) {
		g_multiboot_module* module = (g_multiboot_module*) (multibootInformation->modulesAddress + sizeof(g_multiboot_module) * i);
		g_log_debug("%#   '%s' at %h - %h", module->path, module->moduleStart, module->moduleEnd);
	}

	g_log_debug("%! calculated addresses:", "loader");
	g_log_debug("%#   gdt area:            %h", gdtAreaStart);
	g_log_debug("%#   bitmap:              %h", bitmapStart);
	g_log_debug("%#   reserved area end:   %h", reservedAreaEnd);
#endif

	// Store setup information
	setupInformation.bitmapStart = bitmapStart;
	setupInformation.bitmapEnd = bitmapEnd;

	// Set up the GDT. Here we pass the address of the gdt area, which contains enough space to
	// create the descriptor table and its pointer.
	g_gdt_manager::initialize(gdtAreaStart);

	// Read GRUB map to add free pages to the allocator
	physicalAllocator.initialize((g_bitmap_entry*) bitmapStart);
	g_multiboot_mmap_interpreter::load(&physicalAllocator, reservedAreaEnd);

	// Set up paging, this relocates the multiboot modules
	g_paging_initializer::initialize(reservedAreaEnd, &setupInformation);
	// IMPORTANT: Now the multiboot module location has changed!

	// Load kernel binary
	G_PRETTY_BOOT_STATUS("Locating kernel binary", 3);
	g_log_info("%! locating kernel binary...", "loader");

	g_multiboot_module* kernelModule = g_multiboot_util::findModule(setupInformation.multibootInformation, "/boot/kernel");
	if (kernelModule) {

		G_PRETTY_BOOT_STATUS("Loading kernel", 5);
		g_log_info("%! found kernel binary at %h, loading...", "loader", kernelModule->moduleStart);

		g_kernel_loader::load(kernelModule);
		g_loader::panic("%! something went wrong during boot process, halting", "loader");
	} else {
		G_PRETTY_BOOT_FAIL("Kernel module not found");
		g_loader::panic("%! kernel module not found", "loader");
	}
}

/**
 * 
 */
void g_loader::panic(const char* msg, ...) {

	g_log_info("%! an unrecoverable error has occured. reason:", "lpanic");

	va_list valist;
	va_start(valist, msg);
	g_logger::printFormatted(msg, valist);
	va_end(valist);
	g_logger::printCharacter('\n');

	asm("cli");
	for (;;) {
		asm("hlt");
	}
}
