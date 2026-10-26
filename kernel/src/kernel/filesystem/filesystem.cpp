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

#include "kernel/filesystem/filesystem.hpp"
#include "kernel/filesystem/filesystem_pipedelegate.hpp"
#include "kernel/filesystem/filesystem_process.hpp"
#include "kernel/filesystem/filesystem_ramdiskdelegate.hpp"
#include "kernel/ipc/pipes.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/system/mutex.hpp"
#include "kernel/panic.hpp"
#include "kernel/utils/string.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/tasking/clock.hpp"

static g_fs_node* filesystemRoot;
static g_fs_node* mountFolder;
static g_fs_node* pipesFolder;

static g_fs_virt_id filesystemNextNodeId;
static g_mutex filesystemNextNodeIdLock;

static g_hashmap<g_fs_virt_id, g_fs_node*>* filesystemNodes;

g_fs_open_status _filesystemChooseOrigin(const char* path, g_task* task, g_fs_node*& origin);

void filesystemInitialize()
{
	mutexInitializeTask(&filesystemNextNodeIdLock, __func__);
	filesystemNextNodeId = 0;

	filesystemNodes = hashmapCreateNumeric<g_fs_virt_id, g_fs_node*>(1024);

	filesystemProcessInitialize();
	filesystemCreateRoot();
}

void filesystemCreateRoot()
{
	// Mount ramdisk
	g_fs_delegate* ramdiskDelegate = filesystemCreateDelegate();
	ramdiskDelegate->open = filesystemRamdiskDelegateOpen;
	ramdiskDelegate->discover = filesystemRamdiskDelegateDiscover;
	ramdiskDelegate->read = filesystemRamdiskDelegateRead;
	ramdiskDelegate->write = filesystemRamdiskDelegateWrite;
	ramdiskDelegate->truncate = filesystemRamdiskDelegateTruncate;
	ramdiskDelegate->create = filesystemRamdiskDelegateCreate;
	ramdiskDelegate->getLength = filesystemRamdiskDelegateGetLength;
	ramdiskDelegate->close = filesystemRamdiskDelegateClose;
	ramdiskDelegate->refreshDir = filesystemRamdiskDelegateRefreshDir;

	filesystemRoot = filesystemCreateNode(G_FS_NODE_TYPE_ROOT, "root");
	filesystemRoot->delegate = ramdiskDelegate;

	mountFolder = filesystemCreateNode(G_FS_NODE_TYPE_FOLDER, "mount");
	mountFolder->delegate = filesystemCreateDelegate();
	filesystemAddChild(filesystemRoot, mountFolder);

	g_fs_node* ramdiskMountpoint = filesystemCreateNode(G_FS_NODE_TYPE_MOUNTPOINT, "ramdisk");
	ramdiskMountpoint->physicalId = 0;
	ramdiskMountpoint->delegate = ramdiskDelegate;
	filesystemAddChild(mountFolder, ramdiskMountpoint);

	// Mount pipes
	g_fs_delegate* pipeDelegate = filesystemCreateDelegate();
	pipeDelegate->open = filesystemPipeDelegateOpen;
	pipeDelegate->read = filesystemPipeDelegateRead;
	pipeDelegate->write = filesystemPipeDelegateWrite;
	pipeDelegate->truncate = filesystemPipeDelegateTruncate;
	pipeDelegate->getLength = filesystemPipeDelegateGetLength;
	pipeDelegate->waitForRead = filesystemPipeDelegateWaitForRead;
	pipeDelegate->waitForWrite = filesystemPipeDelegateWaitForWrite;
	pipeDelegate->close = filesystemPipeDelegateClose;

	pipesFolder = filesystemCreateNode(G_FS_NODE_TYPE_MOUNTPOINT, "pipes");
	pipesFolder->delegate = pipeDelegate;
	filesystemAddChild(mountFolder, pipesFolder);
}

g_fs_node* filesystemCreateNode(g_fs_node_type type, const char* name)
{
	g_fs_node* node = (g_fs_node*) heapAllocateClear(sizeof(g_fs_node));
	node->id = filesystemGetNextNodeId();
	node->type = type;
	node->name = stringDuplicate(name);
	node->parent = 0;
	node->children = 0;
	node->delegate = 0;
	node->blocking = false;
	node->upToDate = false;
	mutexInitializeTask(&node->lock, __func__);

	hashmapPut<g_fs_virt_id, g_fs_node*>(filesystemNodes, node->id, node);
	return node;
}

void filesystemAddChild(g_fs_node* parent, g_fs_node* child)
{
	mutexAcquire(&parent->lock);

	child->parent = parent;
	if(!child->delegate)
		child->delegate = parent->delegate;

	g_fs_node_entry* entry = (g_fs_node_entry*) heapAllocate(sizeof(g_fs_node_entry));
	entry->node = child;
	entry->next = parent->children;
	parent->children = entry;

	mutexRelease(&parent->lock);
}

g_fs_virt_id filesystemGetNextNodeId()
{
	mutexAcquire(&filesystemNextNodeIdLock);
	g_fs_virt_id nextId = filesystemNextNodeId++;
	mutexRelease(&filesystemNextNodeIdLock);
	return nextId;
}

g_fs_node* filesystemGetNode(g_fs_virt_id id)
{
	return hashmapGet<g_fs_virt_id, g_fs_node*>(filesystemNodes, id, 0);
}

void filesystemRemovePipeNode(g_fs_phys_id pipe)
{
	mutexAcquire(&pipesFolder->lock);
	auto link = &pipesFolder->children;
	while(*link)
	{
		auto entry = *link;
		auto node = entry->node;
		if(node->physicalId == pipe)
		{
			*link = entry->next;
			hashmapRemove(filesystemNodes, node->id);
			heapFree(node->name);
			heapFree(node);
			heapFree(entry);
			break;
		}
		link = &entry->next;
	}
	mutexRelease(&pipesFolder->lock);
}

g_fs_node* filesystemGetRoot()
{
	return filesystemRoot;
}

g_fs_delegate* filesystemCreateDelegate()
{
	g_fs_delegate* delegate = (g_fs_delegate*) heapAllocateClear(sizeof(g_fs_delegate));
	mutexInitializeTask(&delegate->lock, __func__);
	return delegate;
}

g_fs_delegate* filesystemFindDelegate(g_fs_node* node)
{
	if(!node)
		panic("%! tried to find delegate for null node", "fs");

	if(!node->delegate && node->parent)
		return filesystemFindDelegate(node->parent);

	g_fs_delegate* delegate = node->delegate;
	if(delegate == 0)
		panic("%! failed to find delegate for node %i", "fs", node->id);
	return delegate;
}

bool filesystemFindExistingChild(g_fs_node* parent, const char* name, g_fs_node** outChild)
{
	mutexAcquire(&parent->lock);

	g_fs_node* child = nullptr;

	if(stringEquals(name, ".."))
	{
		child = parent->parent;
	}
	else if(stringEquals(name, "."))
	{
		child = parent;
	}
	else
	{
		g_fs_node_entry* childEntry = parent->children;
		while(childEntry)
		{
			if(stringEquals(name, childEntry->node->name))
			{
				child = childEntry->node;
				break;
			}
			childEntry = childEntry->next;
		}
	}

	mutexRelease(&parent->lock);

	if(outChild)
		*outChild = child;
	return child != nullptr;
}

g_fs_open_status filesystemFindChild(g_fs_node* parent, const char* name, g_fs_node** outChild)
{
	if(filesystemFindExistingChild(parent, name, outChild))
		return G_FS_OPEN_SUCCESSFUL;

	g_fs_delegate* delegate = filesystemFindDelegate(parent);
	if(!delegate->discover)
	{
		*outChild = 0;
		return G_FS_OPEN_ERROR;
	}
	return delegate->discover(parent, name, outChild);
}

g_filesystem_find_result filesystemFind(g_fs_node* parent, const char* path)
{
	if(parent == nullptr)
		parent = filesystemRoot;

	g_fs_node* node = parent;
	g_fs_node* lastFoundParent = node;
	g_fs_open_status status = G_FS_OPEN_SUCCESSFUL;

	char* buf = (char*) heapAllocate(G_PATH_MAX); // TODO SLOW
	stringCopy(buf, path);

	char* nameStart = buf;
	char* nameEnd = nameStart;

	while(nameStart)
	{
		// Find where next name starts and ends
		while(*nameStart == '/')
			++nameStart;

		nameEnd = nameStart;
		while(*nameEnd && *nameEnd != '/')
			++nameEnd;

		// Nothing left to resolve, we found the node
		int remaining = nameEnd - nameStart;
		if(remaining == 0)
			break;

		if(remaining > G_FILENAME_MAX)
		{
			status = G_FS_OPEN_ERROR;
			logInfo("%! tried to resolve path with filename (%i) longer than max (%i): %s", "fs", remaining,
			        G_FILENAME_MAX, path);
			break;
		}

		// Temporary modify our path buffer
		char clobberedChar = nameStart[remaining];
		nameStart[remaining] = 0;

		// Find child with this name
		lastFoundParent = node;
		status = filesystemFindChild(node, nameStart, &node);

		// Unclobber
		nameStart[remaining] = clobberedChar;

		if(status != G_FS_OPEN_SUCCESSFUL)
			break;

		nameStart = nameEnd;
	}
	heapFree(buf);

	return {
			status: status,
			node: node,
			foundAllButLast: ((nameEnd - nameStart) > 0),
			lastFoundParent: lastFoundParent,
			fileNameStart: path + (nameStart - buf)
	};
}

g_fs_open_status filesystemOpen(const char* path, g_file_flag_mode flags, g_task* task, g_fd* outFd)
{
	g_fs_node* origin = nullptr;
	g_fs_open_status findOriginStatus = _filesystemChooseOrigin(path, task, origin);
	if(findOriginStatus != G_FS_OPEN_SUCCESSFUL)
		return findOriginStatus;

	// Try to find existing node
	auto findRes = filesystemFind(origin, path);

	// Handle different open cases
	if(findRes.status == G_FS_OPEN_SUCCESSFUL)
	{
		if(findRes.node->type == G_FS_NODE_TYPE_FOLDER)
		{
			logInfo("%! tried to open folder", "fs");
			return G_FS_OPEN_ERROR;
		}

		if(flags & G_FILE_FLAG_MODE_TRUNCATE)
		{
			if(filesystemTruncate(findRes.node) != G_FS_OPEN_SUCCESSFUL)
			{
				logInfo("%! failed to truncate file %i", "fs", findRes.node->id);
				return G_FS_OPEN_ERROR;
			}
		}
	}
	else if(findRes.status == G_FS_OPEN_NOT_FOUND)
	{
		if(flags & G_FILE_FLAG_MODE_CREATE)
		{
			if(!findRes.foundAllButLast)
			{
				logInfo("%! failed to create file '%s' in parent %i because folders do not exist", "fs", path,
				        origin->id);
				return G_FS_OPEN_ERROR;
			}
			else if(filesystemCreateFile(findRes.lastFoundParent, findRes.fileNameStart, &findRes.node) !=
			        G_FS_OPEN_SUCCESSFUL)
			{
				logInfo("%! failed to create file '%s' in parent %i", "fs", path, origin->id);
				return G_FS_OPEN_ERROR;
			}
		}
		else
		{
			return G_FS_OPEN_NOT_FOUND;
		}
	}
	else
	{
		return findRes.status;
	}

	// Actually open the file
	return filesystemOpenNodeFd(findRes.node, flags, task->process->id, outFd);
}

g_fs_open_status filesystemOpenNode(g_fs_node* file, g_file_flag_mode flags, g_pid process,
                                    g_file_descriptor** outDescriptor, g_fd optionalTargetFd, uint64_t initialOffset)
{
	g_fs_delegate* delegate = filesystemFindDelegate(file);
	if(!delegate->open)
	{
		logInfo("%! failed to open file %i, delegate had no implementation", "fs", file->id);
		return G_FS_OPEN_ERROR;
	}

	g_fs_open_status status = delegate->open(file, flags);
	if(status == G_FS_OPEN_SUCCESSFUL && process != G_PID_NONE)
	{
		status = filesystemProcessCreateDescriptor(process, file->id, flags, outDescriptor, optionalTargetFd, initialOffset);
        if(status != G_FS_OPEN_SUCCESSFUL && delegate->close)
            delegate->close(file, flags);
	}
	return status;
}

g_fs_open_status filesystemOpenNodeFd(g_fs_node* file, g_file_flag_mode flags, g_pid process, g_fd* outFd,
                                      g_fd optionalTargetFd)
{
	g_file_descriptor* descriptor = nullptr;
	auto status = filesystemOpenNode(file, flags, process, &descriptor, optionalTargetFd);
	if(descriptor)
    {
        *outFd = descriptor->id;
        filesystemProcessReleaseDescriptor(descriptor);
    }
	return status;
}

static g_fs_read_status filesystemReadDescriptor(g_file_descriptor* descriptor, uint8_t* buffer,
                                                uint64_t length, int64_t* outRead)
{
	g_mutex_guard lock(&descriptor->operationLock);
	auto status = filesystemRead(descriptor->node, buffer, descriptor->offset, length, outRead);
	if(*outRead > 0)
		descriptor->offset += *outRead;
	return status;
}

g_fs_read_status filesystemRead(g_task* task, g_fd fd, uint8_t* buffer, uint64_t length, int64_t* outRead)
{
	uint64_t generation = 0;
	int64_t bytesRead = -1;
	g_fs_read_status status = G_FS_READ_INVALID_FD;

	while(true)
	{
		bool retryRead = false;
		taskingWaitIf([&]()
		{
			auto descriptor = filesystemProcessAcquireDescriptor(task->process->id, fd, generation);
			if(!descriptor)
			{
				status = G_FS_READ_INVALID_FD;
				bytesRead = -1;
				return false;
			}

			generation = descriptor->generation;
			waitQueueRemove(&descriptor->waiters, task->id);
			status = filesystemReadDescriptor(descriptor, buffer, length, &bytesRead);
			if(status != G_FS_READ_BUSY || !(descriptor->openFlags & G_FILE_FLAG_MODE_BLOCKING))
			{
				filesystemProcessReleaseDescriptor(descriptor);
				return false;
			}

			auto delegate = filesystemFindDelegate(descriptor->node);
			if(!delegate->waitForRead)
			{
				status = G_FS_READ_ERROR;
				filesystemProcessReleaseDescriptor(descriptor);
				return false;
			}

			// Retry even if readiness changed and no sleep is needed.
			retryRead = true;
			bool shouldWait = delegate->waitForRead(task, descriptor->node, length);
			if(shouldWait)
				filesystemProcessRegisterDescriptorWait(descriptor, task);
			filesystemProcessReleaseDescriptor(descriptor);
			return shouldWait;
		}, [task]() { waitQueueRemoveTask(task->id); });

		if(!retryRead)
		{
			*outRead = bytesRead;
			return status;
		}

		// The descriptor reference and operation lock are gone before cancellation.
		taskingHandleTerminationRequest();
	}
}

g_fs_read_status filesystemRead(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outRead)
{
	g_fs_delegate* delegate = filesystemFindDelegate(node);
	if(!delegate->read)
		return G_FS_READ_ERROR;

	return delegate->read(node, buffer, offset, length, outRead);
}

g_fs_length_status filesystemGetLength(g_task* task, g_fd fd, uint64_t* outLength)
{
	auto descriptor = filesystemProcessAcquireDescriptor(task->process->id, fd);
	if(!descriptor)
	{
		return G_FS_LENGTH_INVALID_FD;
	}

	g_fs_node* node = descriptor->node;
	if(!node)
	{
		filesystemProcessReleaseDescriptor(descriptor);
		return G_FS_LENGTH_INVALID_FD;
	}

	auto status = filesystemGetLength(node, outLength);
	filesystemProcessReleaseDescriptor(descriptor);
	return status;
}

g_fs_length_status filesystemGetLength(g_fs_node* node, uint64_t* outLength)
{
	g_fs_delegate* delegate = filesystemFindDelegate(node);
	if(!delegate->getLength)
		return G_FS_LENGTH_ERROR;

	return delegate->getLength(node, outLength);
}

static g_fs_write_status filesystemWriteDescriptor(g_file_descriptor* descriptor, uint8_t* buffer,
                                                  uint64_t length, int64_t* outWrote)
{
	g_mutex_guard lock(&descriptor->operationLock);
	uint64_t startOffset = descriptor->offset;
	if((descriptor->openFlags & G_FILE_FLAG_MODE_APPEND) &&
	   filesystemGetLength(descriptor->node, &startOffset) != G_FS_LENGTH_SUCCESSFUL)
	{
		return G_FS_WRITE_ERROR;
	}

	auto status = filesystemWrite(descriptor->node, buffer, startOffset, length, outWrote);
	if(*outWrote > 0)
		descriptor->offset = startOffset + *outWrote;
	return status;
}

g_fs_write_status filesystemWrite(g_task* task, g_fd fd, uint8_t* buffer, uint64_t length, int64_t* outWrote)
{
	uint64_t generation = 0;
	int64_t bytesWritten = -1;
	g_fs_write_status status = G_FS_WRITE_INVALID_FD;

	while(true)
	{
		bool retryWrite = false;
		taskingWaitIf([&]()
		{
			auto descriptor = filesystemProcessAcquireDescriptor(task->process->id, fd, generation);
			if(!descriptor)
			{
				status = G_FS_WRITE_INVALID_FD;
				bytesWritten = -1;
				return false;
			}

			generation = descriptor->generation;
			waitQueueRemove(&descriptor->waiters, task->id);
			status = filesystemWriteDescriptor(descriptor, buffer, length, &bytesWritten);
			if(status != G_FS_WRITE_BUSY || !(descriptor->openFlags & G_FILE_FLAG_MODE_BLOCKING))
			{
				filesystemProcessReleaseDescriptor(descriptor);
				return false;
			}

			auto delegate = filesystemFindDelegate(descriptor->node);
			if(!delegate->waitForWrite)
			{
				status = G_FS_WRITE_ERROR;
				filesystemProcessReleaseDescriptor(descriptor);
				return false;
			}

			retryWrite = true;
			bool shouldWait = delegate->waitForWrite(task, descriptor->node, length);
			if(shouldWait)
				filesystemProcessRegisterDescriptorWait(descriptor, task);
			filesystemProcessReleaseDescriptor(descriptor);
			return shouldWait;
		}, [task]() { waitQueueRemoveTask(task->id); });

		if(!retryWrite)
		{
			*outWrote = bytesWritten;
			return status;
		}

		// The descriptor reference and operation lock are gone before cancellation.
		taskingHandleTerminationRequest();
	}
}

g_fs_write_status filesystemWrite(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outWrote)
{
	g_fs_delegate* delegate = filesystemFindDelegate(node);
	if(!delegate->write)
		return G_FS_WRITE_ERROR;

	return delegate->write(node, buffer, offset, length, outWrote);
}

g_fs_open_status filesystemCreateFile(g_fs_node* parent, const char* name, g_fs_node** outFile)
{
	g_fs_delegate* delegate = filesystemFindDelegate(parent);
	if(!delegate->create)
		return G_FS_OPEN_ERROR;

	return delegate->create(parent, name, outFile);
}

g_fs_open_status filesystemTruncate(g_fs_node* file)
{
	g_fs_delegate* delegate = filesystemFindDelegate(file);
	if(!delegate->truncate)
		return G_FS_OPEN_ERROR;

	return delegate->truncate(file);
}

g_fs_pipe_status filesystemCreatePipe(g_bool blocking, g_fs_node** outPipeNode)
{
	g_fs_phys_id pipeId;
	g_fs_pipe_status status = pipeCreate(&pipeId, blocking);
	if(status != G_FS_PIPE_SUCCESSFUL)
	{
		logInfo("%! failed to create pipe with status %i", "fs", status);
		return status;
	}

	char pipeName[128];
	const char* pipePrefix = "pipe";
	int pipePrefixLen = stringLength(pipePrefix);
	memoryCopy(pipeName, pipePrefix, pipePrefixLen);
	char* numberEnd = stringWriteNumber(&pipeName[pipePrefixLen], pipeId);
	*numberEnd = 0;

	g_fs_node* pipeNode = filesystemCreateNode(G_FS_NODE_TYPE_PIPE, pipeName);
	pipeNode->physicalId = pipeId;
	pipeNode->blocking = blocking;
	filesystemAddChild(pipesFolder, pipeNode);
	*outPipeNode = pipeNode;
	return G_FS_PIPE_SUCCESSFUL;
}

g_fs_close_status filesystemClose(g_pid pid, g_fd fd, g_bool removeDescriptor)
{
    // Closing always invalidates the number immediately. The delegate's final
    // close is deferred until operations that already pinned it have completed.
    return filesystemProcessCloseDescriptor(pid, fd);
}

g_fs_seek_status filesystemSeek(g_task* task, g_fd fd, g_fs_seek_mode mode, int64_t amount, int64_t* outResult)
{
	auto descriptor = filesystemProcessAcquireDescriptor(task->process->id, fd);
	if(!descriptor)
	{
		return G_FS_SEEK_INVALID_FD;
	}

	g_fs_node* node = descriptor->node;
	if(!node)
	{
		filesystemProcessReleaseDescriptor(descriptor);
		return G_FS_SEEK_INVALID_FD;
	}

	uint64_t length;
	if(filesystemGetLength(node, &length) != G_FS_LENGTH_SUCCESSFUL)
	{
		logInfo("%! failed to seek in file %i, could not get length", "fs", node->id);
		filesystemProcessReleaseDescriptor(descriptor);
		return G_FS_SEEK_ERROR;
	}

	mutexAcquire(&descriptor->operationLock);
	// add amount to offset
	if(mode == G_FS_SEEK_CUR)
	{
		descriptor->offset += amount;
	}
	else if(mode == G_FS_SEEK_SET)
	{
		descriptor->offset = amount;
	}
	else if(mode == G_FS_SEEK_END)
	{
		descriptor->offset = length - amount;
	}

	// validate offset
	if(descriptor->offset > length)
	{
		descriptor->offset = length;
	}
	if(descriptor->offset < 0)
	{
		descriptor->offset = 0;
	}

	*outResult = descriptor->offset;
	mutexRelease(&descriptor->operationLock);
	filesystemProcessReleaseDescriptor(descriptor);
	return G_FS_SEEK_SUCCESSFUL;
}

int filesystemGetAbsolutePathLength(g_fs_node* node)
{
	if(node->type == G_FS_NODE_TYPE_ROOT)
	{
		return 1;
	}

	int length = 0;
	if(node->parent)
	{
		length = filesystemGetAbsolutePathLength(node->parent);
	}

	int calced;
	if(length == 1)
	{
		calced = stringLength(node->name);
	}
	else
	{
		calced = stringLength(node->name) + 1;
	}
	return length + calced;
}

int filesystemGetAbsolutePath(g_fs_node* node, char* buffer)
{
	if(node->type == G_FS_NODE_TYPE_ROOT)
	{
		stringCopy(buffer, "/");
		return 1;
	}

	int length = 0;
	if(node->parent)
	{
		length = filesystemGetAbsolutePath(node->parent, buffer);
	}

	int copied;
	if(length == 1)
	{
		copied = stringCopy(&buffer[length], node->name);
	}
	else
	{
		buffer[length] = '/';
		copied = stringCopy(&buffer[length + 1], node->name) + 1;
	}
	return length + copied;
}

g_fs_open_directory_status filesystemOpenDirectory(g_fs_node* dir)
{
	if(!(dir->type == G_FS_NODE_TYPE_FOLDER || dir->type == G_FS_NODE_TYPE_MOUNTPOINT || dir->type ==
	     G_FS_NODE_TYPE_ROOT))
		return G_FS_OPEN_DIRECTORY_ERROR;

	if(dir->upToDate)
		return G_FS_OPEN_DIRECTORY_SUCCESSFUL;

	g_fs_delegate* delegate = filesystemFindDelegate(dir);
	if(!delegate->refreshDir)
	{
		logDebug("%! failed to open directory %i, delegate had no implementation", "fs", dir->id);
		return G_FS_OPEN_DIRECTORY_SUCCESSFUL;
	}

	auto refreshStatus = delegate->refreshDir(dir);
	if(refreshStatus == G_FS_DIRECTORY_REFRESH_SUCCESSFUL)
	{
		dir->upToDate = true;
		return G_FS_OPEN_DIRECTORY_SUCCESSFUL;
	}

	return G_FS_OPEN_DIRECTORY_ERROR;
}

g_fs_read_directory_status filesystemReadDirectory(g_fs_node* dir, uint32_t index, g_fs_node** outChild)
{
	if(!dir->children)
		return G_FS_READ_DIRECTORY_ERROR;

	auto entry = dir->children;
	while(entry)
	{
		if(!index)
		{
			*outChild = entry->node;
			return G_FS_READ_DIRECTORY_SUCCESSFUL;
		}

		--index;
		entry = entry->next;
	}
	return G_FS_READ_DIRECTORY_EOD;
}

bool filesystemReadToMemory(g_fd fd, size_t offset, uint8_t* buffer, uint64_t len)
{
	auto task = taskingGetCurrentTask();
	auto descriptor = filesystemProcessAcquireDescriptor(task->process->id, fd);
	if(!descriptor)
	{
		return false;
	}
	auto node = descriptor->node;
	if(!node)
	{
		filesystemProcessReleaseDescriptor(descriptor);
		return false;
	}
	uint64_t fileLength;
	if(filesystemGetLength(node, &fileLength) != G_FS_LENGTH_SUCCESSFUL ||
	   offset > fileLength || len > fileLength - offset)
	{
		filesystemProcessReleaseDescriptor(descriptor);
		return false;
	}

	// ELF headers and demand-loaded library pages use explicit offsets. A
	// seek/read pair races with other faults sharing this process's descriptor.
	uint64_t remain = len;
	while(remain)
	{
		int64_t read;
		auto stat = filesystemRead(node, &buffer[len - remain], offset + len - remain, remain, &read);
		if(stat != G_FS_READ_SUCCESSFUL || read <= 0 || (uint64_t) read > remain)
		{
			logInfo("%! failed to read file from fd %i (status %i), remaining: %i", "fs", fd, stat, (uint32_t) remain);
			filesystemProcessReleaseDescriptor(descriptor);
			return false;
		}
		remain -= read;
	}
	filesystemProcessReleaseDescriptor(descriptor);
	return true;
}

bool filesystemGetFileName(g_fd fd, const char** outName)
{
	auto descriptor = filesystemProcessAcquireDescriptor(taskingGetCurrentTask()->process->id, fd);
	if(!descriptor)
	{
		return false;
	}

	auto node = descriptor->node;
	if(!node)
	{
		filesystemProcessReleaseDescriptor(descriptor);
		return false;
	}

	*outName = stringDuplicate(node->name);
	filesystemProcessReleaseDescriptor(descriptor);
	return true;
}

g_fs_real_path_status filesystemRealPath(g_task* task, const char* in, char* out)
{
	g_fs_node* origin;
	auto findOriginStatus = _filesystemChooseOrigin(in, task, origin);
	if(findOriginStatus != G_FS_OPEN_SUCCESSFUL)
		return findOriginStatus;

	auto findResult = filesystemFind(origin, in);
	if(findResult.status == G_FS_OPEN_NOT_FOUND)
		return G_FS_REAL_PATH_NOT_FOUND;
	if(findResult.status != G_FS_OPEN_SUCCESSFUL)
		return G_FS_REAL_PATH_ERROR;

	filesystemGetAbsolutePath(findResult.node, out);
	return G_FS_REAL_PATH_SUCCESS;
}


g_fs_open_status _filesystemChooseOrigin(const char* path, g_task* task, g_fs_node*& origin)
{
	if(path[0] == '/')
	{
		origin = filesystemGetRoot();
	}
	else
	{
		mutexAcquire(&task->process->lock);
		auto current = task->process->environment.workingDirectory;
		auto cwd = stringDuplicate(current ? current : "/");
		mutexRelease(&task->process->lock);

		origin = nullptr;
		auto findCwdRes = filesystemFind(nullptr, cwd);
		heapFree(cwd);
		if(findCwdRes.status == G_FS_OPEN_SUCCESSFUL)
			origin = findCwdRes.node;
		else
			return findCwdRes.status;
	}

	return G_FS_OPEN_SUCCESSFUL;
}

g_fs_stat_status filesystemStat(g_task* task, const char* path, g_fs_stat_data* out)
{

	g_fs_node* origin;
	g_fs_open_status findOriginStatus = _filesystemChooseOrigin(path, task, origin);
	if(findOriginStatus != G_FS_OPEN_SUCCESSFUL)
		return findOriginStatus;

	auto findRes = filesystemFind(origin, path);
	if(findRes.status == G_FS_OPEN_NOT_FOUND)
		return G_FS_STAT_NOT_FOUND;
	if(findRes.status != G_FS_OPEN_SUCCESSFUL)
		return G_FS_STAT_ERROR;

	return filesystemStatNode(findRes.node, out);
}

g_fs_stat_status filesystemFstat(g_task* task, g_fd fd, g_fs_stat_data* out)
{
	auto descriptor = filesystemProcessAcquireDescriptor(task->process->id, fd);
	if(!descriptor)
	{
		return G_FS_STAT_INVALID_FD;
	}

	g_fs_node* node = descriptor->node;
	if(!node)
	{
		filesystemProcessReleaseDescriptor(descriptor);
		return G_FS_STAT_INVALID_FD;
	}

	auto status = filesystemStatNode(node, out);
	filesystemProcessReleaseDescriptor(descriptor);
	return status;
}

g_fs_stat_status filesystemStatNode(g_fs_node* node, g_fs_stat_data* out)
{
	out->virtual_id = node->id;
	out->type = node->type;

	auto lengthStatus = filesystemGetLength(node, &out->size);
	if(lengthStatus != G_FS_LENGTH_SUCCESSFUL)
	{
		logInfo("%! failed to stat file %i with get-length error %i", "filesystem", node->id, lengthStatus);
		out->size = -1;
	}

	// TODO
	out->device = -1;
	out->time_last_access = 0;
	out->time_last_modification = 0;
	out->time_creation = 0;

	return G_FS_STAT_SUCCESS;
}
