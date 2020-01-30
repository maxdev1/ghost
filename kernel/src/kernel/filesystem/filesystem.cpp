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
#include "kernel/filesystem/filesystem_process.hpp"
#include "kernel/filesystem/filesystem_ramdiskdelegate.hpp"
#include "kernel/filesystem/filesystem_pipedelegate.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/tasking/wait.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/ipc/pipes.hpp"
#include "kernel/kernel.hpp"

#include "shared/system/mutex.hpp"
#include "shared/utils/string.hpp"

static g_fs_node* filesystemRoot;
static g_fs_node* mountFolder;
static g_fs_node* pipesFolder;

static g_fs_virt_id filesystemNextNodeId;
static g_mutex filesystemNextNodeIdLock;

static g_hashmap<g_fs_virt_id, g_fs_node*>* filesystemNodes;

void filesystemInitialize()
{
	mutexInitialize(&filesystemNextNodeIdLock);
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

	filesystemRoot = filesystemCreateNode(G_FS_NODE_TYPE_ROOT, "root");
	filesystemRoot->delegate = ramdiskDelegate;

	mountFolder = filesystemCreateNode(G_FS_NODE_TYPE_FOLDER, "mount");
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
	pipeDelegate->waitResolverRead = filesystemPipeDelegateWaitResolverRead;
	pipeDelegate->waitResolverWrite = filesystemPipeDelegateWaitResolverWrite;
	pipeDelegate->close = filesystemPipeDelegateClose;

	pipesFolder = filesystemCreateNode(G_FS_NODE_TYPE_FOLDER, "pipes");
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

	hashmapPut<g_fs_virt_id, g_fs_node*>(filesystemNodes, node->id, node);
	return node;
}

void filesystemAddChild(g_fs_node* parent, g_fs_node* child)
{
	g_fs_delegate* delegate = filesystemFindDelegate(parent);
	mutexAcquire(&delegate->lock);

	child->parent = parent;

	g_fs_node_entry* entry = (g_fs_node_entry*) heapAllocate(sizeof(g_fs_node_entry));
	entry->node = child;
	entry->next = child->children;
	child->children = entry;

	mutexRelease(&delegate->lock);
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

g_fs_node* filesystemGetRoot()
{
	return filesystemRoot;
}

g_fs_delegate* filesystemCreateDelegate()
{
	g_fs_delegate* delegate = (g_fs_delegate*) heapAllocateClear(sizeof(g_fs_delegate));
	mutexInitialize(&delegate->lock);
	return delegate;
}

g_fs_delegate* filesystemFindDelegate(g_fs_node* node)
{
	if(!node)
		kernelPanic("%! tried to find delegate for null node", "fs");

	if(!node->delegate && node->parent)
		return filesystemFindDelegate(node->parent);

	g_fs_delegate* delegate = node->delegate;
	if(delegate == 0)
		kernelPanic("%! failed to find delegate for node %i", "fs", node->id);
	return delegate;
}

g_fs_open_status filesystemFindChild(g_fs_node* parent, const char* name, g_fs_node** outChild)
{
	if(stringEquals(name, ".."))
	{
		*outChild = parent->parent;
		return G_FS_OPEN_SUCCESSFUL;
	}

	if(stringEquals(name, "."))
	{
		*outChild = parent;
		return G_FS_OPEN_SUCCESSFUL;
	}

	g_fs_node_entry* child = parent->children;
	g_fs_node* lastKnown = parent;
	while(child)
	{
		if(stringEquals(name, child->node->name))
		{
			*outChild = child->node;
			return G_FS_OPEN_SUCCESSFUL;
		}
		lastKnown = child->node;
		child = child->next;
	}

	g_fs_delegate* delegate = filesystemFindDelegate(lastKnown);
	if(!delegate->discover)
	{
		*outChild = 0;
		return G_FS_OPEN_ERROR;
	}
	return delegate->discover(lastKnown, name, outChild);
}

g_fs_open_status filesystemFind(g_fs_node* parent, const char* path, g_fs_node** outChild, bool* outFoundAllButLast, g_fs_node** outLastFoundParent,
		const char** outFileNameStart)
{
	if(parent == 0)
	{
		parent = filesystemRoot;
	}
	char* nameBuf = (char*) heapAllocate(sizeof(char) * (G_FILENAME_MAX + 1));

	const char* nameStart = path;
	const char* nameEnd = nameStart;
	g_fs_node* node = parent;
	g_fs_node* lastFoundParent = node;
	g_fs_open_status status = G_FS_OPEN_SUCCESSFUL;

	while(nameStart)
	{
		// Find where next name starts and ends
		while(*nameStart == '/')
			++nameStart;

		nameEnd = nameStart;
		while(*nameEnd && *nameEnd != '/')
			++nameEnd;

		// Nothing left to resolve, we found the node
		int nameLen = nameEnd - nameStart;
		if(nameLen == 0)
			break;

		if(nameLen > G_FILENAME_MAX)
		{
			logInfo("%! tried to resolve path with filename (%i) longer than max (%i): %s", "fs", nameLen, G_FILENAME_MAX, path);
			heapFree(nameBuf);
			return 0;
		}

		// Copy name into temporary buffer
		memoryCopy(nameBuf, nameStart, nameLen);
		nameBuf[nameLen] = 0;

		// Find child with this name
		lastFoundParent = node;
		status = filesystemFindChild(node, nameBuf, &node);
		if(status != G_FS_OPEN_SUCCESSFUL)
			break;

		nameStart = nameEnd;
	}

	// Provide output values if requested
	if(outFoundAllButLast)
		*outFoundAllButLast = ((nameEnd - nameStart) > 0);
	if(outLastFoundParent)
		*outLastFoundParent = lastFoundParent;
	if(outFileNameStart)
		*outFileNameStart = nameStart;

	heapFree(nameBuf);
	*outChild = node;
	return status;
}

g_fs_open_status filesystemOpen(const char* path, g_file_flag_mode flags, g_task* task, g_fd* outFd)
{
	// Decide for relative path origin
	g_fs_node* relative = 0;
	if(path[0] != '/')
	{
		const char* workingDirectoryPath = task->process->environment.workingDirectory;
		if(workingDirectoryPath == 0)
		{
			workingDirectoryPath = "/";
		}

		filesystemFind(0, workingDirectoryPath, &relative);
	}
	if(!relative)
		relative = filesystemGetRoot();

	// Try to find existing node
	g_fs_node* file;
	bool foundAllButLast;
	g_fs_node* lastFoundParent;
	const char* filenameStart;
	g_fs_open_status status = filesystemFind(relative, path, &file, &foundAllButLast, &lastFoundParent, &filenameStart);

	// Handle different open cases
	if(status == G_FS_OPEN_SUCCESSFUL)
	{
		if(flags & G_FILE_FLAG_MODE_TRUNCATE)
		{
			if(filesystemTruncate(file) != G_FS_OPEN_SUCCESSFUL)
			{
				logInfo("%! failed to truncate file %i", "fs", file->id);
				return G_FS_OPEN_ERROR;
			}
		}
	} else if(status == G_FS_OPEN_NOT_FOUND)
	{
		if(flags & G_FILE_FLAG_MODE_CREATE)
		{
			if(!foundAllButLast)
			{
				logInfo("%! failed to create file '%s' in parent %i because folders do not exist", "fs", path, relative->id);
				return G_FS_OPEN_ERROR;
			} else if(filesystemCreateFile(lastFoundParent, filenameStart, &file) != G_FS_OPEN_SUCCESSFUL)
			{
				logInfo("%! failed to create file '%s' in parent %i", "fs", path, relative->id);
				return G_FS_OPEN_ERROR;
			}
		} else
		{
			return G_FS_OPEN_NOT_FOUND;
		}
	} else
	{
		return status;
	}

	// Actually open the file
	return filesystemOpen(file, flags, task, outFd);
}

g_fs_open_status filesystemOpen(g_fs_node* file, g_file_flag_mode flags, g_task* task, g_fd* outFd)
{
	g_fs_delegate* delegate = filesystemFindDelegate(file);
	if(!delegate->open)
		kernelPanic("%! failed to open file %i, delegate had no implementation", "fs", file->id);

	g_fs_open_status status = delegate->open(file);
	if(status == G_FS_OPEN_SUCCESSFUL)
	{
		g_file_descriptor* descriptor;
		filesystemProcessCreateDescriptor(task->process->id, file->id, flags, &descriptor);
		*outFd = descriptor->id;
	}
	return status;
}

g_fs_read_status filesystemRead(g_task* task, g_fd fd, uint8_t* buffer, uint64_t length, int64_t* outRead)
{
	g_file_descriptor* descriptor = filesystemProcessGetDescriptor(task->process->id, fd);
	if(!descriptor)
	{
		return G_FS_READ_INVALID_FD;
	}

	g_fs_node* node = filesystemGetNode(descriptor->nodeId);
	if(!node)
	{
		return G_FS_READ_INVALID_FD;
	}

	int64_t read;
	g_fs_read_status status;
	while((status = filesystemRead(node, buffer, descriptor->offset, length, &read)) == G_FS_READ_BUSY && node->blocking)
	{
		filesystemWaitToRead(task, node);
		taskingKernelThreadYield();
	}
	if(read > 0)
	{
		descriptor->offset += read;
	}
	*outRead = read;
	return status;
}

g_fs_read_status filesystemRead(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outRead)
{
	g_fs_delegate* delegate = filesystemFindDelegate(node);
	if(!delegate->read)
		return G_FS_READ_ERROR;

	return delegate->read(node, buffer, offset, length, outRead);
}

g_fs_length_status filesystemGetLength(g_fs_node* node, uint64_t* outLength)
{
	g_fs_delegate* delegate = filesystemFindDelegate(node);
	if(!delegate->getLength)
		return G_FS_LENGTH_ERROR;

	return delegate->getLength(node, outLength);
}

g_fs_write_status filesystemWrite(g_task* task, g_fd fd, uint8_t* buffer, uint64_t length, int64_t* outWrote)
{
	g_file_descriptor* descriptor = filesystemProcessGetDescriptor(task->process->id, fd);
	if(!descriptor)
	{
		return G_FS_WRITE_INVALID_FD;
	}

	g_fs_node* node = filesystemGetNode(descriptor->nodeId);
	if(!node)
	{
		return G_FS_WRITE_INVALID_FD;
	}

	uint64_t startOffset = descriptor->offset;
	if(descriptor->openFlags & G_FILE_FLAG_MODE_APPEND)
	{
		if(filesystemGetLength(node, &startOffset) != G_FS_LENGTH_SUCCESSFUL)
		{
			logInfo("%! failed to append to file %i, could not get length", "fs", node->id);
			return G_FS_WRITE_ERROR;
		}
	}

	int64_t wrote;
	g_fs_write_status status;
	while((status = filesystemWrite(node, buffer, startOffset, length, &wrote)) == G_FS_WRITE_BUSY && node->blocking)
	{
		filesystemWaitToWrite(task, node);
		taskingKernelThreadYield();
	}
	if(wrote > 0)
	{
		descriptor->offset = startOffset + wrote;
	}
	*outWrote = wrote;
	return status;
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

void filesystemWaitToWrite(g_task* task, g_fs_node* file)
{
	g_fs_delegate* delegate = filesystemFindDelegate(file);
	if(!delegate->waitResolverWrite)
		kernelPanic("%! task %i tried to wait for file %i but delegate didn't provide wait resolver", "filesytem", task->id, file->id);

	waitForFile(task, file, delegate->waitResolverWrite);
}

void filesystemWaitToRead(g_task* task, g_fs_node* file)
{
	g_fs_delegate* delegate = filesystemFindDelegate(file);
	if(!delegate->waitResolverRead)
		kernelPanic("%! task %i tried to wait for file %i but delegate didn't provide wait resolver", "filesytem", task->id, file->id);

	waitForFile(task, file, delegate->waitResolverRead);
}

g_fs_pipe_status filesystemCreatePipe(g_bool blocking, g_fs_node** outPipeNode)
{
	g_fs_phys_id pipeId;
	g_fs_pipe_status status = pipeCreate(&pipeId);
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
	pipeNode->blocking = true;
	filesystemAddChild(pipesFolder, pipeNode);
	*outPipeNode = pipeNode;
	return G_FS_PIPE_SUCCESSFUL;
}

g_fs_close_status filesystemClose(g_pid pid, g_fd fd, g_bool removeDescriptor)
{
	g_file_descriptor* descriptor = filesystemProcessGetDescriptor(pid, fd);
	if(!descriptor)
	{
		logInfo("%! failed to close fd %i in process %i, illegal descriptor", "fs", fd, pid);
		return G_FS_CLOSE_INVALID_FD;
	}

	g_fs_node* file = filesystemGetNode(descriptor->nodeId);
	if(!file)
	{
		logInfo("%! failed to close fd %i in process %i, illegal node", "fs", pid);
		return G_FS_CLOSE_INVALID_FD;
	}

	g_fs_delegate* delegate = filesystemFindDelegate(file);
	if(!delegate->close)
		kernelPanic("%! failed to close file %i, delegate had no implementation", "fs", file->id);

	g_fs_close_status status = delegate->close(file);
	if(status == G_FS_CLOSE_SUCCESSFUL && removeDescriptor)
	{
		filesystemProcessRemoveDescriptor(pid, fd);
	}

	logDebug("%! closed file descriptor %i in process %i", "fs", fd, pid);

	return status;
}

g_fs_seek_status filesystemSeek(g_task* task, g_fd fd, g_fs_seek_mode mode, int64_t amount, int64_t* outResult)
{
	g_file_descriptor* descriptor = filesystemProcessGetDescriptor(task->process->id, fd);
	if(!descriptor)
	{
		return G_FS_SEEK_INVALID_FD;
	}

	g_fs_node* node = filesystemGetNode(descriptor->nodeId);
	if(!node)
	{
		return G_FS_SEEK_INVALID_FD;
	}

	uint64_t length;
	if(filesystemGetLength(node, &length) != G_FS_LENGTH_SUCCESSFUL)
	{
		logInfo("%! failed to seek in file %i, could not get length", "fs", node->id);
		return G_FS_SEEK_ERROR;
	}

	// add amount to offset
	if(mode == G_FS_SEEK_CUR)
	{
		descriptor->offset += amount;
	} else if(mode == G_FS_SEEK_SET)
	{
		descriptor->offset = amount;
	} else if(mode == G_FS_SEEK_END)
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
	return G_FS_SEEK_SUCCESSFUL;
}

int filesystemGetAbsolutePathLength(g_fs_node* node)
{
	if(node->type == G_FS_NODE_TYPE_ROOT)
	{
		return 1;
	}

	int length = 0;
	if(node->parent) {
		length = filesystemGetAbsolutePathLength(node->parent);
	}

	int calced;
	if(length == 1) {
		calced = stringLength(node->name);
	} else {
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
	if(node->parent) {
		length = filesystemGetAbsolutePath(node->parent, buffer);
	}

	int copied;
	if(length == 1) {
		copied = stringCopy(&buffer[length], node->name);
	} else {
		buffer[length] = '/';
		copied = stringCopy(&buffer[length + 1], node->name) + 1;
	}
	return length + copied;
}
