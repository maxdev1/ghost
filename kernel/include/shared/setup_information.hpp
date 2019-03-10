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

#ifndef __SETUP_INFORMATION__
#define __SETUP_INFORMATION__

#include "ghost/types.h"
#include "shared/multiboot/multiboot.hpp"

/**
 * A struct filled by the initial loader containing information about things
 * that the loader has prepared for the kernel.
 * 
 * Loader prepares a page directory where all page tables in the kernel area
 * are allocated.
 * - Kernel binary image is loaded at 0xC0000000
 * - Kernel stack is allocated right after the image
 * - Kernel heap is allocated right after the stack
 * - Directory is recursively mapped
 * 
 * The given bitmap gives information about all the available physical memory
 * in the system.
 */
struct g_setup_information
{
	g_virtual_address bitmapStart;
	g_virtual_address bitmapEnd;
	g_multiboot_information *multibootInformation;

	g_virtual_address kernelImageStart;
	g_virtual_address kernelImageEnd;
	g_virtual_address stackStart;
	g_virtual_address stackEnd;
	g_virtual_address heapStart;
	g_virtual_address heapEnd;

	g_physical_address initialPageDirectoryPhysical;
}__attribute__((packed));

#endif
