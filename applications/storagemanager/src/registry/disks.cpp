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

g_volume_disk* registryDiskFind(g_volume_registry* registry, g_device_id device)
{
	auto entry = registry->disks.find(device);
	return entry == registry->disks.end() ? NULL : &entry->second;
}

g_volume_disk* registryDiskRegister(g_volume_registry* registry, g_device_id device)
{
	g_volume_disk* disk = registryDiskFind(registry, device);
	if(disk)
		return disk;

	disk = &registry->disks[device];
	disk->info.device = device;
	disk->owner = G_TID_NONE;
	disk->releasedOwner = G_TID_NONE;
	return disk;
}

bool registryDiskIsBusy(g_volume_registry* registry, g_device_id device, g_tid requester, g_volume_job_id job)
{
	g_volume_disk* disk = registryDiskFind(registry, device);
	if(!disk || (disk->job && disk->job != job))
		return true;
	if(disk->owner != G_TID_NONE && disk->owner != requester)
		return true;

	for(auto& entry : registry->volumes)
	{
		g_volume_record* volume = &entry.second;
		if(volume->active && volume->info.device == device &&
		   (volume->info.owner != G_TID_NONE || volume->controlPending))
			return true;
	}
	return false;
}

void registryDiskClearOwner(g_volume_disk* disk)
{
	disk->owner = G_TID_NONE;
	disk->claim = 0;
	disk->allowedVolume = 0;
}

static g_volume_disk* registryDiskPrepareResponse(g_volume_registry* registry, g_device_id device,
										g_volume_command command, g_volume_response* response)
{
	response->command = command;
	g_volume_disk* disk = registryDiskFind(registry, device);
	if(!disk)
		response->status = G_VOLUME_NOT_FOUND;
	else
		response->info = disk->info;
	return disk;
}

g_volume_response registryDiskHandleClaim(g_volume_registry* registry, g_tid sender, const g_volume_claim_disk_request* request)
{
	g_volume_response response = {};
	g_volume_disk* disk = registryDiskPrepareResponse(registry, request->device, G_VOLUME_CLAIM_DISK, &response);
	if(!disk)
		return response;

	g_volume_record* volume = registryVolumeFind(registry, request->volume);
	if(request->volume && (!volume || !volume->active || volume->info.device != request->device))
		response.status = G_VOLUME_INVALID;
	else if(registryDiskIsBusy(registry, request->device, sender, 0))
		response.status = G_VOLUME_CONFLICT;
	else
	{
		if(disk->owner == G_TID_NONE)
		{
			disk->owner = sender;
			disk->claim = ++registry->nextClaim;
		}
		disk->allowedVolume = request->volume;
		response.status = G_VOLUME_OK;
	}
	response.info.owner = disk->owner;
	response.info.claim = disk->claim;
	return response;
}

g_volume_response registryDiskHandleRelease(g_volume_registry* registry, g_tid sender, const g_volume_release_disk_request* request)
{
	g_volume_response response = {};
	g_volume_disk* disk = registryDiskPrepareResponse(registry, request->device, G_VOLUME_RELEASE_DISK, &response);
	if(!disk)
		return response;

	if(disk->owner == sender && disk->claim == request->claim && request->claim)
	{
		disk->releasedClaim = disk->claim;
		disk->releasedOwner = sender;
		registryDiskClearOwner(disk);
		response.status = G_VOLUME_OK;
	}
	else if(disk->releasedOwner == sender && disk->releasedClaim == request->claim && request->claim)
		response.status = G_VOLUME_OK;
	else
		response.status = G_VOLUME_NOT_OWNER;
	response.info.owner = disk->owner;
	response.info.claim = disk->claim;
	return response;
}
