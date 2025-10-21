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

#ifndef __KERNEL_IPC_PIPES__
#define __KERNEL_IPC_PIPES__

#include "kernel/utils/wait_queue.hpp"
#include "kernel/system/mutex.hpp"

/**
 * Entry in the reference list of a pipe.
 */
struct g_pipe_reference_entry
{
	g_pid pid;
	g_pipe_reference_entry* next;
};

/**
 * Structure of a pipe.
 */
struct g_pipeline
{
	g_mutex lock;

	uint8_t* buffer;
	uint8_t* writePosition;
	uint8_t* readPosition;
	uint32_t size;
	uint32_t capacity;

	uint16_t referencesRead;
	uint16_t referencesWrite;

	g_wait_queue waitersRead;
	g_wait_queue waitersWrite;
};

/**
 * Initializes the pipes.
 */
void pipeInitialize();

/**
 * Returns the next free pipe id.
 */
g_fs_phys_id pipeGetNextId();

/**
 * Returns a pipe by its id.
 */
g_pipeline* pipeGetById(g_fs_phys_id pipeId);

/**
 * Creates a new pipe.
 */
g_fs_pipe_status pipeCreate(g_fs_phys_id* outPipeId);

/**
 * Adds a read or write reference to the pipe.
 */
void pipeAddReference(g_fs_phys_id pipeId, g_file_flag_mode flags);

/**
 * Removes a reference from the pipe. If all references are gone, the pipe is removed.
 */
void pipeRemoveReference(g_fs_phys_id pipeId, g_file_flag_mode openFlags);

g_fs_read_status pipeRead(g_fs_phys_id pipeId, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outRead);

g_fs_write_status pipeWrite(g_fs_phys_id pipeId, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outWrote);

g_fs_length_status pipeGetLength(g_fs_phys_id pipeId, uint64_t* outLength);

g_fs_open_status pipeTruncate(g_fs_phys_id pipeId);

/**
 * Deletes the pipe.
 */
void pipeDeleteInternal(g_fs_phys_id pipeId, g_pipeline* pipe);

void pipeWaitForRead(g_tid task, g_fs_phys_id pipeId);
void pipeWaitForWrite(g_tid task, g_fs_phys_id pipeId);

#endif
