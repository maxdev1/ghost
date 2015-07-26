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

#ifndef GHOST_SHARED_SETUP_INFORMATION
#define GHOST_SHARED_SETUP_INFORMATION

#include "ghost/stdint.h"
#include <multiboot/multiboot.hpp>

/**
 * A struct filled by the initial loader containing important information about
 * memory areas, multiboot, stack or kernel image position.
 */
struct g_setup_information {
	uint32_t bitmapStart;
	uint32_t bitmapEnd;
	g_multiboot_information* multibootInformation;

	uint32_t kernelImageStart;
	uint32_t kernelImageEnd;
	uint32_t stackStart;
	uint32_t stackEnd;
	uint32_t heapStart;
	uint32_t heapEnd;

	uint32_t initialPageDirectoryPhysical;
};

#endif
