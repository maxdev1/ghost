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

#ifndef __KERNEL_FILESYSTEM_RAMDISK_DELEGATE__
#define __KERNEL_FILESYSTEM_RAMDISK_DELEGATE__

#include "ghost/fs.h"
#include "shared/system/mutex.hpp"
#include "kernel/filesystem/filesystem.hpp"

g_fs_open_status filesystemRamdiskDelegateOpen(g_fs_node* node);

g_fs_close_status filesystemRamdiskDelegateClose(g_fs_node* node);

g_fs_open_status filesystemRamdiskDelegateDiscover(g_fs_node* parent, const char* name, g_fs_node** outNode);

g_fs_read_status filesystemRamdiskDelegateRead(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outRead);

g_fs_write_status filesystemRamdiskDelegateWrite(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outWrote);

g_fs_length_status filesystemRamdiskDelegateGetLength(g_fs_node* node, uint64_t* outLength);

g_fs_open_status filesystemRamdiskDelegateCreate(g_fs_node* parent, const char* name, g_fs_node** outFile);

g_fs_open_status filesystemRamdiskDelegateTruncate(g_fs_node* file);

#endif
