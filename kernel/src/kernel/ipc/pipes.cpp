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

#include "kernel/ipc/pipes.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/utils/hashmap.hpp"

#include "kernel/logger/logger.hpp"

static g_fs_phys_id pipeNextId;
static g_mutex pipeNextIdLock;
static g_hashmap<g_fs_phys_id, g_pipeline*>* pipeMap;

static bool pipeCanWrite(g_pipeline* pipe, uint64_t length)
{
	size_t space = pipe->capacity - pipe->size;
	return length <= pipe->capacity ? space >= length : space > 0;
}

void pipeInitialize()
{
	mutexInitializeTask(&pipeNextIdLock, __func__);
	pipeNextId = 0;

	pipeMap = hashmapCreateNumeric<g_fs_phys_id, g_pipeline*>(128);
}

g_fs_pipe_status pipeCreate(g_fs_phys_id* outPipeId)
{
	g_pipeline* pipe = (g_pipeline*) heapAllocateClear(sizeof(g_pipeline));

	mutexInitializeTask(&pipe->lock, __func__);
	pipe->capacity = G_PIPE_DEFAULT_CAPACITY;
	pipe->buffer = (uint8_t*) memoryAllocateKernel(G_PAGE_ALIGN_UP(pipe->capacity) / G_PAGE_SIZE);
	pipe->readPosition = pipe->buffer;
	pipe->writePosition = pipe->buffer;
	waitQueueInitialize(&pipe->waitersRead);
	waitQueueInitialize(&pipe->waitersWrite);

	g_fs_phys_id pipeId = pipeGetNextId();
	hashmapPut<g_fs_phys_id, g_pipeline*>(pipeMap, pipeId, pipe);
	*outPipeId = pipeId;

	return G_FS_PIPE_SUCCESSFUL;
}

void pipeDeleteInternal(g_fs_phys_id pipeId, g_pipeline* pipe)
{
	memoryFreeKernelRange((g_virtual_address) pipe->buffer);
	heapFree(pipe);
	hashmapRemove(pipeMap, pipeId);

	logDebug("%! deleted pipe %i", "pipe", pipeId);
}

g_fs_phys_id pipeGetNextId()
{
	mutexAcquire(&pipeNextIdLock);
	g_fs_phys_id nextId = pipeNextId++;
	mutexRelease(&pipeNextIdLock);
	return nextId;
}

g_pipeline* pipeGetById(g_fs_phys_id pipeId)
{
	return hashmapGet<g_fs_phys_id, g_pipeline*>(pipeMap, pipeId, 0);
}

void pipeAddReference(g_fs_phys_id pipeId, g_file_flag_mode flags)
{
	g_pipeline* pipe = pipeGetById(pipeId);
	if(!pipe)
	{
		logInfo("%! tried to add reference to non-existing pipe %i", "pipe", pipeId);
		return;
	}

	mutexAcquire(&pipe->lock);

	if(flags & G_FILE_FLAG_MODE_WRITE)
	{
		pipe->referencesWrite++;
	}
	else
	{
		pipe->referencesRead++;
	}

	mutexRelease(&pipe->lock);
}

void pipeRemoveReference(g_fs_phys_id pipeId, g_file_flag_mode flags)
{
	g_pipeline* pipe = pipeGetById(pipeId);
	if(!pipe)
	{
		logInfo("%! tried to remove reference from non-existing pipe %i", "pipe", pipeId);
		return;
	}

	mutexAcquire(&pipe->lock);

	if(flags & G_FILE_FLAG_MODE_WRITE)
	{
		pipe->referencesWrite--;
		waitQueueWake(&pipe->waitersRead);
	}
	else
	{
		pipe->referencesRead--;
		waitQueueWake(&pipe->waitersWrite);
	}

	bool deletePipe = pipe->referencesRead == 0 && pipe->referencesWrite == 0;
	mutexRelease(&pipe->lock);

	if(deletePipe)
	{
		pipeDeleteInternal(pipeId, pipe);
	}
}

g_fs_read_status pipeRead(g_fs_phys_id pipeId, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outRead)
{
	g_pipeline* pipe = pipeGetById(pipeId);
	if(!pipe)
	{
		logInfo("%! tried to read from non-existing pipe %i", "pipe", pipeId);
		return G_FS_READ_ERROR;
	}

	mutexAcquire(&pipe->lock);

	length = (pipe->size >= length) ? length : pipe->size;

	size_t lengthToEnd = ((g_address) pipe->buffer + pipe->capacity) - (size_t) pipe->readPosition;
	if(length > lengthToEnd)
	{
		memoryCopy(buffer, pipe->readPosition, lengthToEnd);

		size_t remaining = length - lengthToEnd;
		memoryCopy(&buffer[lengthToEnd], pipe->buffer, remaining);

		pipe->readPosition = (uint8_t*) ((g_address) pipe->buffer + remaining);
	}
	else
	{
		memoryCopy(buffer, pipe->readPosition, length);

		pipe->readPosition = (uint8_t*) ((g_address) pipe->readPosition + length);
	}

	if(pipe->readPosition == pipe->buffer + pipe->capacity)
		pipe->readPosition = pipe->buffer;

	g_fs_read_status status;
	if(length > 0)
	{
		pipe->size -= length;

		*outRead = length;
		status = G_FS_READ_SUCCESSFUL;
		waitQueueWake(&pipe->waitersWrite);
	}
	else
	{
		*outRead = 0;
		status = pipe->referencesWrite > 0 ? G_FS_READ_BUSY : G_FS_READ_SUCCESSFUL;
	}

	mutexRelease(&pipe->lock);
	return status;
}

g_fs_write_status pipeWrite(g_fs_phys_id pipeId, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outWrote)
{
	g_pipeline* pipe = pipeGetById(pipeId);
	if(!pipe)
		return G_FS_WRITE_ERROR;

	mutexAcquire(&pipe->lock);

	size_t space = (pipe->capacity - pipe->size);

	g_fs_write_status status;
	if(pipe->referencesRead == 0)
	{
		*outWrote = 0;
		status = G_FS_WRITE_ERROR;
	}
	else if(!pipeCanWrite(pipe, length))
	{
		// Keep small writes atomic so packet-based producers can safely use nonblocking pipes.
		*outWrote = 0;
		status = G_FS_WRITE_BUSY;
	}
	else if(space > 0)
	{
		uint64_t writeLength = (space >= length) ? length : space;

		size_t lengthToEnd = ((g_address) pipe->buffer + pipe->capacity) - (size_t) pipe->writePosition;
		if(writeLength > lengthToEnd)
		{
			memoryCopy(pipe->writePosition, buffer, lengthToEnd);

			size_t remaining = writeLength - lengthToEnd;
			memoryCopy(pipe->buffer, &buffer[lengthToEnd], remaining);

			pipe->writePosition = (uint8_t*) ((g_address) pipe->buffer + remaining);
		}
		else
		{
			memoryCopy(pipe->writePosition, buffer, writeLength);

			pipe->writePosition = (uint8_t*) ((g_address) pipe->writePosition + writeLength);
		}

		if(pipe->writePosition == pipe->buffer + pipe->capacity)
			pipe->writePosition = pipe->buffer;

		pipe->size += writeLength;
		*outWrote = writeLength;

		status = G_FS_WRITE_SUCCESSFUL;
		waitQueueWake(&pipe->waitersRead);
	}
	else
	{
		*outWrote = 0;
		status = G_FS_WRITE_BUSY;
	}

	mutexRelease(&pipe->lock);

	return status;
}

g_fs_length_status pipeGetLength(g_fs_phys_id pipeId, uint64_t* outLength)
{
	g_pipeline* pipe = pipeGetById(pipeId);
	if(!pipe)
		return G_FS_LENGTH_ERROR;

	return pipe->size;
}

g_fs_open_status pipeTruncate(g_fs_phys_id pipeId)
{
	g_pipeline* pipe = pipeGetById(pipeId);
	if(!pipe)
		return G_FS_OPEN_ERROR;

	mutexAcquire(&pipe->lock);
	pipe->size = 0;
	pipe->readPosition = pipe->buffer;
	pipe->writePosition = pipe->buffer;
	mutexRelease(&pipe->lock);

	return G_FS_OPEN_SUCCESSFUL;
}

bool pipeWaitForRead(g_task* task, g_fs_phys_id pipeId, uint64_t)
{
	g_pipeline* pipe = pipeGetById(pipeId);
	if(!pipe)
		return false;

	mutexAcquire(&pipe->lock);
	bool shouldWait = pipe->size == 0 && pipe->referencesWrite > 0;
	if(shouldWait)
	{
		taskingPrepareWait(task, "read");
		waitQueueAdd(&pipe->waitersRead, task->id);
	}
	mutexRelease(&pipe->lock);
	return shouldWait;
}

bool pipeWaitForWrite(g_task* task, g_fs_phys_id pipeId, uint64_t length)
{
	g_pipeline* pipe = pipeGetById(pipeId);
	if(!pipe)
		return false;

	mutexAcquire(&pipe->lock);
	bool shouldWait = !pipeCanWrite(pipe, length) && pipe->referencesRead > 0;
	if(shouldWait)
	{
		taskingPrepareWait(task, "write");
		waitQueueAdd(&pipe->waitersWrite, task->id);
	}
	mutexRelease(&pipe->lock);
	return shouldWait;
}
