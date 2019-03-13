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
#include "kernel/tasking/wait_resolver.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/kernel.hpp"

#include "shared/system/mutex.hpp"
#include "shared/utils/string.hpp"

g_fs_open_status filesystemPipeDelegateOpen(g_fs_node* node)
{
	// TODO increase pipe references
	return G_FS_OPEN_ERROR;
}

g_fs_close_status filesystemPipeDelegateClose(g_fs_node* node)
{
	// TODO decrease pipe references
	return G_FS_CLOSE_ERROR;
}

g_fs_open_status filesystemPipeDelegateDiscover(g_fs_node* parent, const char* name, g_fs_node** outNode)
{
	return G_FS_OPEN_ERROR;
}

g_fs_read_status filesystemPipeDelegateRead(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outRead)
{
	return G_FS_READ_ERROR;
}

g_fs_write_status filesystemPipeDelegateWrite(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outWrote)
{
	return G_FS_WRITE_ERROR;
}

g_fs_length_status filesystemPipeDelegateGetLength(g_fs_node* node, uint64_t* outLength)
{
	return G_FS_LENGTH_ERROR;
}

g_fs_open_status filesystemPipeDelegateCreate(g_fs_node* parent, const char* name, g_fs_node** outFile)
{
	return G_FS_OPEN_ERROR;
}

g_fs_open_status filesystemPipeDelegateTruncate(g_fs_node* file)
{
	// TODO empty pipe
	return G_FS_OPEN_ERROR;
}

bool filesystemPipeDelegateWaitResolverRead(g_task* task)
{
	g_wait_resolver_for_file_data* waitData = (g_wait_resolver_for_file_data*) task->waitData;
	// TODO return whether we can now read from the pipe in waitData
	return false;
}

bool filesystemPipeDelegateWaitResolverWrite(g_task* task)
{
	g_wait_resolver_for_file_data* waitData = (g_wait_resolver_for_file_data*) task->waitData;
	// TODO return whether we can now write to the pipe in waitData
	return false;
}
