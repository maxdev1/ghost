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

#include <libstorage/admin.hpp>
#include "client.hpp"

static bool storageManagerAdminResponseValid(const g_volume_admin_request& request,
											 const g_volume_admin_response& reply)
{
	const auto& disk = reply.disk;
	const auto& job = reply.job;
	if(reply.magic != G_VOLUME_ADMIN_MAGIC || reply.version != G_VOLUME_ADMIN_VERSION ||
	   reply.command != request.command || reply.status > G_VOLUME_PROTOCOL_ERROR || disk.layout > G_DISK_UNSUPPORTED ||
	   job.state > G_VOLUME_JOB_FAILED || job.phase > G_VOLUME_JOB_RESCANNING || job.result > G_VOLUME_PROTOCOL_ERROR ||
	   job.ioSafe > 1 || disk.model[sizeof(disk.model) - 1] || disk.serial[sizeof(disk.serial) - 1])
		return false;
	if(reply.status != G_VOLUME_OK)
		return true;

	switch(request.command)
	{
		case G_DISK_GET:
			return disk.device == request.device;
		case G_DISK_NEXT:
			return disk.device > request.device;
		case G_DISK_PARTITION_NEXT:
			return disk.device == request.device && reply.partition.device == request.device &&
				   reply.partition.number > request.partition;
		case G_VOLUME_JOB_GET:
			return job.id == request.job;
		case G_DISK_BOOT_GET:
			return disk.device == request.device && reply.boot.device == request.device &&
				   reply.boot.volume == request.volume && reply.boot.claim == request.claim && reply.boot.generation &&
				   reply.boot.generation == disk.generation && reply.boot.diskIdentity && reply.boot.partition >= 1 &&
				   reply.boot.partition <= 4 && reply.boot.firstSector && reply.boot.firstSector < disk.sectorCount &&
				   reply.boot.sectorCount && reply.boot.sectorCount <= disk.sectorCount - reply.boot.firstSector &&
				   reply.boot.bootSectors == request.sectorCount && reply.boot.bootSectors <= reply.boot.firstSector;
		default:
			return true;
	}
}

g_volume_status storageManagerAdminRequest(const g_volume_admin_request& request, g_volume_admin_response* response)
{
	static_assert(sizeof(g_volume_admin_response) <= G_MESSAGE_MAXIMUM_MESSAGE_LENGTH, "admin response fits IPC");
	if(!response || request.magic != G_VOLUME_ADMIN_MAGIC || request.version != G_VOLUME_ADMIN_VERSION ||
	   request.reserved || request.command < G_DISK_GET || request.command > G_DISK_BOOT_COMMIT)
		return G_VOLUME_INVALID;
	*response = {};

	g_volume_exchange exchange;
	auto status = storageManagerClientSend(request, exchange);
	if(status != G_VOLUME_OK)
		return status;

	g_volume_admin_response reply;
	status = storageManagerClientReceive(exchange, &reply, sizeof(reply));
	if(status != G_VOLUME_OK)
		return status;
	if(!storageManagerAdminResponseValid(request, reply))
		return G_VOLUME_PROTOCOL_ERROR;
	*response = reply;
	return reply.status;
}

g_volume_status diskGet(g_device_id device, g_disk_info* info)
{
	if(!device || !info)
		return G_VOLUME_INVALID;
	g_volume_admin_request request;
	request.device = device;
	g_volume_admin_response response;
	auto status = storageManagerAdminRequest(request, &response);
	if(status == G_VOLUME_OK)
		*info = response.disk;
	return status;
}

g_volume_status diskNext(g_device_id after, g_disk_info* info)
{
	if(!info)
		return G_VOLUME_INVALID;

	g_volume_admin_request request;
	request.command = G_DISK_NEXT;
	request.device = after;

	g_volume_admin_response response;
	auto status = storageManagerAdminRequest(request, &response);

	if(status == G_VOLUME_OK)
		*info = response.disk;
	return status;
}

g_volume_status diskPartitionNext(g_device_id device, uint32_t after, g_disk_partition_info* info)
{
	if(!device || !info)
		return G_VOLUME_INVALID;

	g_volume_admin_request request;
	request.command = G_DISK_PARTITION_NEXT;
	request.device = device;
	request.partition = after;

	g_volume_admin_response response;
	auto status = storageManagerAdminRequest(request, &response);

	if(status == G_VOLUME_OK)
		*info = response.partition;
	return status;
}

g_volume_status storageManagerJobGet(g_volume_job_id job, g_volume_job_info* info)
{
	if(!job || !info)
		return G_VOLUME_INVALID;

	g_volume_admin_request request;
	request.command = G_VOLUME_JOB_GET;
	request.job = job;

	g_volume_admin_response response;
	auto status = storageManagerAdminRequest(request, &response);

	if(status == G_VOLUME_OK)
		*info = response.job;
	return status;
}

g_volume_status storageManagerAdminStart(g_volume_admin_request request, g_volume_job_info* info)
{
	if(!info || !((request.command >= G_DISK_TABLE_CREATE && request.command <= G_VOLUME_WIPE) ||
				  request.command == G_DISK_BOOT_WRITE || request.command == G_DISK_BOOT_COMMIT))
		return G_VOLUME_INVALID;

	if(!request.operation)
		request.operation = uint32_t(g_get_message_tx_id()) + 1ull;

	g_volume_admin_response response;
	auto status = storageManagerAdminRequest(request, &response);

	if(status == G_VOLUME_OK && (!response.job.id || response.job.command != request.command))
		return G_VOLUME_PROTOCOL_ERROR;
	if(status == G_VOLUME_OK && (request.command == G_DISK_BOOT_WRITE || request.command == G_DISK_BOOT_COMMIT) &&
	   (response.job.device != request.device || response.job.volume != request.volume))
		return G_VOLUME_PROTOCOL_ERROR;
	if(status == G_VOLUME_OK)
		*info = response.job;
	return status;
}

g_volume_status storageManagerFormatterRegister(const char* filesystem)
{
	if(!filesystem || !*filesystem || strlen(filesystem) >= sizeof(g_volume_admin_request{}.filesystem))
		return G_VOLUME_INVALID;

	g_volume_admin_request request;
	request.command = G_VOLUME_FORMATTER_REGISTER;
	strcpy(request.filesystem, filesystem);
	g_volume_admin_response response;
	return storageManagerAdminRequest(request, &response);
}
