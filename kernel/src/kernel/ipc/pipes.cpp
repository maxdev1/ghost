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
#include "kernel/utils/hashmap.hpp"

#include "shared/logger/logger.hpp"

static g_fs_phys_id pipeNextId;
static g_mutex pipeNextIdLock;
static g_hashmap<g_fs_phys_id, g_pipeline*>* pipeMap;

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
	pipe->buffer = (uint8_t*) memoryAllocateKernelRange(G_PAGE_ALIGN_UP(pipe->capacity) / G_PAGE_SIZE);
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

	mutexRelease(&pipe->lock);

	if(pipe->referencesRead == 0 && pipe->referencesWrite == 0)
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

	uint32_t lengthToEnd = ((uint32_t) pipe->buffer + pipe->capacity) - (uint32_t) pipe->readPosition;
	if(length > lengthToEnd)
	{
		memoryCopy(buffer, pipe->readPosition, lengthToEnd);

		uint32_t remaining = length - lengthToEnd;
		memoryCopy(&buffer[lengthToEnd], pipe->buffer, remaining);

		pipe->readPosition = (uint8_t*) ((uint32_t) pipe->buffer + remaining);
	}
	else
	{
		memoryCopy(buffer, pipe->readPosition, length);

		pipe->readPosition = (uint8_t*) ((uint32_t) pipe->readPosition + length);
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

	uint32_t space = (pipe->capacity - pipe->size);

	g_fs_write_status status;
	if(space > 0)
	{
		length = (space >= length) ? length : space;

		uint32_t lengthToEnd = ((uint32_t) pipe->buffer + pipe->capacity) - (uint32_t) pipe->writePosition;
		if(length > lengthToEnd)
		{
			memoryCopy(pipe->writePosition, buffer, lengthToEnd);

			uint32_t remaining = length - lengthToEnd;
			memoryCopy(pipe->buffer, &buffer[lengthToEnd], remaining);

			pipe->writePosition = (uint8_t*) ((uint32_t) pipe->buffer + remaining);
		}
		else
		{
			memoryCopy(pipe->writePosition, buffer, length);

			pipe->writePosition = (uint8_t*) ((uint32_t) pipe->writePosition + length);
		}

		if(pipe->writePosition == pipe->buffer + pipe->capacity)
			pipe->writePosition = pipe->buffer;

		pipe->size += length;
		*outWrote = length;

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

void pipeWaitForRead(g_tid task, g_fs_phys_id pipeId)
{
	g_pipeline* pipe = pipeGetById(pipeId);
	if(!pipe)
		return;

	mutexAcquire(&pipe->lock);
	waitQueueAdd(&pipe->waitersRead, task);
	mutexRelease(&pipe->lock);
}

void pipeWaitForWrite(g_tid task, g_fs_phys_id pipeId)
{
	g_pipeline* pipe = pipeGetById(pipeId);
	if(!pipe)
		return;

	mutexAcquire(&pipe->lock);
	waitQueueAdd(&pipe->waitersWrite, task);
	mutexRelease(&pipe->lock);
}
