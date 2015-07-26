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
#include <multiboot/multiboot.hpp>
#include <kernelloader/setup_information.hpp>

/**
 * The initial kernel heap is sized as 16MB. For example, the stacked
 * physical allocator needs this amount to convert its bitmap to stack pages.
 */
#define G_KERNEL_HEAP_SIZE	0x1000000

/**
 * The kernel loader class loads the given module as an ELF executable.
 */
class g_kernel_loader {
public:

	/**
	 * Loads the given module as the kernel.
	 *
	 * @param kernelModule	the multiboot module containing the kernel
	 */
	static void load(g_multiboot_module* kernelModule);

private:

	/**
	 * Checks the ELF32 header for validity.
	 *
	 * @param header		the header to check
	 */
	static void checkHeader(elf32_ehdr* header);

	/**
	 * Loads the kernel binary starting at the given ELF header, writing the binary location
	 * in memory to the setupInformation structure.
	 *
	 * @param header			the ELF header
	 * @param setupInformation	the setup information where the load information should be stored
	 */
	static void loadBinary(elf32_ehdr* header, g_setup_information* setupInformation);
};

#endif
