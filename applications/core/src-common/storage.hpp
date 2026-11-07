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

#ifndef CORE_STORAGE
#define CORE_STORAGE
#include "arguments.hpp"
#include <libstorage/admin.hpp>
#include <ghost/tasks.h>
#include <ghost/system.h>
#include <cstdio>
#include <cstring>

static inline const char* storageStatusText(g_volume_status status)
{
	switch(status)
	{
		case G_VOLUME_OK:
			return "success";
		case G_VOLUME_INVALID:
			return "invalid parameters or insufficient space";
		case G_VOLUME_NOT_FOUND:
			return "resource not found";
		case G_VOLUME_CONFLICT:
			return "layout, identity, or ownership conflict";
		case G_VOLUME_NOT_OWNER:
			return "ownership changed";
		case G_VOLUME_BUSY:
			return "disk or volume is busy; unmount its volumes first";
		case G_VOLUME_IO_ERROR:
			return "storage I/O error; partial changes are possible";
		case G_VOLUME_UNSUPPORTED:
			return "unsupported operation or layout";
		case G_VOLUME_TIMED_OUT:
			return "request timed out; inspect the disk and its active job before retrying";
		case G_VOLUME_UNAVAILABLE:
			return "manager or driver unavailable";
		case G_VOLUME_PROTOCOL_ERROR:
			return "invalid service response";
		default:
			return "unknown error";
	}
}

static inline int storageError(const char* tool, g_volume_status status)
{
	fprintf(stderr, "%s: %s\n", tool, storageStatusText(status));
	return 1;
}

/**
 * Sizes are exact sector counts or binary byte quantities divisible by the
 * device's sector size. Overflow and implicit rounding are rejected.
 */
static inline bool storageSectors(const char* text, uint32_t sectorSize, uint64_t* result)
{
	if(!result)
		return false;

	uint64_t value;
	auto c = text;
	if(!coreParseUnsignedPrefix(c, value))
		return false;
	if(!value)
		return false;
	if(!*c || !strcmp(c, "s"))
	{
		*result = value;
		return true;
	}

	uint64_t multiplier = !strcmp(c, "KiB")	  ? 1024
						  : !strcmp(c, "MiB") ? 1048576
						  : !strcmp(c, "GiB") ? 1073741824
						  : !strcmp(c, "TiB") ? 1099511627776ull
											  : 0;
	if(!multiplier || !sectorSize || value > UINT64_MAX / multiplier || value * multiplier % sectorSize)
		return false;

	*result = value * multiplier / sectorSize;
	return true;
}

static inline const char* storageLayoutText(g_disk_layout layout)
{
	switch(layout)
	{
		case G_DISK_NONE:
			return "none";
		case G_DISK_MBR:
			return "mbr";
		case G_DISK_GPT:
			return "gpt";
		case G_DISK_INVALID:
			return "invalid";
		default:
			return "unsupported";
	}
}

static inline const char* storagePhaseText(g_volume_job_phase phase)
{
	switch(phase)
	{
		case G_VOLUME_JOB_CHECKING:
			return "checking";
		case G_VOLUME_JOB_ZEROING:
			return "zeroing";
		case G_VOLUME_JOB_EDITING:
			return "editing table";
		case G_VOLUME_JOB_FORMATTING:
			return "formatting";
		default:
			return "rescanning";
	}
}

static inline void storagePrintJob(const g_volume_job_info& job)
{
	printf("Job %llu: %s, %llu/%llu sectors, %s\n", (unsigned long long) job.id, storagePhaseText(job.phase),
		   (unsigned long long) job.completed, (unsigned long long) job.total,
		   job.state == G_VOLUME_JOB_RUNNING ? "running" : storageStatusText(job.result));
	if(!job.ioSafe)
		printf("Disk remains reserved because I/O completion is uncertain.\n");
}

static inline int storageJobQuery(const char* tool, const char* text)
{
	uint64_t id;
	if(!coreParseUnsigned(text, &id) || !id)
		return storageError(tool, G_VOLUME_INVALID);

	g_volume_job_info job;
	auto status = storageManagerJobGet(id, &job);
	if(status != G_VOLUME_OK)
		return storageError(tool, status);

	storagePrintJob(job);
	return job.state == G_VOLUME_JOB_FAILED ? 1 : 0;
}

template<typename Request>
static inline int storageRunJob(const char* tool, Request request, g_volume_job_info* final = nullptr)
{
	g_volume_job_info job;
	auto status = storageManagerAdminStart(request, &job);
	if(status != G_VOLUME_OK)
		return storageError(tool, status);

	printf("Job %llu accepted.\n", (unsigned long long) job.id);
	fflush(stdout);
	auto lastPhase = job.phase;
	uint64_t reported = g_millis();

	while(job.state == G_VOLUME_JOB_RUNNING)
	{
		g_sleep(50);
		status = storageManagerJobGet(job.id, &job);
		if(status != G_VOLUME_OK)
		{
			fprintf(stderr, "%s: job %llu may still be running; query it with '%s job %llu get'\n", tool,
					(unsigned long long) job.id, tool, (unsigned long long) job.id);
			return storageError(tool, status);
		}

		if(job.state == G_VOLUME_JOB_RUNNING && (lastPhase != job.phase || g_millis() - reported >= 1000))
		{
			storagePrintJob(job);
			fflush(stdout);
			lastPhase = job.phase;
			reported = g_millis();
		}
	}

	storagePrintJob(job);
	if(final)
		*final = job;
	if(job.state != G_VOLUME_JOB_SUCCEEDED)
		return storageError(tool, job.result);
	if(job.partition)
		printf("Partition %u created; volume %llu.\n", job.partition, (unsigned long long) job.volume);
	return 0;
}
#endif
