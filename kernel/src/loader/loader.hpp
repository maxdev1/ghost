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

#ifndef __LOADER__
#define __LOADER__

#include "loader/memory/gdt.hpp"
#include "loader/memory/paging.hpp"
#include "shared/memory/bitmap_page_allocator.hpp"
#include "shared/memory/memory.hpp"
#include "shared/multiboot/multiboot.hpp"
#include "shared/setup_information.hpp"
#include <ghost/stdint.h>

/**
 * Initialization function, called from the loader assembly. Checks the
 * multiboot magic number and then passes the multiboot structure to the
 * loader for further initialization.
 *
 * @param multibootStruct	the multiboot structure passed by bootloader
 * @param magicNumber		the magic number passed by bootloader
 */
extern "C" void loaderMain(g_multiboot_information* multibootStruct, uint32_t magicNumber);

/**
 * Initializes memory management.
 */
void loaderInitializeMemory();

/**
 * Searches for the kernel multiboot module and executes it.
 */
void loaderStartKernel();

/**
 * Sets the global descriptor table up and returns the end address.
 */
g_address loaderSetupGdt();

/**
 * This function is used to find a chunk of free memory before any memory management was initialized.
 * It is used to find a place to put the GDT and the bitmap, to avoid accidentally overwriting
 * multiboot modules.
 */
g_address loaderFindNextFreePages(g_address start, int pages);

void loaderEnableLoggingFeatures();

#endif
