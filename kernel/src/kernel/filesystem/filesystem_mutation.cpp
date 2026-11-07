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
	auto status = filesystemChooseOrigin(path, task, origin);
	if(status != G_FS_OPEN_SUCCESSFUL)
		return status;

	*result = filesystemFindPolicy(origin, path, false);
	return result->status;
}

g_fs_mutation_status filesystemMutationLookupStatus(g_fs_open_status status)
{
	switch(status)
	{
		case G_FS_OPEN_ALREADY_EXISTS:
			return G_FS_MUTATION_ALREADY_EXISTS;
		case G_FS_OPEN_LOOP:
			return G_FS_MUTATION_LOOP;
		case G_FS_OPEN_NAME_TOO_LONG:
			return G_FS_MUTATION_NAME_TOO_LONG;
		case G_FS_OPEN_NOT_DIRECTORY:
			return G_FS_MUTATION_NOT_DIRECTORY;
		case G_FS_OPEN_INVALID_LINK:
			return G_FS_MUTATION_INVALID_LINK;
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
	if(directory ? found.node->type != G_FS_NODE_TYPE_FOLDER
				 : (found.node->type != G_FS_NODE_TYPE_FILE && found.node->type != G_FS_NODE_TYPE_SYMLINK))
		return directory ? G_FS_MUTATION_NOT_DIRECTORY : G_FS_MUTATION_INVALID_ARGUMENT;

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

static bool filesystemRenameIsFile(g_fs_node* node)
{
	return node->type == G_FS_NODE_TYPE_FILE || node->type == G_FS_NODE_TYPE_SYMLINK;
}

static g_fs_mutation_status filesystemRenameValidateDestination(g_fs_node* node, g_fs_delegate* delegate,
                                                               g_fs_node* parent, const char* name)
{
	if(!parent || !name || !*name || stringLength(name) >= G_FILENAME_MAX)
		return G_FS_MUTATION_INVALID_ARGUMENT;
	for(auto cursor = name; *cursor; ++cursor)
		if(*cursor == '/')
			return G_FS_MUTATION_NOT_FOUND;
	if(stringEquals(name, ".") || stringEquals(name, ".."))
		return G_FS_MUTATION_INVALID_ARGUMENT;

	if(parent->type != G_FS_NODE_TYPE_FOLDER && parent->type != G_FS_NODE_TYPE_MOUNTPOINT &&
	   parent->type != G_FS_NODE_TYPE_ROOT)
		return G_FS_MUTATION_INVALID_ARGUMENT;
	if(filesystemFindDelegate(parent) != delegate)
		return G_FS_MUTATION_CROSS_DEVICE;

	for(auto ancestor = parent; ancestor; ancestor = ancestor->parent)
		if(ancestor == node)
			return G_FS_MUTATION_INVALID_ARGUMENT;
	if(!delegate->rename)
		return G_FS_MUTATION_NOT_SUPPORTED;

	return G_FS_MUTATION_SUCCESSFUL;
}

/**
 * Publish a completed backend rename using the prepared name and directory entry.
 * The caller holds the delegate lock; cached nodes keep their existing identities.
 */
static void filesystemRenamePublish(g_fs_node* node, g_fs_node* parent, char* name,
                                    g_fs_node_entry* entry, g_fs_node* replaced)
{
	if(replaced)
		filesystemUnlinkNode(replaced);
	filesystemUnlinkNode(node);

	g_mutex_guard nodeGuard(&node->lock);
	auto oldName = node->name;
	node->name = name;
	node->type = filesystemStorageNodeType(node->type, name);
	node->parent = parent;
	memoryAtomicStore(&node->removed, false, G_MEMORY_ORDER_RELEASE);
	filesystemPublishNode(node, entry);
	parent->upToDate = false;
	heapFree(oldName);
}

/**
 * Move an existing cached node without changing its identity, so open descriptors
 * and cached descendants continue to refer to the same filesystem objects.
 */
g_fs_mutation_status filesystemRename(g_task* task, const char* source, const char* destination, bool replace)
{
	g_filesystem_find_result from{};
	auto status = filesystemMutationFind(task, source, &from);
	if(status != G_FS_OPEN_SUCCESSFUL)
		return filesystemMutationLookupStatus(status);

	auto node = from.node;
	if(!filesystemRenameIsFile(node) && node->type != G_FS_NODE_TYPE_FOLDER)
		return G_FS_MUTATION_BUSY;

	auto delegate = filesystemFindDelegate(node);
	g_mutex_guard renameGuard(&delegate->lock);
	g_filesystem_find_result to{};
	status = filesystemMutationFind(task, destination, &to);
	if(status != G_FS_OPEN_SUCCESSFUL && status != G_FS_OPEN_NOT_FOUND)
		return filesystemMutationLookupStatus(status);

	auto replaced = status == G_FS_OPEN_SUCCESSFUL ? to.node : nullptr;
	if(replaced == node)
		return replace ? G_FS_MUTATION_SUCCESSFUL : G_FS_MUTATION_ALREADY_EXISTS;
	if(replaced && (!replace || !filesystemRenameIsFile(node) || !filesystemRenameIsFile(replaced)))
		return G_FS_MUTATION_ALREADY_EXISTS;
	if(!replaced && !to.foundAllButLast)
		return G_FS_MUTATION_INVALID_ARGUMENT;

	auto parent = replaced ? replaced->parent : to.lastFoundParent;
	auto name = replaced ? replaced->name : to.fileNameStart;
	auto result = filesystemRenameValidateDestination(node, delegate, parent, name);
	if(result != G_FS_MUTATION_SUCCESSFUL)
		return result;

	auto copy = (char*) heapTryAllocate(stringLength(name) + 1);
	if(!copy)
		return G_FS_MUTATION_ERROR;

	auto entry = (g_fs_node_entry*) heapTryAllocate(sizeof(g_fs_node_entry));
	if(!entry)
	{
		heapFree(copy);
		return G_FS_MUTATION_ERROR;
	}
	stringCopy(copy, name);

	result = delegate->rename(node, parent, copy, replaced);
	if(result != G_FS_MUTATION_SUCCESSFUL)
	{
		heapFree(copy);
		heapFree(entry);
		return result;
	}

	filesystemRenamePublish(node, parent, copy, entry, replaced);
	return G_FS_MUTATION_SUCCESSFUL;
}
