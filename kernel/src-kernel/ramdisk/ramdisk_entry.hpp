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

#ifndef GHOST_RAMDISK_RAMDISKENTRY
#define GHOST_RAMDISK_RAMDISKENTRY

#include "ghost/ramdisk.h"

/**
 * Struct of a ramdisk entry
 */
struct g_ramdisk_entry {
	g_ramdisk_entry* next;
	g_ramdisk_entry_type type;
	uint32_t id;
	uint32_t parentid;

	char* name;

	uint32_t datalength;
	uint8_t* data;

	bool data_on_ramdisk;
	uint32_t not_on_rd_buffer_length;
};

#endif
