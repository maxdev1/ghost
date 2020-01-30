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

#include "kernel/filesystem/filesystem_process.hpp"
#include "kernel/memory/memory.hpp"
#include "shared/logger/logger.hpp"

static g_hashmap<g_pid, g_filesystem_process*>* filesystemProcessInfo;

void filesystemProcessInitialize()
{
	filesystemProcessInfo = hashmapCreateNumeric<g_pid, g_filesystem_process*>(128);
}

void filesystemProcessCreate(g_pid pid)
{
	g_filesystem_process* info = (g_filesystem_process*) heapAllocate(sizeof(g_filesystem_process));

	info->nextDescriptor = 3; // after stdin, stdout, stderr
	mutexInitialize(&info->nextDescriptorLock);
	info->descriptors = hashmapCreateNumeric<g_fd, g_file_descriptor*>(128);

	hashmapPut(filesystemProcessInfo, pid, info);
}

g_fs_open_status filesystemProcessCreateDescriptor(g_pid pid, g_fs_virt_id nodeId, g_file_flag_mode flags, g_file_descriptor** outDescriptor, g_fd optionalFd)
{
	g_filesystem_process* info = hashmapGet<g_pid, g_filesystem_process*>(filesystemProcessInfo, pid, 0);
	if(!info)
	{
		logInfo("%! tried to create file descriptor in process %i that doesn't exist", "filesystem", pid);
		return G_FS_OPEN_ERROR;
	}

	g_file_descriptor* descriptor = (g_file_descriptor*) heapAllocate(sizeof(g_file_descriptor));

	if(optionalFd == G_FD_NONE) {
		mutexAcquire(&info->nextDescriptorLock);
		descriptor->id = info->nextDescriptor++;
		mutexRelease(&info->nextDescriptorLock);
	} else {
		descriptor->id = optionalFd;
	}
	descriptor->nodeId = nodeId;
	descriptor->offset = 0;
	descriptor->openFlags = flags;

	hashmapPut<g_fd, g_file_descriptor*>(info->descriptors, descriptor->id, descriptor);
	*outDescriptor = descriptor;
	return G_FS_OPEN_SUCCESSFUL;
}

g_file_descriptor* filesystemProcessGetDescriptor(g_pid pid, g_fd fd)
{
	g_filesystem_process* info = hashmapGet<g_pid, g_filesystem_process*>(filesystemProcessInfo, pid, 0);
	if(!info)
	{
		logInfo("%! tried to create file descriptor in process %i that doesn't exist", "filesystem", pid);
		return 0;
	}

	return hashmapGet<g_fd, g_file_descriptor*>(info->descriptors, fd, 0);
}

void filesystemProcessRemove(g_pid pid)
{
	g_filesystem_process* info = hashmapGet<g_pid, g_filesystem_process*>(filesystemProcessInfo, pid, 0);
	if(!info)
		return;

	g_hashmap_iterator<g_fd, g_file_descriptor*> iter = hashmapIteratorStart<g_fd, g_file_descriptor*>(info->descriptors);
	while(hashmapIteratorHasNext<g_fd, g_file_descriptor*>(&iter))
	{
		g_hashmap_entry<g_fd, g_file_descriptor*>* entry = hashmapIteratorNext<g_fd, g_file_descriptor*>(&iter);
		filesystemClose(pid, entry->key, false);
		heapFree(entry->value);
	}
	hashmapIteratorEnd<g_fd, g_file_descriptor*>(&iter);

	heapFree(info);
	hashmapRemove<g_pid, g_filesystem_process*>(filesystemProcessInfo, pid);
}

void filesystemProcessRemoveDescriptor(g_pid pid, g_fd fd)
{
	g_filesystem_process* info = hashmapGet<g_pid, g_filesystem_process*>(filesystemProcessInfo, pid, 0);
	if(!info)
		return;

	g_file_descriptor* descriptor = hashmapGet<g_fd, g_file_descriptor*>(info->descriptors, fd, 0);
	if(!descriptor)
		return;

	hashmapRemove(info->descriptors, fd);
	heapFree(descriptor);
}

g_file_descriptor* filesystemProcessCloneDescriptor(g_file_descriptor* sourceFd, g_pid targetPid, g_fd targetFd) {

	if(targetFd != G_FD_NONE)
		filesystemProcessRemoveDescriptor(targetPid, targetFd);
	
	g_file_descriptor* createdFd;
	g_fs_open_status status = filesystemProcessCreateDescriptor(targetPid, sourceFd->nodeId, sourceFd->openFlags, &createdFd, targetFd);
	if(status != G_FS_OPEN_SUCCESSFUL)
	{
		logInfo("%! failed to clone descriptor %i to process %i in descriptor %i with status %i", "filesystem", sourceFd->id, targetPid, targetFd, status);
		return 0;
	}

	createdFd->offset = sourceFd->offset;
	return createdFd;
}
