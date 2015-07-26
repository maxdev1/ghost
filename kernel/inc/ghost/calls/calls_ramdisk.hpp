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

#ifndef GHOST_API_CALLS_RAMDISKCALLS
#define GHOST_API_CALLS_RAMDISKCALLS

#include "ghost/ramdisk.h"

/**
 * @field path
 * 		the absolute path of the file to find
 *
 * @field nodeId
 * 		the resulting node id. if the file is
 * 		not found, this field is -1.
 *
 * @security-level APPLICATION
 */
typedef struct {
	char* path;

	int32_t nodeId;
}__attribute__((packed)) g_syscall_ramdisk_find;

/**
 * @field childName
 * 		the relative path of the file to find
 *
 * @field parentId
 * 		the id of the parent folder
 *
 * @field nodeId
 * 		the resulting node id. if the file does
 * 		not exist, this field is -1.
 */
typedef struct {
	int32_t parentId;
	char* childName;

	int32_t nodeId;
}__attribute__((packed)) g_syscall_ramdisk_find_child;

/**
 * @field nodeId
 * 		the id of the file to read from
 *
 * @field offset
 * 		the offset within the file
 *
 * @field length
 * 		the maximum number of bytes to write to the buffer
 *
 * @field buffer
 * 		the target buffer
 *
 * @field readBytes
 * 		the resulting number of bytes that were actually read
 */
typedef struct {
	int32_t nodeId;
	uint32_t offset;
	uint32_t length;
	char* buffer;

	int32_t readBytes;
}__attribute__((packed)) g_syscall_ramdisk_read;

/**
 * @field nodeId
 * 		the id of the file to get information of
 *
 * @field type
 * 		the file type, or UNKNOWN/-1 if not existent
 *
 * @field length
 * 		the files length
 *
 * @field name
 * 		the name of the file
 */
typedef struct {
	int32_t nodeId;

	g_ramdisk_entry_type type;
	uint32_t length;
	char name[G_RAMDISK_MAXIMUM_PATH_LENGTH];
}__attribute__((packed)) g_syscall_ramdisk_info;

/**
 * @field nodeId
 * 		the id of the folder
 *
 * @field count
 * 		the resulting number of children
 */
typedef struct {
	int32_t nodeId;

	uint32_t count;
}__attribute__((packed)) g_syscall_ramdisk_child_count;

/**
 * @field nodeId
 * 		the id of the folder
 *
 * @field index
 * 		the index of the child entry
 *
 * @field childId
 * 		the resulting child id
 */
typedef struct {
	int32_t nodeId;
	uint32_t index;

	int32_t childId;
}__attribute__((packed)) g_syscall_ramdisk_child_at;

#endif
