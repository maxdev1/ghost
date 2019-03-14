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
static g_mutex pipeReferenceLock;

void pipeInitialize()
{
	mutexInitialize(&pipeNextIdLock);
	pipeNextId = 0;

	mutexInitialize(&pipeReferenceLock);

	pipeMap = hashmapCreateNumeric<g_fs_phys_id, g_pipeline*>(128);
}

g_fs_pipe_status pipeCreate(g_fs_phys_id* outPipeId)
{
	g_pipeline* pipe = (g_pipeline*) heapAllocateClear(sizeof(g_pipeline));

	mutexInitialize(&pipe->lock);
	pipe->capacity = G_PIPE_DEFAULT_CAPACITY;
	pipe->size = 0;
	pipe->buffer = (uint8_t*) heapAllocate(pipe->capacity);
	pipe->readPosition = pipe->buffer;
	pipe->writePosition = pipe->buffer;

	g_fs_phys_id pipeId = pipeGetNextId();
	hashmapPut<g_fs_phys_id, g_pipeline*>(pipeMap, pipeId, pipe);
	*outPipeId = pipeId;

	return G_FS_PIPE_SUCCESSFUL;
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

void pipeAddReference(g_fs_phys_id pipeId)
{
	mutexAcquire(&pipeReferenceLock);
	g_pipeline* pipe = pipeGetById(pipeId);

	if(pipe)
	{
		mutexAcquire(&pipe->lock);
		pipe->references++;
		mutexRelease(&pipe->lock);
	} else
	{
		logInfo("%! tried to add reference to non-existing pipe %i", "pipe", pipeId);
	}

	mutexRelease(&pipeReferenceLock);
}

void pipeRemoveReference(g_fs_phys_id pipeId)
{
	mutexAcquire(&pipeReferenceLock);
	g_pipeline* pipe = pipeGetById(pipeId);

	if(pipe)
	{
		mutexAcquire(&pipe->lock);
		pipe->references--;
		mutexRelease(&pipe->lock);

		if(pipe->references == 0)
		{
			pipeDeleteInternal(pipeId, pipe);
		}
	} else
	{
		logInfo("%! tried to remove reference from non-existing pipe %i", "pipe", pipeId);
	}

	mutexRelease(&pipeReferenceLock);
}

void pipeDeleteInternal(g_fs_phys_id pipeId, g_pipeline* pipe)
{
	heapFree(pipe);
	hashmapRemove(pipeMap, pipeId);

	logDebug("%! deleted pipe %i", "pipe", pipeId);
}

g_fs_read_status pipeRead(g_fs_phys_id pipeId, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outRead)
{
	g_pipeline* pipe = pipeGetById(pipeId);
	if(!pipe)
		return G_FS_READ_ERROR;

	mutexAcquire(&pipe->lock);

	// find out what size can be read at maximum
	length = (pipe->size >= length) ? length : pipe->size;

	// calculate how much space remains until the end of the pipe
	uint32_t spaceToEnd = ((uint32_t) pipe->buffer + pipe->capacity) - (uint32_t) pipe->readPosition;

	// if we want to read more than remaining
	if(length > spaceToEnd)
	{
		// copy bytes from the location of the read pointer
		memoryCopy(buffer, pipe->readPosition, spaceToEnd);

		// copy remaining data
		uint32_t remaining = length - spaceToEnd;
		memoryCopy(&buffer[spaceToEnd], pipe->buffer, remaining);

		// set the read pointer to it's new location
		pipe->readPosition = (uint8_t*) ((uint32_t) pipe->buffer + remaining);

	} else
	{
		// there are enough bytes left from read pointer, copy it to buffer
		memoryCopy(buffer, pipe->readPosition, length);

		// set the read pointer to it's new location
		pipe->readPosition = (uint8_t*) ((uint32_t) pipe->readPosition + length);
	}

	// reset read pointer if end reached
	if(pipe->readPosition == pipe->buffer + pipe->capacity)
	{
		pipe->readPosition = pipe->buffer;
	}

	g_fs_read_status status;
	// if any bytes could be copied
	if(length > 0)
	{
		// decrease pipes remaining bytes
		pipe->size -= length;

		// finish with success
		*outRead = length;
		status = G_FS_READ_SUCCESSFUL;

	} else
	{
		*outRead = 0;
		status = G_FS_READ_BUSY;
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
		// check how many bytes can be written
		length = (space >= length) ? length : space;

		// check how many bytes can be written at the write pointer
		uint32_t spaceToEnd = ((uint32_t) pipe->buffer + pipe->capacity) - (uint32_t) pipe->writePosition;

		if(length > spaceToEnd)
		{
			// write bytes at the write pointer
			memoryCopy(pipe->writePosition, buffer, spaceToEnd);

			// write remaining bytes to the start of the pipe
			uint32_t remain = length - spaceToEnd;
			memoryCopy(pipe->buffer, &buffer[spaceToEnd], remain);

			// set the write pointer to the new location
			pipe->writePosition = (uint8_t*) ((uint32_t) pipe->buffer + remain);

		} else
		{
			// just write bytes at write pointer
			memoryCopy(pipe->writePosition, buffer, length);

			// set the write pointer to the new location
			pipe->writePosition = (uint8_t*) ((uint32_t) pipe->writePosition + length);
		}

		// reset write pointer if end reached
		if(pipe->writePosition == pipe->buffer + pipe->capacity)
		{
			pipe->writePosition = pipe->buffer;
		}

		pipe->size += length;
		*outWrote = length;
		status = G_FS_WRITE_SUCCESSFUL;

	} else
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
