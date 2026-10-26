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
#include "kernel/logger/logger.hpp"

static g_hashmap<g_pid, g_filesystem_process*>* filesystemProcessInfo;
static g_mutex descriptorLock;
static uint64_t nextDescriptorGeneration;

void filesystemProcessInitialize()
{
    mutexInitializeGlobal(&descriptorLock, __func__);
    filesystemProcessInfo = hashmapCreateNumeric<g_pid, g_filesystem_process*>(128);
}

void filesystemProcessCreate(g_pid pid)
{
    auto info = (g_filesystem_process*) heapAllocateClear(sizeof(g_filesystem_process));
    info->nextDescriptor = 3;
    info->descriptors = hashmapCreateNumeric<g_fd, g_file_descriptor*>(128);
    mutexAcquire(&descriptorLock);
    hashmapPut(filesystemProcessInfo, pid, info);
    mutexRelease(&descriptorLock);
}

g_fs_open_status filesystemProcessCreateDescriptor(g_pid pid, g_fs_virt_id nodeId, g_file_flag_mode flags,
                                                   g_file_descriptor** outDescriptor, g_fd optionalFd, uint64_t initialOffset)
{
    *outDescriptor = nullptr;
    auto descriptor = (g_file_descriptor*) heapAllocateClear(sizeof(g_file_descriptor));
    descriptor->nodeId = nodeId;
    descriptor->node = filesystemGetNode(nodeId);
    descriptor->openFlags = flags;
    descriptor->offset = initialOffset;
    descriptor->references = 1;
    mutexInitializeTask(&descriptor->operationLock, __func__);
    waitQueueInitialize(&descriptor->waiters);
    mutexAcquire(&descriptorLock);
    auto info = hashmapGet(filesystemProcessInfo, pid, (g_filesystem_process*) nullptr);
    if(!info || !descriptor->node ||
       (optionalFd != G_FD_NONE && hashmapGet(info->descriptors, optionalFd, (g_file_descriptor*) nullptr)))
    {
        mutexRelease(&descriptorLock);
        heapFree(descriptor);
        return G_FS_OPEN_ERROR;
    }
    if(optionalFd == G_FD_NONE)
    {
        while(hashmapGet(info->descriptors, info->nextDescriptor, (g_file_descriptor*) nullptr))
            ++info->nextDescriptor;
        descriptor->id = info->nextDescriptor++;
    }
    else
        descriptor->id = optionalFd;
    descriptor->generation = ++nextDescriptorGeneration;
    hashmapPut(info->descriptors, descriptor->id, descriptor);
    mutexRelease(&descriptorLock);
    *outDescriptor = descriptor;
    return G_FS_OPEN_SUCCESSFUL;
}

g_file_descriptor* filesystemProcessAcquireDescriptor(g_pid pid, g_fd fd, uint64_t generation)
{
    mutexAcquire(&descriptorLock);
    auto info = hashmapGet(filesystemProcessInfo, pid, (g_filesystem_process*) nullptr);
    auto descriptor = info ? hashmapGet(info->descriptors, fd, (g_file_descriptor*) nullptr) : nullptr;
    if(descriptor && generation && descriptor->generation != generation)
        descriptor = nullptr;
    if(descriptor)
        ++descriptor->references;
    mutexRelease(&descriptorLock);
    return descriptor;
}

void filesystemProcessReleaseDescriptor(g_file_descriptor* descriptor)
{
    if(!descriptor)
        return;
    mutexAcquire(&descriptorLock);
    bool dispose = --descriptor->references == 0 && descriptor->closed;
    mutexRelease(&descriptorLock);
    if(dispose)
    {
        auto delegate = filesystemFindDelegate(descriptor->node);
        // Delegate close may destroy a pipe and its VFS node. No descriptor
        // reader or creator can still be borrowing either object at this point.
        if(delegate->close)
            delegate->close(descriptor->node, descriptor->openFlags);
        heapFree(descriptor);
    }
}

void filesystemProcessRegisterDescriptorWait(g_file_descriptor* descriptor, g_task* task)
{
    // Delegate wait preparation has finished and released its locks. Close
    // either finds this registration, or was earlier and we undo the sleep.
    mutexAcquire(&descriptorLock);
    if(descriptor->closed)
        taskingWake(task);
    else
        waitQueueAdd(&descriptor->waiters, task->id);
    mutexRelease(&descriptorLock);
}

g_fs_close_status filesystemProcessCloseDescriptor(g_pid pid, g_fd fd)
{
    mutexAcquire(&descriptorLock);
    auto info = hashmapGet(filesystemProcessInfo, pid, (g_filesystem_process*) nullptr);
    auto descriptor = info ? hashmapTake(info->descriptors, fd, (g_file_descriptor*) nullptr) : nullptr;
    if(!descriptor)
    {
        mutexRelease(&descriptorLock);
        return G_FS_CLOSE_INVALID_FD;
    }
    descriptor->closed = true;
    ++descriptor->references; // keep it alive while notifying blocked operations
    mutexRelease(&descriptorLock);
    waitQueueWake(&descriptor->waiters);
    filesystemProcessReleaseDescriptor(descriptor);
    return G_FS_CLOSE_SUCCESSFUL;
}

void filesystemProcessRemoveDescriptor(g_pid pid, g_fd fd)
{
    filesystemProcessCloseDescriptor(pid, fd);
}

void filesystemProcessRemove(g_pid pid)
{
    mutexAcquire(&descriptorLock);
    auto info = hashmapTake(filesystemProcessInfo, pid, (g_filesystem_process*) nullptr);
    mutexRelease(&descriptorLock);
    if(!info)
        return;
    // No new lookup can reach this detached map. In-flight pins are independent
    // of the map/process storage and release their objects on completion.
    while(true)
    {
        auto iter = hashmapIteratorStart(info->descriptors);
        bool found = hashmapIteratorHasNext(&iter);
        auto descriptor = found ? hashmapIteratorNext(&iter)->value : nullptr;
        hashmapIteratorEnd(&iter);
        if(!descriptor)
            break;
        mutexAcquire(&descriptorLock);
        hashmapRemove(info->descriptors, descriptor->id);
        descriptor->closed = true;
        ++descriptor->references;
        mutexRelease(&descriptorLock);
        waitQueueWake(&descriptor->waiters);
        filesystemProcessReleaseDescriptor(descriptor);
    }
    hashmapDestroy(info->descriptors);
    heapFree(info);
}

g_fs_clonefd_status filesystemProcessCloneDescriptor(g_pid sourcePid, g_fd sourceFd, g_pid targetPid, g_fd targetFd, g_fd* outFd)
{
	auto sourceDescriptor = filesystemProcessAcquireDescriptor(sourcePid, sourceFd);
	if(!sourceDescriptor)
		return G_FS_CLONEFD_INVALID_SOURCE_FD;

	if(sourcePid == targetPid && sourceFd == targetFd)
    {
        *outFd = sourceFd;
        filesystemProcessReleaseDescriptor(sourceDescriptor);
        return G_FS_CLONEFD_SUCCESSFUL;
    }
    if(targetFd != G_FD_NONE)
        filesystemClose(targetPid, targetFd, true);
    g_fs_node* node = sourceDescriptor->node;
	if(!node)
	{
		logInfo("%! failed to clone descriptor %i to process %i, node not found", "fs", sourceDescriptor->id, targetPid);
		filesystemProcessReleaseDescriptor(sourceDescriptor);
		return G_FS_CLONEFD_INVALID_SOURCE_FD;
	}

	g_file_descriptor* createdFd;
    mutexAcquire(&sourceDescriptor->operationLock);
    const auto offset = sourceDescriptor->offset;
    mutexRelease(&sourceDescriptor->operationLock);
	g_fs_open_status status = filesystemOpenNode(node, sourceDescriptor->openFlags, targetPid, &createdFd, targetFd, offset);
	if(status != G_FS_OPEN_SUCCESSFUL)
	{
		logInfo("%! failed to clone descriptor %i to process %i in descriptor %i with status %i", "fs", sourceDescriptor->id, targetPid, targetFd, status);
		filesystemProcessReleaseDescriptor(sourceDescriptor);
		return G_FS_CLONEFD_ERROR;
	}
	*outFd = createdFd->id;

    filesystemProcessReleaseDescriptor(createdFd);

	filesystemProcessReleaseDescriptor(sourceDescriptor);
	return G_FS_CLONEFD_SUCCESSFUL;
}

/**
 *
 */
void filesystemProcessCreateStdioPipe(g_pid sourcePid, g_fd sourceFd, g_pid targetPid, g_fd targetFd, g_fd* outFd)
{
	if(sourceFd == G_FD_NONE)
	{
		*outFd = G_FD_NONE;
		return;
	}

	auto status = filesystemProcessCloneDescriptor(sourcePid, sourceFd, targetPid, targetFd, outFd);
	if(status != G_FS_CLONEFD_SUCCESSFUL)
	{
		logInfo("%! failed to create stdio by cloning fd %i:%i to %i:%i", "filesystem", sourcePid, sourceFd, targetPid, targetFd);
		return;
	}
}

/**
 *
 */
void filesystemProcessCreateStdio(g_pid sourcePid, g_fd* sourceStdio, g_pid targetPid, g_fd* targetStdio)
{
	filesystemProcessCreateStdioPipe(sourcePid, sourceStdio[0], targetPid, 0, &targetStdio[0]);
	filesystemProcessCreateStdioPipe(sourcePid, sourceStdio[1], targetPid, 1, &targetStdio[1]);
	filesystemProcessCreateStdioPipe(sourcePid, sourceStdio[2], targetPid, 2, &targetStdio[2]);
}
