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

#ifndef GHOST_LOADER_KERNELLOADER_KERNELLOADER
#define GHOST_LOADER_KERNELLOADER_KERNELLOADER

#include "ghost/elf32.h"

#include "shared/multiboot/multiboot.hpp"
#include "shared/setup_information.hpp"

/**
 * The initial kernel heap has a size of 16MiB.
 */
#define G_KERNEL_HEAP_SIZE	0x1000000

/**
 * Loads the given kernel binary module.
 *
 * @param kernelModule	the multiboot module containing the kernel
 */
void kernelLoaderLoad(g_multiboot_module* kernelModule);

/**
 * Checks the ELF32 header for validity.
 *
 * @param header the header to check
 */
void kernelLoaderCheckHeader(elf32_ehdr* header);

/**
 * Loads the kernel binary starting at the given ELF header.
 *
 * @param header the ELF header
 */
void kernelLoaderLoadBinary(elf32_ehdr* header);

/**
 * Creates the kernels heap.
 */
void kernelLoaderCreateHeap();

/**
 * Changes stack and calls the kernel main function.
 */
void kernelLoaderEnterMain(g_address entryAddress, g_address newEsp);

#endif
