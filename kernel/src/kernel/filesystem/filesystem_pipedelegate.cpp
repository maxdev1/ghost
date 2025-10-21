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

#include "kernel/filesystem/filesystem_pipedelegate.hpp"
#include "kernel/ipc/pipes.hpp"
#include "kernel/kernel.hpp"
#include "kernel/memory/memory.hpp"

#include "kernel/system/mutex.hpp"
#include "kernel/utils/string.hpp"

g_fs_open_status filesystemPipeDelegateOpen(g_fs_node* node, g_file_flag_mode flags)
{
	pipeAddReference(node->physicalId, flags);
	return G_FS_OPEN_SUCCESSFUL;
}

g_fs_close_status filesystemPipeDelegateClose(g_fs_node* node, g_file_flag_mode openFlags)
{
	pipeRemoveReference(node->physicalId, openFlags);
	return G_FS_CLOSE_SUCCESSFUL;
}

g_fs_read_status filesystemPipeDelegateRead(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outRead)
{
	return pipeRead(node->physicalId, buffer, offset, length, outRead);
}

g_fs_write_status filesystemPipeDelegateWrite(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outWrote)
{
	return pipeWrite(node->physicalId, buffer, offset, length, outWrote);
}

g_fs_length_status filesystemPipeDelegateGetLength(g_fs_node* node, uint64_t* outLength)
{
	return pipeGetLength(node->physicalId, outLength);
}

g_fs_open_status filesystemPipeDelegateTruncate(g_fs_node* file)
{
	return pipeTruncate(file->physicalId);
}

void filesystemPipeDelegateWaitForRead(g_tid task, g_fs_node* node)
{
	pipeWaitForRead(task, node->physicalId);
}

void filesystemPipeDelegateWaitForWrite(g_tid task, g_fs_node* node)
{
	pipeWaitForWrite(task, node->physicalId);
}
