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

#include "administration.hpp"
#include <algorithm>
#include <cstring>
#include <ghost/system.h>
#include <ghost/tasks.h>
#include <libblockdevice/driver.hpp>
#include <map>
#include <new>
#include <string>

struct g_volume_admin_job
{
	g_volume_job_info info;
	g_volume_admin_request request;
	g_volume_admin_request original;
	g_volume_disk_snapshot snapshot;
	g_tid client = G_TID_NONE;
	g_tid thread = G_TID_NONE;
	g_tid formatter = G_TID_NONE;
	uint64_t completed = 0;
	uint64_t total = 0;
	uint64_t lastPause = 0;
	uint32_t phase = G_VOLUME_JOB_CHECKING;
	uint32_t createdPartition = 0;
	bool finished = false;
	bool processed = false;
	bool changed = false;
	bool ioSafe = true;
	g_volume_status result = G_VOLUME_OK;
	g_block_buffer buffer;
};

static std::map<g_volume_job_id, g_volume_admin_job*> storageManagerAdminJobs;
static std::map<std::pair<g_tid, uint64_t>, g_volume_job_id> storageManagerAdminOperations;
static std::map<std::string, g_tid> storageManagerFormatters;
static g_volume_job_id storageManagerNextJob = 1;
static g_volume_scan_function diskScanner = nullptr;
static uint32_t storageManagerAdminPowerRequest;
static bool storageManagerAdminPowerAcknowledged;

void storageManagerAdminInitialize(g_volume_scan_function scan)
{
	diskScanner = scan;
	storageManagerAdminPowerRequest = 0;
	storageManagerAdminPowerAcknowledged = false;
}

bool storageManagerAdminQuiescing() { return storageManagerAdminPowerRequest != 0; }

static bool storageManagerAdminReceivePower(const g_message_header* message, const void* content)
{
	if(message->sender != G_TID_NONE || message->length != sizeof(g_power_event))
		return false;

	g_power_event event;
	memcpy(&event, content, sizeof(event));
	if(event.magic != G_POWER_EVENT_MAGIC)
		return false;
	if(!event.request || event.action > G_POWER_REBOOT)
		return true;

	if(event.type == G_POWER_EVENT_PREPARE && !storageManagerAdminQuiescing())
	{
		storageManagerAdminPowerRequest = event.request;
		storageManagerAdminPowerAcknowledged = false;
	}
	else if(event.type == G_POWER_EVENT_CANCEL && event.request == storageManagerAdminPowerRequest)
	{
		storageManagerAdminPowerRequest = 0;
		storageManagerAdminPowerAcknowledged = false;
		g_power_acknowledge(event.request, 1);
	}
	return true;
}

/**
 * Keep serving claims and completions while accepted work drains. Readiness
 * keeps new work excluded until the kernel cancels or commits the power request.
 */
void storageManagerAdminPollPower(bool controlsPending)
{
	if(!storageManagerAdminQuiescing() || storageManagerAdminPowerAcknowledged)
		return;
	bool pending = controlsPending;
	bool ready = true;

	for(const auto& entry: storageManagerAdminJobs)
	{
		auto job = entry.second;
		pending |= !job->processed;
		if(job->processed && !job->ioSafe)
			ready = false;
	}

	if(ready && pending)
		return;
	if(g_power_acknowledge(storageManagerAdminPowerRequest, ready) == G_POWER_SUCCESSFUL)
		storageManagerAdminPowerAcknowledged = true;
}

static void storageManagerAdminReply(g_tid client, g_message_transaction tx, g_volume_admin_response response)
{
	g_send_message_tm(client, &response, sizeof(response), tx, G_MESSAGE_SEND_MODE_NON_BLOCKING);
}

static bool diskAdminRead(void* context, uint64_t lba, uint8_t* bytes, uint32_t count)
{
	auto job = static_cast<g_volume_admin_job*>(context);

	if(blockDeviceReadBuffer(&job->buffer, job->request.device, lba, count) != G_BLOCK_SUCCESS)
		return false;

	memcpy(bytes, job->buffer.data, count * G_VOLUME_SECTOR_SIZE);
	return true;
}

static bool diskAdminWrite(void* context, uint64_t lba, const uint8_t* bytes, uint32_t count)
{
	auto job = static_cast<g_volume_admin_job*>(context);
	job->changed = true;
	memcpy(job->buffer.data, bytes, count * G_VOLUME_SECTOR_SIZE);
	return blockDeviceWriteBuffer(&job->buffer, job->request.device, lba, count) == G_BLOCK_SUCCESS;
}

static bool diskAdminFlush(void* context)
{
	auto job = static_cast<g_volume_admin_job*>(context);
	return blockDeviceFlush(job->snapshot.disk.driver, job->request.device) == G_BLOCK_SUCCESS;
}

static void storageManagerAdminProgress(void* context, g_volume_job_phase phase, uint64_t done, uint64_t total)
{
	auto job = static_cast<g_volume_admin_job*>(context);
	__atomic_store_n(&job->completed, done, __ATOMIC_RELAXED);
	__atomic_store_n(&job->total, total, __ATOMIC_RELAXED);
	__atomic_store_n(&job->phase, uint32_t(phase), __ATOMIC_RELAXED);
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

static g_volume_status volumeFormat(g_volume_admin_job* job)
{
	auto request = job->request;
	request.command = G_VOLUME_FORMAT_WORK;
	request.job = job->info.id;
	request.type = job->snapshot.disk.driver;

	auto tx = g_get_message_tx_id();
	if(g_send_message_tm(job->formatter, &request, sizeof(request), tx, G_MESSAGE_SEND_MODE_NON_BLOCKING) !=
	   G_MESSAGE_SEND_STATUS_SUCCESSFUL)
		return G_VOLUME_UNAVAILABLE;

	job->changed = true;
	job->ioSafe = false;

	auto owner = g_get_pid_for_tid(job->formatter);
	g_tid responder = G_TID_NONE;
	uint8_t incoming[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	for(;;)
	{
		if(owner == G_PID_NONE || g_get_pid_for_tid(job->formatter) != owner)
			return G_VOLUME_UNAVAILABLE;

		auto status = g_receive_message_tm(incoming, sizeof(incoming), tx, G_MESSAGE_RECEIVE_MODE_NON_BLOCKING);
		if(status == G_MESSAGE_RECEIVE_STATUS_EMPTY)
		{
			if(responder != G_TID_NONE && g_get_pid_for_tid(responder) == G_PID_NONE)
				return G_VOLUME_UNAVAILABLE;
			g_sleep(1);
			continue;
		}

		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			return G_VOLUME_UNAVAILABLE;

		auto header = reinterpret_cast<g_message_header*>(incoming);
		if(header->sender == G_TID_NONE || g_get_pid_for_tid(header->sender) != owner)
			continue;

		responder = header->sender;
		if(header->length != sizeof(g_volume_admin_response))
			return G_VOLUME_PROTOCOL_ERROR;

		g_volume_admin_response response;
		memcpy(&response, G_MESSAGE_CONTENT(incoming), sizeof(response));
		if(response.magic != G_VOLUME_ADMIN_MAGIC || response.version != G_VOLUME_ADMIN_VERSION ||
		   response.command != G_VOLUME_FORMAT_WORK || response.job.id != job->info.id ||
		   response.status > G_VOLUME_PROTOCOL_ERROR || response.job.state > G_VOLUME_JOB_FAILED ||
		   response.job.ioSafe > 1)
			return G_VOLUME_PROTOCOL_ERROR;

		storageManagerAdminProgress(job, G_VOLUME_JOB_FORMATTING, response.job.completed, response.job.total);
		if(response.job.state == G_VOLUME_JOB_RUNNING)
			continue;

		job->ioSafe = response.job.ioSafe;
		if(responder == job->formatter)
			return response.status;

		request.command = G_VOLUME_FORMAT_ACK;

		while(g_send_message_tm(responder, &request, sizeof(request), tx, G_MESSAGE_SEND_MODE_NON_BLOCKING) ==
					  G_MESSAGE_SEND_STATUS_FULL &&
			  g_get_pid_for_tid(responder) != G_PID_NONE)
		{
			g_sleep(1);
		}
		return response.status;
	}
}

static bool diskAdminDiscardsLayout(const g_volume_admin_request& request)
{
	return request.command == G_DISK_WIPE ||
		   (request.command == G_DISK_TABLE_CREATE && (request.flags & G_DISK_TABLE_REPLACE));
}

/**
 * Revalidate the observed geometry and partition metadata under the disk
 * reservation before performing an edit. Replacement jobs still check the MBR.
 */
static g_volume_status diskAdminCheckSnapshot(g_volume_admin_job* job, const g_volume_storage_io& io)
{
	uint8_t mbr[G_VOLUME_SECTOR_SIZE];
	if(!diskAdminRead(job, 0, mbr, 1))
		return G_VOLUME_IO_ERROR;

	if(memcmp(mbr, job->snapshot.mbr, sizeof(mbr)))
		return G_VOLUME_CONFLICT;

	if(diskAdminDiscardsLayout(job->request))
		return G_VOLUME_OK;

	g_volume_disk_snapshot current;
	std::vector<g_volume_info> volumes;

	auto status = diskInspect(job->snapshot.disk, io, &current, &volumes);
	if(status != G_VOLUME_OK)
		return status;
	if(current.disk.layout != job->snapshot.disk.layout || current.partitions.size() != job->snapshot.partitions.size())
		return G_VOLUME_CONFLICT;

	for(size_t i = 0; i < current.partitions.size(); ++i)
	{
		const auto& actual = current.partitions[i];
		const auto& expected = job->snapshot.partitions[i];
		if(actual.number != expected.number || actual.firstSector != expected.firstSector ||
		   actual.sectorCount != expected.sectorCount || actual.type != expected.type ||
		   memcmp(actual.typeGuid, expected.typeGuid, sizeof(actual.typeGuid)))
			return G_VOLUME_CONFLICT;
	}
	return G_VOLUME_OK;
}

static g_volume_status storageManagerAdminExecute(g_volume_admin_job* job, const g_volume_storage_io& io)
{
	const auto& request = job->request;
	const auto& disk = job->snapshot.disk;
	switch(request.command)
	{
		case G_VOLUME_FORMAT:
			return volumeFormat(job);
		case G_DISK_TABLE_CREATE:
			return diskTableCreate(job->snapshot, io, request.flags & G_DISK_TABLE_REPLACE);
		case G_DISK_PARTITION_CREATE:
			return diskPartitionCreate(job->snapshot, request, io, &job->createdPartition);
		case G_DISK_PARTITION_DELETE:
			return diskPartitionDelete(job->snapshot, request.partition, io);
		case G_DISK_WIPE:
			return diskZero(disk, 0, disk.sectorCount, io);
		case G_VOLUME_WIPE:
			return diskZero(disk, request.firstSector, request.sectorCount, io);
		case G_DISK_BOOT_WRITE:
			return diskBootWrite(job->snapshot, request, io);
		case G_DISK_BOOT_COMMIT:
			return diskBootCommit(job->snapshot, request, io);
		default:
			return G_VOLUME_INVALID;
	}
}

static void storageManagerAdminRun(void* argument)
{
	auto job = static_cast<g_volume_admin_job*>(argument);
	job->lastPause = g_millis();

	g_block_device_info actual{};
	const auto& disk = job->snapshot.disk;
	if(blockDeviceGetInfo(disk.driver, disk.device, &actual) != G_BLOCK_SUCCESS)
	{
		job->result = G_VOLUME_IO_ERROR;
	}
	else if(actual.sectorSize != disk.sectorSize || actual.sectorCount != disk.sectorCount ||
			strcmp(actual.serial, disk.serial))
	{
		job->result = G_VOLUME_CONFLICT;
	}
	else if(blockDeviceOpenBuffer(disk.driver, &job->buffer) != G_BLOCK_SUCCESS)
	{
		job->result = G_VOLUME_IO_ERROR;
	}
	else
	{
		g_volume_storage_io io{job,
							   diskAdminRead,
							   diskAdminWrite,
							   diskAdminFlush,
							   storageManagerAdminProgress,
							   std::min(actual.maxTransferSectors, G_VOLUME_MAX_TRANSFER_SECTORS)};
		job->result = diskAdminCheckSnapshot(job, io);
		if(job->result == G_VOLUME_OK)
			job->result = storageManagerAdminExecute(job, io);
	}

	if(job->buffer.handle)
	{
		auto closed = blockDeviceCloseBuffer(&job->buffer);
		if(closed != G_BLOCK_SUCCESS)
			closed = blockDeviceCloseBuffer(&job->buffer);
		if(closed != G_BLOCK_SUCCESS)
		{
			job->ioSafe = false;
			job->result = G_VOLUME_IO_ERROR;
		}
	}

	__atomic_store_n(&job->finished, true, __ATOMIC_RELEASE);
}

static g_volume_job_info storageManagerAdminJobInfo(const g_volume_admin_job* job)
{
	auto info = job->info;
	info.completed = __atomic_load_n(&job->completed, __ATOMIC_RELAXED);
	info.total = __atomic_load_n(&job->total, __ATOMIC_RELAXED);
	info.phase = g_volume_job_phase(__atomic_load_n(&job->phase, __ATOMIC_RELAXED));
	return info;
}

static bool storageManagerAdminSameOperation(const g_volume_admin_request& a, const g_volume_admin_request& b)
{
	return a.command == b.command && a.device == b.device && a.volume == b.volume && a.partition == b.partition &&
		   a.generation == b.generation && a.firstSector == b.firstSector && a.sectorCount == b.sectorCount &&
		   a.alignment == b.alignment && a.flags == b.flags && a.type == b.type && a.slot == b.slot &&
		   a.layout == b.layout && a.claim == b.claim && a.diskIdentity == b.diskIdentity && a.dataSize == b.dataSize &&
		   !memcmp(a.data, b.data, sizeof(a.data)) && !memcmp(a.filesystem, b.filesystem, sizeof(a.filesystem)) &&
		   !memcmp(a.label, b.label, sizeof(a.label));
}

static g_volume_status diskAdminPrepareBootRequest(g_volume_registry* registry, g_tid sender,
												   g_volume_admin_request* request)
{
	auto found = registry->disks.find(request->device);
	if(found == registry->disks.end())
		return G_VOLUME_NOT_FOUND;

	const auto& disk = found->second;

	if(!request->claim || disk.owner != sender || disk.claim != request->claim || disk.allowedVolume != request->volume)
		return G_VOLUME_NOT_OWNER;

	if(registry->diskBusy(request->device, sender))
		return G_VOLUME_BUSY;

	if((disk.snapshot.disk.flags & (G_BLOCK_WRITABLE | G_BLOCK_FLUSH_SUPPORTED)) !=
	   (G_BLOCK_WRITABLE | G_BLOCK_FLUSH_SUPPORTED))
		return G_VOLUME_UNSUPPORTED;

	auto volume = registry->volumes.find(request->volume);
	if(volume == registry->volumes.end() || !volume->second.active || volume->second.info.device != request->device)
		return G_VOLUME_NOT_FOUND;

	request->partition = volume->second.info.partition;
	g_disk_boot_info boot;
	auto status = diskBootInfo(disk.snapshot, *request, &boot);
	if(status != G_VOLUME_OK)
		return status;

	if(boot.firstSector != volume->second.info.firstSector || boot.sectorCount != volume->second.info.sectorCount)
		return G_VOLUME_CONFLICT;

	return G_VOLUME_OK;
}

static g_volume_status storageManagerAdminStartJob(g_volume_registry* registry, g_tid sender,
												   g_volume_admin_request request, g_volume_admin_response* response)
{
	if(!request.operation)
		return G_VOLUME_INVALID;

	auto duplicate = storageManagerAdminOperations.find({sender, request.operation});
	if(duplicate != storageManagerAdminOperations.end())
	{
		auto job = storageManagerAdminJobs[duplicate->second];
		if(!storageManagerAdminSameOperation(request, job->original))
			return G_VOLUME_CONFLICT;
		response->job = storageManagerAdminJobInfo(job);
		return G_VOLUME_OK;
	}

	if(storageManagerAdminQuiescing())
		return G_VOLUME_BUSY;

	auto original = request;
	if(request.command == G_DISK_BOOT_WRITE || request.command == G_DISK_BOOT_COMMIT)
	{
		auto status = diskAdminPrepareBootRequest(registry, sender, &request);
		if(status != G_VOLUME_OK)
			return status;
	}

	if(request.command == G_VOLUME_FORMAT || request.command == G_VOLUME_WIPE)
	{
		auto volume = registry->volumes.find(request.volume);
		if(volume == registry->volumes.end() || !volume->second.active)
			return G_VOLUME_NOT_FOUND;
		request.device = volume->second.info.device;
		request.firstSector = volume->second.info.firstSector;
		request.sectorCount = volume->second.info.sectorCount;
	}

	auto found = registry->disks.find(request.device);
	if(found == registry->disks.end())
		return G_VOLUME_NOT_FOUND;

	auto& disk = found->second;
	if(request.generation != disk.snapshot.disk.generation || !request.generation)
		return G_VOLUME_CONFLICT;

	if(disk.snapshot.disk.sectorSize != G_VOLUME_SECTOR_SIZE)
		return G_VOLUME_UNSUPPORTED;

	if(!diskAdminDiscardsLayout(request) &&
	   (disk.snapshot.disk.layout == G_DISK_INVALID || disk.snapshot.disk.layout == G_DISK_UNSUPPORTED))
		return G_VOLUME_UNSUPPORTED;

	if(registry->diskBusy(request.device, sender))
		return G_VOLUME_BUSY;

	if((disk.snapshot.disk.flags & (G_BLOCK_WRITABLE | G_BLOCK_FLUSH_SUPPORTED)) !=
	   (G_BLOCK_WRITABLE | G_BLOCK_FLUSH_SUPPORTED))
		return G_VOLUME_UNSUPPORTED;

	if(request.command == G_DISK_TABLE_CREATE && request.layout != G_DISK_MBR)
		return G_VOLUME_UNSUPPORTED;

	if(request.command == G_DISK_TABLE_CREATE && (request.flags & ~G_DISK_TABLE_REPLACE))
		return G_VOLUME_INVALID;

	g_tid formatter = G_TID_NONE;
	if(request.command == G_VOLUME_FORMAT)
	{
		if(!memchr(request.filesystem, 0, sizeof(request.filesystem)) ||
		   !memchr(request.label, 0, sizeof(request.label)))
			return G_VOLUME_INVALID;
		auto selected = storageManagerFormatters.find(request.filesystem);
		if(selected == storageManagerFormatters.end() || g_get_pid_for_tid(selected->second) == G_PID_NONE)
			return G_VOLUME_UNSUPPORTED;
		formatter = selected->second;
		if(registry->volumes[request.volume].info.scheme == G_VOLUME_WHOLE_DISK &&
		   disk.snapshot.disk.layout != G_DISK_NONE)
			return G_VOLUME_CONFLICT;
	}


	auto job = new(std::nothrow) g_volume_admin_job;
	if(!job)
		return G_VOLUME_UNAVAILABLE;
	job->info.id = storageManagerNextJob++;
	job->info.command = request.command;
	job->info.device = request.device;
	job->info.volume = request.volume;
	job->request = request;
	job->snapshot = disk.snapshot;
	job->original = original;
	job->client = sender;
	job->formatter = formatter;
	disk.job = job->info.id;
	job->thread = g_create_task_d(reinterpret_cast<void*>(storageManagerAdminRun), job);
	if(job->thread == G_TID_NONE)
	{
		disk.job = 0;
		delete job;
		return G_VOLUME_UNAVAILABLE;
	}
	storageManagerAdminJobs[job->info.id] = job;
	storageManagerAdminOperations[{sender, request.operation}] = job->info.id;
	response->job = storageManagerAdminJobInfo(job);
	return G_VOLUME_OK;
}

static g_volume_status storageManagerAdminRegisterFormatter(g_tid sender, const g_volume_admin_request& request)
{
	if(!memchr(request.filesystem, 0, sizeof(request.filesystem)) || strcmp(request.filesystem, "fat32"))
		return G_VOLUME_UNSUPPORTED;

	auto formatter = storageManagerFormatters.find(request.filesystem);
	if(formatter != storageManagerFormatters.end() && formatter->second != sender &&
	   g_get_pid_for_tid(formatter->second) != G_PID_NONE)
		return G_VOLUME_CONFLICT;

	storageManagerFormatters[request.filesystem] = sender;
	return G_VOLUME_OK;
}

static g_volume_admin_response diskAdminQuery(g_volume_registry* registry, g_tid sender,
											  const g_volume_admin_request& request)
{
	g_volume_admin_response response;
	response.command = request.command;
	response.status = G_VOLUME_NOT_FOUND;
	auto disk = request.command == G_DISK_NEXT ? registry->disks.upper_bound(request.device)
											   : registry->disks.find(request.device);
	if(disk == registry->disks.end())
		return response;
	response.disk = disk->second.snapshot.disk;
	response.disk.activeJob = disk->second.job;

	if(request.command == G_DISK_BOOT_GET)
	{
		auto bootRequest = request;
		response.status = diskAdminPrepareBootRequest(registry, sender, &bootRequest);
		if(response.status == G_VOLUME_OK)
			response.status = diskBootInfo(disk->second.snapshot, bootRequest, &response.boot);
		return response;
	}
	if(request.command == G_DISK_GET || request.command == G_DISK_NEXT)
	{
		response.status = G_VOLUME_OK;
		return response;
	}
	if(request.command != G_DISK_PARTITION_NEXT)
	{
		response.status = G_VOLUME_INVALID;
		return response;
	}
	for(auto part: disk->second.snapshot.partitions)
	{
		if(part.number <= request.partition)
			continue;
		for(const auto& entry: registry->volumes)
		{
			const auto& volume = entry.second;
			if(volume.active && volume.info.device == disk->first && volume.info.partition == part.number)
			{
				part.volume = volume.info.id;
				response.volume = volume.info;
				break;
			}
		}
		response.partition = part;
		response.status = G_VOLUME_OK;
		break;
	}
	return response;
}

bool storageManagerAdminReceive(g_volume_registry* registry, const g_message_header* message, const void* content)
{
	if(storageManagerAdminReceivePower(message, content))
		return true;

	if(message->length != sizeof(g_volume_admin_request))
		return false;

	g_volume_admin_request request;
	memcpy(&request, content, sizeof(request));
	if(request.magic != G_VOLUME_ADMIN_MAGIC)
		return false;

	g_volume_admin_response response;
	response.command = request.command;
	if(request.version != G_VOLUME_ADMIN_VERSION || request.reserved || message->sender == G_TID_NONE)
	{
		storageManagerAdminReply(message->sender, message->transaction, response);
		return true;
	}

	response.status = G_VOLUME_NOT_FOUND;
	if(request.command == G_VOLUME_FORMATTER_REGISTER)
		response.status = storageManagerAdminRegisterFormatter(message->sender, request);
	else if(request.command == G_VOLUME_JOB_GET)
	{
		auto job = storageManagerAdminJobs.find(request.job);
		if(job != storageManagerAdminJobs.end())
		{
			response.status = G_VOLUME_OK;
			response.job = storageManagerAdminJobInfo(job->second);
		}
	}
	else if((request.command >= G_DISK_TABLE_CREATE && request.command <= G_VOLUME_WIPE) ||
			request.command == G_DISK_BOOT_WRITE || request.command == G_DISK_BOOT_COMMIT)
		response.status = storageManagerAdminStartJob(registry, message->sender, request, &response);
	else
		response = diskAdminQuery(registry, message->sender, request);

	storageManagerAdminReply(message->sender, message->transaction, response);
	return true;
}

void storageManagerAdminPoll(g_volume_registry* registry)
{
	for(auto& entry: storageManagerAdminJobs)
	{
		auto job = entry.second;

		if(job->processed)
			continue;

		if(!__atomic_load_n(&job->finished, __ATOMIC_ACQUIRE))
		{
			if(g_get_pid_for_tid(job->thread) != G_PID_NONE)
				continue;
			job->result = G_VOLUME_IO_ERROR;
			job->ioSafe = false;
		}
		job->processed = true;
		job->info.partition = job->createdPartition;

		auto& disk = registry->disks[job->request.device];
		if(job->ioSafe && job->changed && diskScanner && job->request.command != G_DISK_BOOT_WRITE)
		{
			storageManagerAdminProgress(job, G_VOLUME_JOB_RESCANNING, 0, 1);
			auto status = diskScanner(disk.info, job->client, job->info.id, true);
			if(status != G_VOLUME_OK && job->result == G_VOLUME_OK)
				job->result = status;
			if(status == G_VOLUME_IO_ERROR)
				job->ioSafe = false;
		}

		if(job->ioSafe)
			disk.job = 0;

		job->info.ioSafe = job->ioSafe;
		job->info.result = job->result;
		job->info.state = job->result == G_VOLUME_OK ? G_VOLUME_JOB_SUCCEEDED : G_VOLUME_JOB_FAILED;

		if(job->info.partition)
			for(const auto& volume: registry->volumes)
				if(volume.second.active && volume.second.info.device == job->request.device &&
				   volume.second.info.partition == job->info.partition)
					job->info.volume = volume.second.info.id;
	}
}
