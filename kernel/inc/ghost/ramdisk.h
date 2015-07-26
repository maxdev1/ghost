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

#ifndef __GHOST_SYS_RAMDISK__
#define __GHOST_SYS_RAMDISK__

#include "ghost/common.h"

__BEGIN_C

// ramdisk spawning status codes
typedef uint8_t g_ramdisk_spawn_status;
#define G_RAMDISK_SPAWN_STATUS_SUCCESSFUL			1
#define G_RAMDISK_SPAWN_STATUS_FAILED_NOT_FOUND		2
#define G_RAMDISK_SPAWN_STATUS_FAILED_NOT_VALID		3
#define G_RAMDISK_SPAWN_STATUS_FAILED_NOT_PERMITTED	4

/**
 * Maximum length of paths within the ramdisk
 */
#define G_RAMDISK_MAXIMUM_PATH_LENGTH				512

// types of ramdisk entries
typedef int g_ramdisk_entry_type;
#define G_RAMDISK_ENTRY_TYPE_UNKNOWN	-1
#define G_RAMDISK_ENTRY_TYPE_FOLDER		0
#define G_RAMDISK_ENTRY_TYPE_FILE		1

/**
 * Ramdisk entry information struct used within system calls
 */
typedef struct {
	g_ramdisk_entry_type type;
	char name[512];
	unsigned int length;
} g_ramdisk_entry_info;

__END_C

#endif
