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

#ifndef __KERNEL_FILESYSTEM_PROCESS__
#define __KERNEL_FILESYSTEM_PROCESS__

#include "ghost/kernel.h"
#include "ghost/fs.h"
#include "kernel/utils/hashmap.hpp"
#include "kernel/filesystem/filesystem.hpp"

/**
 * Structure of a file descriptor.
 */
struct g_file_descriptor
{
	g_fd id;
	uint64_t offset;
	g_fs_virt_id nodeId;
	g_file_flag_mode openFlags;
};

/**
 * Per-process file system information structure.
 */
struct g_filesystem_process
{
	g_fd nextDescriptor;
	g_mutex nextDescriptorLock;
	g_hashmap<g_fd, g_file_descriptor*>* descriptors;
};

/**
 * Initializes this unit.
 */
void filesystemProcessInitialize();

/**
 * Creates a file system information structure for a process.
 */
void filesystemProcessCreate(g_pid pid);

/**
 * Removes file system information for a process. Closes all file descriptors of this process.
 */
void filesystemProcessRemove(g_pid pid);

/**
 * Creates a file descriptor opening a node.
 */
g_fs_open_status filesystemProcessCreateDescriptor(g_pid pid, g_fs_virt_id nodeId, g_file_flag_mode flags,
		g_file_descriptor** outDescriptor, g_fd optionalFd = G_FD_NONE);

/**
 * Finds a file descriptor.
 */
g_file_descriptor* filesystemProcessGetDescriptor(g_pid pid, g_fd fd);

/**
 * Closes a file descriptor.
 */
void filesystemProcessRemoveDescriptor(g_pid pid, g_fd fd);

/**
 * Clones a file descriptor.
 */
g_file_descriptor* filesystemProcessCloneDescriptor(g_file_descriptor* descriptor, g_pid targetPid, g_fd targetFd);

#endif
