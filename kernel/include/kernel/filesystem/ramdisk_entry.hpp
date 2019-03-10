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

#ifndef __KERNEL_RAMDISK_ENTRY__
#define __KERNEL_RAMDISK_ENTRY__

#include "ghost/stdint.h"
#include "ghost/ramdisk.h"

/**
 * Struct of a ramdisk entry
 */
struct g_ramdisk_entry
{
	g_ramdisk_entry* next;
	g_ramdisk_entry_type type;
	g_ramdisk_id id;
	g_ramdisk_id parentid;

	char* name;

	uint32_t dataSize;
	uint8_t* data;

	bool dataOnRamdisk;
	uint32_t notOnRdBufferLength;
};

#endif
