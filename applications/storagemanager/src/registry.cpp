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
#include <cstdio>
#include <cstring>

bool g_volume_registry::diskBusy(g_device_id device, g_tid requester, g_volume_job_id job) const
{
	auto disk = disks.find(device);
	if(disk == disks.end())
		return true;

	if(disk->second.job && disk->second.job != job)
		return true;

	if(disk->second.owner != G_TID_NONE && disk->second.owner != requester)
		return true;

	for(const auto& entry: volumes)
	{
		const auto& record = entry.second;
		if(record.active && record.info.device == device && (record.info.owner != G_TID_NONE || record.controlPending))
			return true;
	}
	return false;
}

bool g_volume_registry::volumePathClaimed(const char* path, g_volume_id except) const
{
	for(const auto& entry: volumes)
	{
		const auto& record = entry.second;
		if(record.active && (!except || record.info.id != except) && record.info.owner != G_TID_NONE &&
		   !strcmp(record.info.path, path))
			return true;
	}
	return false;
}

static bool volumeSamePartition(const g_volume_info& a, const g_volume_info& b)
{
	return a.firstSector == b.firstSector && a.sectorCount == b.sectorCount && a.partition == b.partition &&
		   a.scheme == b.scheme;
}

std::vector<g_volume_event> g_volume_registry::diskReplace(const g_volume_info& disk,
														   const std::vector<g_volume_info>& discovered)
{
	std::vector<g_volume_event> events;
	disks[disk.device].info = disk;
	g_volume_event diskEvent;
	diskEvent.type = G_VOLUME_DISK_DISCOVERED;
	diskEvent.info = disk;
	events.push_back(diskEvent);

	for(auto& entry: volumes)
	{
		auto& record = entry.second;
		if(!record.active || record.info.device != disk.device)
			continue;
		bool exists = false;
		for(const auto& candidate: discovered)
			exists |= volumeSamePartition(record.info, candidate);
		if(!exists)
		{
			record.active = false;
			g_volume_event event;
			event.type = G_VOLUME_REMOVED;
			event.info = record.info;
			events.push_back(event);
		}
	}

	for(auto candidate: discovered)
	{
		g_volume_record* found = nullptr;
		for(auto& entry: volumes)
		{
			auto& record = entry.second;
			if(record.active && record.info.device == disk.device && volumeSamePartition(record.info, candidate))
			{
				found = &record;
				break;
			}
		}
		if(!found)
		{
			candidate.id = nextVolume++;
			found = &volumes[candidate.id];
			found->info = candidate;
		}
		g_volume_event event;
		event.info = found->info;
		events.push_back(event);
	}
	return events;
}

static bool volumeValidName(const char* name, size_t capacity)
{
	if(!*name || !memchr(name, 0, capacity) || !strcmp(name, ".") || !strcmp(name, ".."))
		return false;
	for(const char* c = name; *c; ++c)
	{
		if(!((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || *c == '-' ||
			 *c == '_' || *c == '.'))
			return false;
	}
	return true;
}

static void volumeClearOwner(g_volume_info& info)
{
	info.owner = G_TID_NONE;
	info.claim = 0;
	info.mount = -1;
	info.state = G_VOLUME_AVAILABLE;
	info.path[0] = 0;
}

static void diskClearOwner(g_volume_disk& disk)
{
	disk.owner = G_TID_NONE;
	disk.claim = 0;
	disk.allowedVolume = 0;
}

g_volume_response g_volume_registry::diskHandleRequest(g_tid sender, const g_volume_request& request)
{
	g_volume_response response;
	response.command = request.command;
	auto disk = disks.find(request.device);
	if(disk == disks.end())
	{
		response.status = G_VOLUME_NOT_FOUND;
		return response;
	}

	auto& record = disk->second;
	response.info = record.info;
	if(request.command == G_VOLUME_CLAIM_DISK)
	{
		auto volume = volumes.find(request.volume);
		if(request.volume &&
		   (volume == volumes.end() || !volume->second.active || volume->second.info.device != request.device))
			response.status = G_VOLUME_INVALID;
		else if(diskBusy(request.device, sender))
			response.status = G_VOLUME_CONFLICT;
		else
		{
			if(record.owner == G_TID_NONE)
			{
				record.owner = sender;
				record.claim = nextClaim++;
			}
			record.allowedVolume = request.volume;
			response.status = G_VOLUME_OK;
		}
	}
	else if(record.owner == sender && record.claim == request.claim && request.claim)
	{
		record.releasedClaim = record.claim;
		record.releasedOwner = sender;
		diskClearOwner(record);
		response.status = G_VOLUME_OK;
	}
	else
	{
		response.status = record.releasedOwner == sender && record.releasedClaim == request.claim && request.claim
								  ? G_VOLUME_OK
								  : G_VOLUME_NOT_OWNER;
	}

	response.info.owner = record.owner;
	response.info.claim = record.claim;
	return response;
}

g_volume_status g_volume_registry::volumeClaim(g_tid sender, const g_volume_request& request, g_volume_info& info)
{
	if(!volumeValidName(request.filesystem, sizeof(request.filesystem)) ||
	   !volumeValidName(request.name, sizeof(request.name)))
		return G_VOLUME_INVALID;
	char path[64];
	if(!strcmp(request.name, "fat-root"))
		strcpy(path, "/");
	else
		snprintf(path, sizeof(path), "/mount/%s", request.name);
	if(info.owner != G_TID_NONE)
	{
		return info.owner == sender && !strcmp(info.filesystem, request.filesystem) && !strcmp(info.path, path)
					   ? G_VOLUME_OK
					   : G_VOLUME_CONFLICT;
	}
	if(info.handler == G_TID_NONE)
	{
		info.handler = sender;
		strcpy(info.filesystem, request.filesystem);
	}
	auto& disk = disks[info.device];
	if(disk.job || (disk.owner != G_TID_NONE && (disk.allowedVolume != info.id || info.handler != sender)))
		return G_VOLUME_CONFLICT;
	if(volumePathClaimed(path))
		return G_VOLUME_CONFLICT;

	info.owner = sender;
	info.handler = sender;
	info.claim = nextClaim++;
	info.state = G_VOLUME_MOUNTING;
	strcpy(info.filesystem, request.filesystem);
	strcpy(info.path, path);
	return G_VOLUME_OK;
}

g_volume_response g_volume_registry::storageManagerHandleRequest(g_tid sender, const g_volume_request& request)
{
	g_volume_response response;
	response.command = request.command;

	if(sender == G_TID_NONE || request.magic != G_VOLUME_MAGIC || request.version != G_VOLUME_VERSION ||
	   request.reserved || request.command < G_VOLUME_GET || request.command > G_VOLUME_STAT)
		return response;

	if(request.command == G_VOLUME_CLAIM_DISK || request.command == G_VOLUME_RELEASE_DISK)
		return diskHandleRequest(sender, request);

	if(request.command == G_VOLUME_LIST)
	{
		for(auto it = volumes.upper_bound(request.volume); it != volumes.end(); ++it)
		{
			if(it->second.active && (!request.device || it->second.info.device == request.device))
			{
				response.status = G_VOLUME_OK;
				response.info = it->second.info;
				return response;
			}
		}
		response.status = G_VOLUME_NOT_FOUND;
		return response;
	}

	auto it = volumes.find(request.volume);
	if(it == volumes.end() || !it->second.active)
	{
		response.status = G_VOLUME_NOT_FOUND;
		return response;
	}

	auto& record = it->second;
	auto& info = record.info;
	response.status = G_VOLUME_OK;
	switch(request.command)
	{
		case G_VOLUME_GET:
			break;
		case G_VOLUME_OFFER:
			if(!volumeValidName(request.filesystem, sizeof(request.filesystem)))
			{
				response.status = G_VOLUME_INVALID;
				break;
			}
			if(info.owner == G_TID_NONE && info.handler == G_TID_NONE)
			{
				info.handler = sender;
				strcpy(info.filesystem, request.filesystem);
			}
			break;
		case G_VOLUME_CLAIM:
			response.status = volumeClaim(sender, request, info);
			break;
		case G_VOLUME_COMMIT:
			if(info.owner != sender || !request.claim || info.claim != request.claim)
				response.status = G_VOLUME_NOT_OWNER;
			else if(request.mount < 0 || (info.state == G_VOLUME_MOUNTED && info.mount != request.mount))
				response.status = G_VOLUME_INVALID;
			else
			{
				info.mount = request.mount;
				info.state = G_VOLUME_MOUNTED;
			}
			break;
		case G_VOLUME_RELEASE:
			if(info.owner == sender && request.claim && info.claim == request.claim)
			{
				record.releasedClaim = info.claim;
				record.releasedOwner = sender;
				volumeClearOwner(info);
			}
			else if(record.releasedOwner != sender || !request.claim || record.releasedClaim != request.claim)
				response.status = G_VOLUME_NOT_OWNER;
			break;
		default:
			response.status = G_VOLUME_INVALID;
			break;
	}

	response.info = info;
	return response;
}

void g_volume_registry::storageManagerForgetOwner(g_tid owner)
{
	for(auto& entry: volumes)
	{
		auto& info = entry.second.info;
		if(info.handler == owner)
			info.handler = G_TID_NONE;
		if(info.owner == owner)
			volumeClearOwner(info);
	}
	for(auto& entry: disks)
	{
		if(entry.second.owner == owner)
			diskClearOwner(entry.second);
	}
}
