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

#include "registry.hpp"
#include <ghost/messages.h>
#include <stdio.h>
#include <string.h>

g_volume_record* registryVolumeFind(g_volume_registry* registry, g_volume_id id)
{
	auto entry = registry->volumes.find(id);
	return entry == registry->volumes.end() ? NULL : &entry->second;
}

bool registryVolumeIsPathClaimed(g_volume_registry* registry, const char* path, g_volume_id except)
{
	for(auto& entry : registry->volumes)
	{
		g_volume_record* volume = &entry.second;
		if(volume->active && volume->info.id != except && volume->info.owner != G_TID_NONE &&
		   !strcmp(volume->info.path, path))
			return true;
	}
	return false;
}

static bool registryVolumeMatchesRegion(const g_volume_info* a, const g_volume_info* b)
{
	return a->firstSector == b->firstSector && a->sectorCount == b->sectorCount &&
		   a->partition == b->partition && a->scheme == b->scheme;
}

static bool registryVolumeIsPresent(const g_volume_info* volume, const g_volume_disk_snapshot* snapshot)
{
	if(snapshot->disk.layout == G_DISK_NONE)
		return volume->partition == 0 && volume->scheme == G_VOLUME_WHOLE_DISK && volume->firstSector == 0 &&
			   volume->sectorCount == snapshot->disk.sectorCount;
	if(snapshot->disk.layout != G_DISK_MBR && snapshot->disk.layout != G_DISK_GPT)
		return false;

	for(uint32_t i = 0; i < snapshot->disk.partitions; ++i)
	{
		const g_disk_partition_info* part = &snapshot->partitions[i];
		if(!part->container && volume->partition == part->number && volume->scheme == part->scheme &&
		   volume->firstSector == part->firstSector && volume->sectorCount == part->sectorCount)
			return true;
	}
	return false;
}

static g_volume_status registryVolumePublishEvent(g_volume_event_type type, const g_volume_info* info)
{
	g_volume_event event = {};
	event.type = type;
	event.info = *info;
	if(g_send_topic_message(G_VOLUME_EVENT_TOPIC, &event, sizeof(event)) != G_MESSAGE_SEND_STATUS_SUCCESSFUL)
		return G_VOLUME_UNAVAILABLE;
	return G_VOLUME_OK;
}

g_volume_status registryVolumeReconcileDisk(g_volume_registry* registry, const g_volume_info* info, bool manual)
{
	g_volume_disk* disk = registryDiskFind(registry, info->device);
	if(!disk)
		return G_VOLUME_NOT_FOUND;
	disk->info = *info;
	g_volume_status status = registryVolumePublishEvent(G_VOLUME_DISK_DISCOVERED, info);
	if(status != G_VOLUME_OK)
		return status;

	for(auto& entry : registry->volumes)
	{
		g_volume_record* volume = &entry.second;
		if(!volume->active || volume->info.device != info->device || registryVolumeIsPresent(&volume->info, &disk->snapshot))
			continue;

		volume->active = false;
		status = registryVolumePublishEvent(G_VOLUME_REMOVED, &volume->info);
		if(status != G_VOLUME_OK)
			return status;
	}

	const g_volume_disk_snapshot* snapshot = &disk->snapshot;
	uint32_t count = snapshot->disk.partitions;
	if(snapshot->disk.layout == G_DISK_NONE)
		count = 1;
	for(uint32_t i = 0; i < count; ++i)
	{
		g_volume_info candidate = *info;
		if(snapshot->disk.layout == G_DISK_NONE)
		{
			candidate.firstSector = 0;
			candidate.sectorCount = snapshot->disk.sectorCount;
			candidate.partition = 0;
			candidate.scheme = G_VOLUME_WHOLE_DISK;
		}
		else
		{
			const g_disk_partition_info* part = &snapshot->partitions[i];
			if(part->container)
				continue;
			candidate.firstSector = part->firstSector;
			candidate.sectorCount = part->sectorCount;
			candidate.partition = part->number;
			candidate.scheme = part->scheme;
		}

		g_volume_record* record = NULL;
		for(auto& entry : registry->volumes)
		{
			g_volume_record* volume = &entry.second;
			if(volume->active && volume->info.device == info->device && registryVolumeMatchesRegion(&volume->info, &candidate))
			{
				record = volume;
				break;
			}
		}
		if(!record)
		{
			candidate.id = ++registry->nextVolume;
			record = &registry->volumes[candidate.id];
			record->info = candidate;
			record->active = true;
			record->releasedOwner = G_TID_NONE;
		}
		if(manual)
		{
			record->manual = true;
			record->info.handler = G_TID_NONE;
			record->info.filesystem[0] = 0;
		}
		status = registryVolumePublishEvent(manual ? G_VOLUME_PROBE : G_VOLUME_DISCOVERED, &record->info);
		if(status != G_VOLUME_OK)
			return status;
	}
	return G_VOLUME_OK;
}

static bool registryVolumeIsValidName(const char* name, size_t capacity)
{
	if(!*name || !memchr(name, 0, capacity) || !strcmp(name, ".") || !strcmp(name, ".."))
		return false;
	for(const char* c = name; *c; ++c)
	{
		bool letter = (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z');
		bool digit = *c >= '0' && *c <= '9';
		if(!letter && !digit && *c != '-' && *c != '_' && *c != '.')
			return false;
	}
	return true;
}

void registryVolumeClearOwner(g_volume_info* info)
{
	info->owner = G_TID_NONE;
	info->claim = 0;
	info->mount = -1;
	info->state = G_VOLUME_AVAILABLE;
	info->path[0] = 0;
}

static g_volume_record* registryVolumePrepareResponse(g_volume_registry* registry, g_volume_id id,
											g_volume_command command, g_volume_response* response)
{
	response->command = command;
	g_volume_record* volume = registryVolumeFind(registry, id);
	if(!volume || !volume->active)
	{
		response->status = G_VOLUME_NOT_FOUND;
		return NULL;
	}
	response->status = G_VOLUME_OK;
	response->info = volume->info;
	return volume;
}

g_volume_response registryVolumeHandleGet(g_volume_registry* registry, const g_volume_get_request* request)
{
	g_volume_response response = {};
	registryVolumePrepareResponse(registry, request->volume, G_VOLUME_GET, &response);
	return response;
}

g_volume_response registryVolumeHandleList(g_volume_registry* registry, const g_volume_list_request* request)
{
	g_volume_response response = {};
	response.command = G_VOLUME_LIST;
	response.status = G_VOLUME_NOT_FOUND;
	for(auto entry = registry->volumes.upper_bound(request->volume); entry != registry->volumes.end(); ++entry)
	{
		g_volume_record* volume = &entry->second;
		if(!volume->active || (request->device && volume->info.device != request->device))
			continue;
		response.status = G_VOLUME_OK;
		response.info = volume->info;
		break;
	}
	return response;
}

g_volume_response registryVolumeHandleClaim(g_volume_registry* registry, g_tid sender, const g_volume_claim_request* request)
{
	g_volume_response response = {};
	g_volume_record* volume = registryVolumePrepareResponse(registry, request->volume, G_VOLUME_CLAIM, &response);
	if(!volume)
		return response;

	g_volume_info* info = &volume->info;
	if(!registryVolumeIsValidName(request->filesystem, sizeof(request->filesystem)) || !registryVolumeIsValidName(request->name, sizeof(request->name)))
	{
		response.status = G_VOLUME_INVALID;
		return response;
	}
	char path[64];
	if(!strcmp(request->name, "fat-root"))
		strcpy(path, "/");
	else
		snprintf(path, sizeof(path), "/mount/%s", request->name);

	if(info->owner != G_TID_NONE)
	{
		if(info->owner != sender || strcmp(info->filesystem, request->filesystem) || strcmp(info->path, path))
			response.status = G_VOLUME_CONFLICT;
		return response;
	}
	g_volume_disk* disk = registryDiskFind(registry, info->device);
	if(!disk || disk->job || (disk->owner != G_TID_NONE &&
	   (disk->allowedVolume != info->id || (info->handler != G_TID_NONE && info->handler != sender))) ||
	   registryVolumeIsPathClaimed(registry, path, 0))
	{
		response.status = G_VOLUME_CONFLICT;
		return response;
	}

	info->owner = sender;
	info->handler = sender;
	info->claim = ++registry->nextClaim;
	info->state = G_VOLUME_MOUNTING;
	strcpy(info->filesystem, request->filesystem);
	strcpy(info->path, path);
	response.info = *info;
	return response;
}

g_volume_response registryVolumeHandleCommit(g_volume_registry* registry, g_tid sender, const g_volume_commit_request* request)
{
	g_volume_response response = {};
	g_volume_record* volume = registryVolumePrepareResponse(registry, request->volume, G_VOLUME_COMMIT, &response);
	if(!volume)
		return response;
	g_volume_info* info = &volume->info;
	if(info->owner != sender || !request->claim || info->claim != request->claim)
		response.status = G_VOLUME_NOT_OWNER;
	else if(request->mount < 0 || (info->state == G_VOLUME_MOUNTED && info->mount != request->mount))
		response.status = G_VOLUME_INVALID;
	else
	{
		info->mount = request->mount;
		info->state = G_VOLUME_MOUNTED;
	}
	response.info = *info;
	return response;
}

g_volume_response registryVolumeHandleRelease(g_volume_registry* registry, g_tid sender, const g_volume_release_request* request)
{
	g_volume_response response = {};
	g_volume_record* volume = registryVolumePrepareResponse(registry, request->volume, G_VOLUME_RELEASE, &response);
	if(!volume)
		return response;
	g_volume_info* info = &volume->info;
	if(info->owner == sender && request->claim && info->claim == request->claim)
	{
		volume->releasedClaim = info->claim;
		volume->releasedOwner = sender;
		registryVolumeClearOwner(info);
	}
	else if(volume->releasedOwner != sender || !request->claim || volume->releasedClaim != request->claim)
		response.status = G_VOLUME_NOT_OWNER;
	response.info = *info;
	return response;
}

g_volume_response registryVolumeHandleOffer(g_volume_registry* registry, g_tid sender, const g_volume_offer_request* request)
{
	g_volume_response response = {};
	g_volume_record* volume = registryVolumePrepareResponse(registry, request->volume, G_VOLUME_OFFER, &response);
	if(!volume)
		return response;
	if(!registryVolumeIsValidName(request->filesystem, sizeof(request->filesystem)))
		response.status = G_VOLUME_INVALID;
	else if(volume->info.owner == G_TID_NONE && volume->info.handler == G_TID_NONE)
	{
		volume->info.handler = sender;
		strcpy(volume->info.filesystem, request->filesystem);
	}
	response.info = volume->info;
	return response;
}
