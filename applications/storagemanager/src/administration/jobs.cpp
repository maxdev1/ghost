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
#include <ghost/system.h>
#include <ghost/tasks.h>
#include <new>
#include <string.h>
#include "boot.hpp"
#include "format.hpp"
#include "power.hpp"
#include "registry/discovery.hpp"

static std::map<g_volume_job_id, g_volume_admin_job*> adminJobs;
static g_volume_job_id adminNextJob = 1;

g_volume_admin_job* adminJobFind(g_volume_job_id id)
{
	auto entry = adminJobs.find(id);
	return entry == adminJobs.end() ? NULL : entry->second;
}

static g_volume_status adminJobPrepare(g_volume_registry* registry, g_tid sender, g_volume_admin_job* job)
{
	const g_volume_admin_request* request = &job->request;
	g_device_id device = request->device;
	uint64_t generation = request->generation;
	if(request->command == G_DISK_BOOT_WRITE || request->command == G_DISK_BOOT_COMMIT)
	{
		job->boot = request->boot;
		g_volume_status status = adminBootPrepare(registry, sender, &job->boot);
		if(status != G_VOLUME_OK)
			return status;

		device = job->boot.device;
		generation = job->boot.generation;
		job->info.volume = job->boot.volume;
	}
	else if(request->command == G_VOLUME_FORMAT || request->command == G_VOLUME_WIPE)
	{
		g_volume_record* volume = registryVolumeFind(registry, request->volume);
		if(!volume || !volume->active)
			return G_VOLUME_NOT_FOUND;


		device = volume->info.device;
		job->info.volume = request->volume;
		job->firstSector = volume->info.firstSector;
		job->sectorCount = volume->info.sectorCount;
	}

	g_volume_disk* disk = registryDiskFind(registry, device);
	if(!disk)
		return G_VOLUME_NOT_FOUND;
	if(!generation || generation != disk->snapshot.disk.generation)
		return G_VOLUME_CONFLICT;
	if(disk->snapshot.disk.sectorSize != G_VOLUME_SECTOR_SIZE)
		return G_VOLUME_UNSUPPORTED;
	if(!adminRequestDiscardsLayout(request) &&
	   (disk->snapshot.disk.layout == G_DISK_INVALID || disk->snapshot.disk.layout == G_DISK_UNSUPPORTED))
		return G_VOLUME_UNSUPPORTED;
	if(registryDiskIsBusy(registry, device, sender, 0))
		return G_VOLUME_BUSY;

	uint32_t required = G_BLOCK_WRITABLE | G_BLOCK_FLUSH_SUPPORTED;
	if((disk->snapshot.disk.flags & required) != required)
		return G_VOLUME_UNSUPPORTED;

	if(request->command == G_DISK_TABLE_CREATE)
	{
		if(request->layout != G_DISK_MBR)
			return G_VOLUME_UNSUPPORTED;
		if(request->flags & ~G_DISK_TABLE_REPLACE)
			return G_VOLUME_INVALID;
	}
	if(request->command == G_VOLUME_FORMAT)
	{
		g_volume_record* volume = registryVolumeFind(registry, request->volume);
		g_volume_status status = adminFormatPrepare(job, disk->snapshot.disk.layout, volume->info.scheme);
		if(status != G_VOLUME_OK)
			return status;
	}
	job->info.device = device;
	job->snapshot = disk->snapshot;
	return G_VOLUME_OK;
}

g_volume_status adminJobStart(g_volume_registry* registry, g_tid sender, const g_volume_admin_request* request,
							  g_volume_admin_response* response)
{
	if(!request->operation)
		return G_VOLUME_INVALID;
	for(auto& entry: adminJobs)
	{
		g_volume_admin_job* prior = entry.second;
		if(prior->client != sender || prior->request.operation != request->operation)
			continue;
		if(!adminRequestMatches(request, &prior->request))
			return G_VOLUME_CONFLICT;
		response->job = adminJobGetInfo(prior);
		return G_VOLUME_OK;
	}
	if(adminPowerIsQuiescing())
		return G_VOLUME_BUSY;

	g_volume_admin_job* job = new(std::nothrow) g_volume_admin_job{};
	if(!job)
		return G_VOLUME_UNAVAILABLE;
	job->info.command = request->command;
	job->request = *request;
	job->client = sender;
	job->formatter = G_TID_NONE;
	job->ioSafe = true;
	job->phase = G_VOLUME_JOB_CHECKING;
	g_volume_status status = adminJobPrepare(registry, sender, job);
	if(status != G_VOLUME_OK)
	{
		delete job;
		return status;
	}
	job->info.id = adminNextJob++;
	g_volume_disk* disk = registryDiskFind(registry, job->info.device);
	disk->job = job->info.id;
	job->thread = g_create_task_d((void*) adminJobRun, job);
	if(job->thread == G_TID_NONE)
	{
		disk->job = 0;
		delete job;
		return G_VOLUME_UNAVAILABLE;
	}
	adminJobs[job->info.id] = job;
	response->job = adminJobGetInfo(job);
	return G_VOLUME_OK;
}

/**
 * Rescan changed storage before releasing its reservation. Uncertain I/O keeps
 * the disk reserved and prevents power preparation from reporting readiness.
 */
static void adminJobFinish(g_volume_registry* registry, g_volume_admin_job* job)
{
	job->processed = true;
	job->info.partition = job->createdPartition;
	g_volume_disk* disk = registryDiskFind(registry, job->info.device);
	if(job->ioSafe && job->changed && job->info.command != G_DISK_BOOT_WRITE)
	{
		adminJobReportProgress(job, G_VOLUME_JOB_RESCANNING, 0, 1);
		g_volume_status status = registryDiscoveryScanDisk(registry, &disk->info, job->client, job->info.id, true);
		if(status != G_VOLUME_OK && job->result == G_VOLUME_OK)
			job->result = status;
		if(status == G_VOLUME_IO_ERROR)
			job->ioSafe = false;
	}
	if(job->ioSafe)
		disk->job = 0;
	job->info.ioSafe = job->ioSafe;
	job->info.result = job->result;
	job->info.state = job->result == G_VOLUME_OK ? G_VOLUME_JOB_SUCCEEDED : G_VOLUME_JOB_FAILED;
	if(job->info.partition)
		for(auto& entry: registry->volumes)
		{
			g_volume_record* volume = &entry.second;
			if(volume->active && volume->info.device == job->info.device &&
			   volume->info.partition == job->info.partition)
				job->info.volume = volume->info.id;
		}
}

void adminJobPoll(g_volume_registry* registry)
{
	for(auto& entry: adminJobs)
	{
		g_volume_admin_job* job = entry.second;
		if(job->processed)
			continue;
		if(!memoryAtomicLoad(&job->finished, G_MEMORY_ORDER_ACQUIRE))
		{
			if(g_get_pid_for_tid(job->thread) != G_PID_NONE)
				continue;
			job->result = G_VOLUME_IO_ERROR;
			job->ioSafe = false;
		}
		adminJobFinish(registry, job);
	}
}

bool adminJobHasPending(bool* ioSafe)
{
	bool pending = false;
	*ioSafe = true;

	for(auto& entry: adminJobs)
	{
		g_volume_admin_job* job = entry.second;
		if(!job->processed)
			pending = true;
		else if(!job->ioSafe)
			*ioSafe = false;
	}

	return pending;
}

g_volume_job_info adminJobGetInfo(const g_volume_admin_job* job)
{
	g_volume_job_info info = job->info;
	info.completed = memoryAtomicLoad(&job->completed, G_MEMORY_ORDER_RELAXED);
	info.total = memoryAtomicLoad(&job->total, G_MEMORY_ORDER_RELAXED);
	info.phase = (g_volume_job_phase) memoryAtomicLoad(&job->phase, G_MEMORY_ORDER_RELAXED);
	return info;
}
