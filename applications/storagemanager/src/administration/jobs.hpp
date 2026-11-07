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

#ifndef STORAGEMANAGER_JOBS
#define STORAGEMANAGER_JOBS

#include "requests.hpp"

/**
 * The message loop owns the job and reservation. The worker publishes its
 * result with finished; progress counters are read atomically while it runs.
 */
struct g_volume_admin_job
{
	g_volume_job_info info;
	g_volume_admin_request request;
	g_disk_boot_info boot;
	g_volume_disk_snapshot snapshot;

	uint64_t firstSector;
	uint64_t sectorCount;
	g_tid client;
	g_tid thread;
	g_tid formatter;

	uint64_t completed;
	uint64_t total;
	uint64_t lastPause;
	uint32_t phase;
	uint32_t createdPartition;

	bool finished;
	bool processed;
	bool changed;
	bool ioSafe;

	g_volume_status result;
	g_block_buffer buffer;
};

struct g_volume_registry;

/**
 * Returns the retained running or completed job for id, or null if unknown.
 */
g_volume_admin_job* adminJobFind(g_volume_job_id id);

/**
 * Validates and reserves the target disk before starting a worker job. Matching
 * operation retries from the same sender return the existing job instead.
 */
g_volume_status adminJobStart(g_volume_registry* registry, g_tid sender,
							 const g_volume_admin_request* request, g_volume_admin_response* response);

/**
 * Finalizes completed or lost workers and rescans changed disks. Reservations
 * remain held when the job's I/O safety is uncertain.
 */
void adminJobPoll(g_volume_registry* registry);

/**
 * Reports whether any jobs await completion processing. Sets ioSafe to false
 * if a processed job left I/O safety uncertain.
 */
bool adminJobHasPending(bool* ioSafe);

/**
 * Builds a public job-status snapshot with atomically read phase and progress.
 */
g_volume_job_info adminJobGetInfo(const g_volume_admin_job* job);

/**
 * Executes the job passed as the task argument, closes its I/O buffer and
 * publishes completion for the manager's message loop.
 */
void adminJobRun(void* argument);

/**
 * Atomically publishes phase and progress, periodically pausing the worker
 * during disk operations to let other tasks run.
 */
void adminJobReportProgress(g_volume_admin_job* job, g_volume_job_phase phase, uint64_t done, uint64_t total);

#endif
