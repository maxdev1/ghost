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

#include "requests.hpp"
#include "boot.hpp"
#include "format.hpp"
#include "registry/registry.hpp"
#include "jobs.hpp"
#include <string.h>

bool adminRequestMatches(const g_volume_admin_request* left, const g_volume_admin_request* right)
{
	if(left->command != right->command || left->operation != right->operation ||
	   left->generation != right->generation || left->device != right->device || left->volume != right->volume ||
	   left->layout != right->layout || left->flags != right->flags || left->partition != right->partition ||
	   left->firstSector != right->firstSector || left->dataSize != right->dataSize)
		return false;

	const g_disk_partition_options* lp = &left->partitionCreate;
	const g_disk_partition_options* rp = &right->partitionCreate;
	if(lp->firstSector != rp->firstSector || lp->sectorCount != rp->sectorCount || lp->alignment != rp->alignment ||
	   lp->flags != rp->flags || lp->type != rp->type || lp->slot != rp->slot)
		return false;

	const g_disk_boot_info* leftBoot = &left->boot;
	const g_disk_boot_info* rightBoot = &right->boot;
	if(leftBoot->device != rightBoot->device || leftBoot->volume != rightBoot->volume ||
	   leftBoot->claim != rightBoot->claim || leftBoot->generation != rightBoot->generation ||
	   leftBoot->diskIdentity != rightBoot->diskIdentity || leftBoot->partition != rightBoot->partition ||
	   leftBoot->firstSector != rightBoot->firstSector || leftBoot->sectorCount != rightBoot->sectorCount ||
	   leftBoot->bootSectors != rightBoot->bootSectors)
		return false;

	return !memcmp(left->filesystem, right->filesystem, sizeof(left->filesystem)) &&
		   !memcmp(left->label, right->label, sizeof(left->label)) &&
		   !memcmp(left->data, right->data, sizeof(left->data));
}

bool adminRequestDiscardsLayout(const g_volume_admin_request* request)
{
	return request->command == G_DISK_WIPE ||
		   (request->command == G_DISK_TABLE_CREATE && (request->flags & G_DISK_TABLE_REPLACE));
}

static g_volume_admin_request adminRequestDecode(const g_volume_admin_header* header)
{
	g_volume_admin_request operation = {};
	operation.command = header->command;
	switch(header->command)
	{
		case G_DISK_TABLE_CREATE:
		{
			const g_disk_table_create_request* request = (const g_disk_table_create_request*) header;
			operation.operation = request->operation;
			operation.generation = request->generation;
			operation.device = request->device;
			operation.layout = request->layout;
			operation.flags = request->flags;
			break;
		}
		case G_DISK_PARTITION_CREATE:
		{
			const g_disk_partition_create_request* request = (const g_disk_partition_create_request*) header;
			operation.operation = request->operation;
			operation.generation = request->generation;
			operation.device = request->device;
			operation.partitionCreate = {request->firstSector, request->sectorCount, request->alignment,
										 request->flags,	   request->type,		 request->slot};
			break;
		}
		case G_DISK_PARTITION_DELETE:
		{
			const g_disk_partition_delete_request* request = (const g_disk_partition_delete_request*) header;
			operation.operation = request->operation;
			operation.generation = request->generation;
			operation.device = request->device;
			operation.partition = request->partition;
			break;
		}
		case G_DISK_WIPE:
		{
			const g_disk_wipe_request* request = (const g_disk_wipe_request*) header;
			operation.operation = request->operation;
			operation.generation = request->generation;
			operation.device = request->device;
			break;
		}
		case G_VOLUME_FORMAT:
		{
			const g_volume_format_request* request = (const g_volume_format_request*) header;
			operation.operation = request->operation;
			operation.generation = request->generation;
			operation.volume = request->volume;
			memcpy(operation.filesystem, request->filesystem, sizeof(operation.filesystem));
			memcpy(operation.label, request->label, sizeof(operation.label));
			break;
		}
		case G_VOLUME_WIPE:
		{
			const g_volume_wipe_request* request = (const g_volume_wipe_request*) header;
			operation.operation = request->operation;
			operation.generation = request->generation;
			operation.volume = request->volume;
			break;
		}
		case G_DISK_BOOT_WRITE:
		{
			const g_disk_boot_write_request* request = (const g_disk_boot_write_request*) header;
			operation.operation = request->operation;
			operation.boot = request->boot;
			operation.firstSector = request->firstSector;
			operation.dataSize = request->dataSize;
			memcpy(operation.data, request->data, sizeof(operation.data));
			break;
		}
		case G_DISK_BOOT_COMMIT:
		{
			const g_disk_boot_commit_request* request = (const g_disk_boot_commit_request*) header;
			operation.operation = request->operation;
			operation.boot = request->boot;
			operation.flags = request->flags;
			operation.dataSize = G_DISK_BOOT_CODE_SIZE;
			memcpy(operation.data, request->code, G_DISK_BOOT_CODE_SIZE);
			break;
		}
		default:
			break;
	}
	return operation;
}

static void adminRequestDescribeDisk(const g_volume_disk* disk, g_volume_admin_response* response)
{
	response->status = G_VOLUME_NOT_FOUND;
	if(disk)
	{
		response->disk = disk->snapshot.disk;
		response->disk.activeJob = disk->job;
		response->status = G_VOLUME_OK;
	}
}

void adminRequestHandle(g_volume_registry* registry, const g_message_header* message,
								const g_volume_admin_header* header)
{
	if(message->length < sizeof(*header) || header->magic != G_VOLUME_ADMIN_MAGIC)
		return;

	g_volume_admin_response response = {};
	response.command = header->command;
	response.status = G_VOLUME_PROTOCOL_ERROR;

	if(header->version != G_VOLUME_ADMIN_VERSION || message->sender == G_TID_NONE ||
	   storageManagerAdminRequestSize(header->command) != message->length)
	{
		g_send_message_tm(message->sender, &response, sizeof(response), message->transaction,
						  G_MESSAGE_SEND_MODE_NON_BLOCKING);
		return;
	}

	switch(header->command)
	{
		case G_DISK_GET:
		{
			const g_disk_get_request* request = (const g_disk_get_request*) header;
			adminRequestDescribeDisk(registryDiskFind(registry, request->device), &response);
			break;
		}
		case G_DISK_NEXT:
		{
			const g_disk_next_request* request = (const g_disk_next_request*) header;
			auto entry = registry->disks.upper_bound(request->device);
			adminRequestDescribeDisk(entry == registry->disks.end() ? NULL : &entry->second, &response);
			break;
		}
		case G_DISK_PARTITION_NEXT:
		{
			const g_disk_partition_next_request* request = (const g_disk_partition_next_request*) header;
			g_volume_disk* disk = registryDiskFind(registry, request->device);
			adminRequestDescribeDisk(disk, &response);
			if(!disk)
				break;
			response.status = G_VOLUME_NOT_FOUND;
			for(uint32_t i = 0; i < disk->snapshot.disk.partitions; ++i)
			{
				const g_disk_partition_info* part = &disk->snapshot.partitions[i];
				if(part->number <= request->partition)
					continue;
				response.partition = *part;
				for(auto& entry: registry->volumes)
				{
					g_volume_record* volume = &entry.second;
					if(volume->active && volume->info.device == request->device &&
					   volume->info.partition == part->number)
					{
						response.partition.volume = volume->info.id;
						response.volume = volume->info;
						break;
					}
				}
				response.status = G_VOLUME_OK;
				break;
			}
			break;
		}
		case G_DISK_BOOT_GET:
		{
			const g_disk_boot_get_request* request = (const g_disk_boot_get_request*) header;
			g_volume_disk* disk = registryDiskFind(registry, request->device);
			adminRequestDescribeDisk(disk, &response);
			if(!disk)
				break;
			g_disk_boot_info boot = {};
			boot.device = request->device;
			boot.volume = request->volume;
			boot.claim = request->claim;
			boot.diskIdentity = request->diskIdentity;
			boot.bootSectors = request->sectorCount;
			response.status = adminBootPrepare(registry, message->sender, &boot);
			if(response.status == G_VOLUME_OK)
				response.status = diskBootGetInfo(&disk->snapshot, &boot, &response.boot);
			break;
		}
		case G_VOLUME_JOB_GET:
		{
			const g_volume_job_get_request* request = (const g_volume_job_get_request*) header;
			g_volume_admin_job* job = adminJobFind(request->job);
			response.status = G_VOLUME_NOT_FOUND;
			if(job)
			{
				response.status = G_VOLUME_OK;
				response.job = adminJobGetInfo(job);
			}
			break;
		}
		case G_VOLUME_FORMATTER_REGISTER:
			response.status = adminFormatRegister(message->sender,
																   (const g_volume_formatter_register_request*) header);
			break;
		case G_DISK_TABLE_CREATE:
		case G_DISK_PARTITION_CREATE:
		case G_DISK_PARTITION_DELETE:
		case G_DISK_WIPE:
		case G_VOLUME_FORMAT:
		case G_VOLUME_WIPE:
		case G_DISK_BOOT_WRITE:
		case G_DISK_BOOT_COMMIT:
		{
			g_volume_admin_request request = adminRequestDecode(header);
			response.status = adminJobStart(registry, message->sender, &request, &response);
			break;
		}
		default:
			break;
	}
	g_send_message_tm(message->sender, &response, sizeof(response), message->transaction,
					  G_MESSAGE_SEND_MODE_NON_BLOCKING);
}
