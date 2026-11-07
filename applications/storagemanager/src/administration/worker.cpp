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

#include "jobs.hpp"
#include <ghost/memory.h>
#include "format.hpp"
#include <string.h>
#include <ghost/messages.h>
#include <ghost/system.h>

void adminJobReportProgress(g_volume_admin_job* job, g_volume_job_phase phase, uint64_t done, uint64_t total)
{
	memoryAtomicStore(&job->completed, done, G_MEMORY_ORDER_RELAXED);
	memoryAtomicStore(&job->total, total, G_MEMORY_ORDER_RELAXED);
	memoryAtomicStore(&job->phase, (uint32_t) phase, G_MEMORY_ORDER_RELAXED);
	/**
	 * Targeted IPC yields favor the disk driver. Periodically block this worker
	 * so sustained bulk I/O also leaves time for unrelated runnable tasks.
	 */
	if(phase != G_VOLUME_JOB_RESCANNING && g_millis() - job->lastPause >= 4)
	{
		g_sleep(1);
		job->lastPause = g_millis();
	}
}

static g_volume_status adminJobCheckSnapshot(g_volume_admin_job* job, const g_volume_storage_io* io)
{
	if(adminRequestDiscardsLayout(&job->request))
	{
		uint8_t mbr[G_VOLUME_SECTOR_SIZE];
		if(!diskIoRead(io, 0, mbr, 1))
			return G_VOLUME_IO_ERROR;
		return memcmp(mbr, job->snapshot.mbr, sizeof(mbr)) ? G_VOLUME_CONFLICT : G_VOLUME_OK;
	}
	g_volume_disk_snapshot current;
	g_volume_status status = diskLayoutInspect(&job->snapshot.disk, io, &current);
	if(status != G_VOLUME_OK)
		return status;
	if(memcmp(current.mbr, job->snapshot.mbr, G_VOLUME_SECTOR_SIZE) ||
	   current.disk.layout != job->snapshot.disk.layout || current.disk.partitions != job->snapshot.disk.partitions)
		return G_VOLUME_CONFLICT;
	for(uint32_t i = 0; i < current.disk.partitions; ++i)
	{
		const g_disk_partition_info* a = &current.partitions[i];
		const g_disk_partition_info* b = &job->snapshot.partitions[i];
		if(a->number != b->number || a->firstSector != b->firstSector || a->sectorCount != b->sectorCount ||
		   a->type != b->type || memcmp(a->typeGuid, b->typeGuid, sizeof(a->typeGuid)))
			return G_VOLUME_CONFLICT;
	}
	return G_VOLUME_OK;
}

static g_volume_status adminJobExecute(g_volume_admin_job* job, const g_volume_storage_io* io)
{
	const g_volume_admin_request* request = &job->request;
	switch(request->command)
	{
		case G_VOLUME_FORMAT:
			return adminFormatRun(job);
		case G_DISK_TABLE_CREATE:
			return diskPartitionCreateTable(&job->snapshot, io, request->flags & G_DISK_TABLE_REPLACE);
		case G_DISK_PARTITION_CREATE:
			return diskPartitionCreate(&job->snapshot, &request->partitionCreate, io, &job->createdPartition);
		case G_DISK_PARTITION_DELETE:
			return diskPartitionDelete(&job->snapshot, request->partition, io);
		case G_DISK_WIPE:
			return diskWipeRange(&job->snapshot.disk, 0, job->snapshot.disk.sectorCount, io);
		case G_VOLUME_WIPE:
			return diskWipeRange(&job->snapshot.disk, job->firstSector, job->sectorCount, io);
		case G_DISK_BOOT_WRITE:
			return diskBootWrite(&job->snapshot, &job->boot, request->firstSector, request->data, request->dataSize, io);
		case G_DISK_BOOT_COMMIT:
			return diskBootCommit(&job->snapshot, &job->boot, request->data, request->flags, io);
		default:
			return G_VOLUME_INVALID;
	}
}

void adminJobRun(void* argument)
{
	g_volume_admin_job* job = (g_volume_admin_job*) argument;
	job->lastPause = g_millis();

	g_block_device_info actual = {};
	const g_disk_info* disk = &job->snapshot.disk;
	if(blockDeviceGetInfo(disk->driver, disk->device, &actual) != G_BLOCK_SUCCESS)
	{
		job->result = G_VOLUME_IO_ERROR;
	}
	else if(actual.sectorSize != disk->sectorSize || actual.sectorCount != disk->sectorCount ||
			strcmp(actual.serial, disk->serial))
	{
		job->result = G_VOLUME_CONFLICT;
	}
	else if(blockDeviceOpenBuffer(disk->driver, &job->buffer) != G_BLOCK_SUCCESS)
	{
		job->result = G_VOLUME_IO_ERROR;
	}
	else
	{
		g_volume_storage_io io = {};
		io.device = job->info.device;
		io.buffer = &job->buffer;
		io.job = job;
		io.maxSectors = actual.maxTransferSectors;
		if(io.maxSectors > G_VOLUME_MAX_TRANSFER_SECTORS)
			io.maxSectors = G_VOLUME_MAX_TRANSFER_SECTORS;

		job->result = adminJobCheckSnapshot(job, &io);
		if(job->result == G_VOLUME_OK)
			job->result = adminJobExecute(job, &io);
	}

	if(job->buffer.handle)
	{
		g_block_status closed = blockDeviceCloseBuffer(&job->buffer);
		if(closed != G_BLOCK_SUCCESS)
			closed = blockDeviceCloseBuffer(&job->buffer);
		if(closed != G_BLOCK_SUCCESS)
		{
			job->ioSafe = false;
			job->result = G_VOLUME_IO_ERROR;
		}
	}

	memoryAtomicStore(&job->finished, true, G_MEMORY_ORDER_RELEASE);
}
