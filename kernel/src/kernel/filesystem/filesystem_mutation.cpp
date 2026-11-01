/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "kernel/filesystem/filesystem.hpp"
#include "kernel/filesystem/filesystem_process.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/utils/string.hpp"

void filesystemUnlinkNode(g_fs_node* node)
{
	if(memoryAtomicLoad(&node->removed, G_MEMORY_ORDER_ACQUIRE) || !node->parent)
		return;

	g_mutex_guard guard(&node->parent->lock);
	auto link = &node->parent->children;
	while(*link && (*link)->node != node)
		link = &(*link)->next;
	if(*link)
	{
		auto entry = *link;
		*link = entry->next;
		heapFree(entry);
	}

	memoryAtomicStore(&node->removed, true, G_MEMORY_ORDER_RELEASE);
	node->parent->upToDate = false;
}

static g_fs_open_status filesystemMutationFind(g_task* task, const char* path, g_filesystem_find_result* result)
{
	g_fs_node* origin = nullptr;
	if(path[0] != '/')
	{
		auto cwd = static_cast<char*>(heapTryAllocate(G_PATH_MAX));
		if(!cwd)
			return G_FS_OPEN_ERROR;

		mutexAcquire(&task->process->lock);
		auto current = task->process->environment.workingDirectory;
		auto length = current ? stringLength(current) : 1;
		if(length >= G_PATH_MAX)
		{
			mutexRelease(&task->process->lock);
			heapFree(cwd);
			return G_FS_OPEN_ERROR;
		}
		memoryCopy(cwd, current ? current : "/", length + 1);
		mutexRelease(&task->process->lock);

		auto found = filesystemFind(nullptr, cwd);
		heapFree(cwd);

		if(found.status != G_FS_OPEN_SUCCESSFUL)
			return found.status;
		origin = found.node;
	}
	*result = filesystemFind(origin, path);
	return result->status;
}

static g_fs_mutation_status filesystemMutationLookupStatus(g_fs_open_status status)
{
	switch(status)
	{
		case G_FS_OPEN_NOT_FOUND:
			return G_FS_MUTATION_NOT_FOUND;
		case G_FS_OPEN_BUSY:
			return G_FS_MUTATION_BUSY;
		default:
			return G_FS_MUTATION_ERROR;
	}
}

g_fs_mutation_status filesystemMakeDirectory(g_task* task, const char* path)
{
	g_filesystem_find_result found{};
	auto status = filesystemMutationFind(task, path, &found);
	if(status == G_FS_OPEN_SUCCESSFUL)
		return G_FS_MUTATION_ALREADY_EXISTS;
	if(status != G_FS_OPEN_NOT_FOUND)
		return filesystemMutationLookupStatus(status);
	if(!found.foundAllButLast)
		return G_FS_MUTATION_NOT_FOUND;
	auto delegate = filesystemFindDelegate(found.lastFoundParent);
	if(!delegate->makeDirectory)
		return G_FS_MUTATION_NOT_SUPPORTED;
	g_fs_node* created = nullptr;
	return delegate->makeDirectory(found.lastFoundParent, found.fileNameStart, &created);
}

g_fs_mutation_status filesystemRemove(g_task* task, const char* path, bool directory)
{
	g_filesystem_find_result found{};
	auto status = filesystemMutationFind(task, path, &found);
	if(status != G_FS_OPEN_SUCCESSFUL)
		return filesystemMutationLookupStatus(status);
	if(found.node->type != (directory ? G_FS_NODE_TYPE_FOLDER : G_FS_NODE_TYPE_FILE))
		return G_FS_MUTATION_INVALID_ARGUMENT;
	auto delegate = filesystemFindDelegate(found.node);
	return delegate->mutate ? delegate->mutate(found.node, G_FS_DELEGATE_REQUEST_REMOVE, 0)
							: G_FS_MUTATION_NOT_SUPPORTED;
}

g_fs_mutation_status filesystemMutateDescriptor(g_task* task, g_fd fd, g_fs_delegate_request_type type, uint64_t length)
{
	auto descriptor = filesystemProcessAcquireDescriptor(task->process->id, fd);
	if(!descriptor)
		return G_FS_MUTATION_NOT_FOUND;
	mutexAcquire(&descriptor->operationLock);
	mutexAcquire(&descriptor->node->lock);
	auto delegate = filesystemFindDelegate(descriptor->node);
	g_fs_mutation_status status = G_FS_MUTATION_NOT_SUPPORTED;
	if(type == G_FS_DELEGATE_REQUEST_RESIZE && !(descriptor->openFlags & G_FILE_FLAG_MODE_WRITE))
		status = G_FS_MUTATION_READ_ONLY;
	else if(delegate->mutate)
		status = delegate->mutate(descriptor->node, type, length);
	mutexRelease(&descriptor->node->lock);
	mutexRelease(&descriptor->operationLock);
	filesystemProcessReleaseDescriptor(descriptor);
	return status;
}
