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

#include "mount.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ghost/tasks.h>

static uint32_t fatRootDisk;
static uint32_t fatRootSector;
static bool fatRootSectorSpecified;

bool fatMountFlush(g_fat_mount* mount)
{
	return !mount->failed && (!(mount->device.info.flags & G_BLOCK_WRITABLE) || fatVolumeFlush(mount));
}

bool fatMountConfigureRoot(const char* specification)
{
	auto length = strlen(specification);
	if((length != 8 && length != 17) || strspn(specification, "0123456789abcdefABCDEF:") != length ||
	   (length == 17 && specification[8] != ':'))
		return false;
	char disk[9];
	memcpy(disk, specification, 8);
	disk[8] = 0;
	if(strspn(disk, "0123456789abcdefABCDEF") != 8)
		return false;
	fatRootDisk = strtoul(disk, nullptr, 16);
	if(length == 17)
	{
		if(strspn(specification + 9, "0123456789abcdefABCDEF") != 8)
			return false;
		fatRootSector = strtoul(specification + 9, nullptr, 16);
		fatRootSectorSpecified = true;
	}
	return true;
}

/**
 * The boot target is identified by its MBR disk signature and optional partition
 * LBA, so another FAT volume on that disk cannot claim the root mount.
 */
static bool fatMountIsBootRoot(g_fat_mount* mount)
{
	if(!fatRootDisk || (fatRootSectorSpecified && mount->source.firstSector != fatRootSector))
		return false;
	return blockDeviceReadBuffer(&mount->device.buffer, mount->device.id, 0, 1) == G_BLOCK_SUCCESS &&
		   fatRead32(mount->device.buffer.data + 440) == fatRootDisk;
}

/**
 * A detached mount retains its claim until its DMA window is closed and the
 * manager acknowledges release. Repeated calls resume incomplete teardown.
 */
g_volume_status fatMountFinishUnmount(g_fat_mount* mount)
{
	if(mount->storage)
		return G_VOLUME_BUSY;
	if(!fatDeviceClose(&mount->device))
		return G_VOLUME_IO_ERROR;
	if(mount->claim && volumeRelease(mount->source.id, mount->claim) != G_VOLUME_OK)
		return G_VOLUME_IO_ERROR;
	mount->claim = 0;
	mount->failed = false;
	return G_VOLUME_OK;
}

g_volume_status fatMountStart(g_fat_mount* mount, const char* requestedName)
{
	if(!mount->source.id)
		return G_VOLUME_INVALID;
	if(requestedName && !volumeMountNameValid(requestedName))
		return G_VOLUME_INVALID;
	g_volume_info current;
	if(volumeGet(mount->source.id, &current) != G_VOLUME_OK)
		return G_VOLUME_NOT_FOUND;
	if(mount->storage)
	{
		if(requestedName)
		{
			char path[64];
			snprintf(path, sizeof(path), "/mount/%s", requestedName);
			if(strcmp(path, mount->source.path))
				return G_VOLUME_BUSY;
		}
		return volumeCommit(mount->source.id, mount->claim, mount->id) == G_VOLUME_OK ? G_VOLUME_OK : G_VOLUME_IO_ERROR;
	}
	if(mount->claim && fatMountFinishUnmount(mount) != G_VOLUME_OK)
		return G_VOLUME_IO_ERROR;
	mount->dirty = true;
	mount->allocationHintsInvalid = false;
	if(!fatDeviceOpen(&mount->device))
	{
		fatDeviceClose(&mount->device);
		return G_VOLUME_IO_ERROR;
	}
	if(!fatVolumeProbe(&mount->device, &mount->volume))
	{
		fatDeviceClose(&mount->device);
		return G_VOLUME_NOT_FOUND;
	}
	char name[32];
	if(requestedName)
		strcpy(name, requestedName);
	else if(mount->source.partition <= 1)
		snprintf(name, sizeof(name), "fat%u", mount->device.id);
	else
		snprintf(name, sizeof(name), "fat%up%u", mount->device.id, mount->source.partition);
	if(fatMountIsBootRoot(mount))
	{
		if(requestedName)
		{
			fatDeviceClose(&mount->device);
			return G_VOLUME_BUSY;
		}
		strcpy(name, "fat-root");
		mount->bootRoot = true;
	}
	g_volume_info claimed;
	auto claimStatus = volumeClaim(mount->source.id, "fat", name, &claimed);
	if(claimStatus != G_VOLUME_OK)
	{
		fatDeviceClose(&mount->device);
		return claimStatus == G_VOLUME_CONFLICT || claimStatus == G_VOLUME_BUSY ? G_VOLUME_BUSY : G_VOLUME_IO_ERROR;
	}
	mount->claim = claimed.claim;
	mount->source = claimed;
	auto status = g_fs_delegate_register(name, G_FAT_ROOT, &mount->id, &mount->storage);
	if(status != G_FS_DELEGATE_SUCCESSFUL)
	{
		mount->storage = nullptr;
		mount->id = G_FS_VIRT_ID_NONE;
		fatMountFinishUnmount(mount);
		return status == G_FS_DELEGATE_BUSY ? G_VOLUME_BUSY : G_VOLUME_IO_ERROR;
	}
	if(volumeCommit(mount->source.id, mount->claim, mount->id) != G_VOLUME_OK)
		return G_VOLUME_IO_ERROR;
	klog("fat: mounted /mount/%s FAT%u at sector %lu (FAT filesystem)", name, mount->volume.fatBits,
		 mount->volume.firstSector);
	return G_VOLUME_OK;
}

static g_volume_status fatMountUnmount(g_fat_mount* mount)
{
	if(!mount->storage)
		return mount->claim ? fatMountFinishUnmount(mount) : G_VOLUME_NOT_FOUND;
	if(mount->bootRoot)
		return G_VOLUME_BUSY;
	for(auto inode = mount->inodes; inode; inode = inode->next)
		if(inode->opens)
			return G_VOLUME_BUSY;
	bool flushed = fatMountFlush(mount);
	auto result = g_fs_delegate_unregister(mount->id);
	if(result != G_FS_DELEGATE_SUCCESSFUL)
		return result == G_FS_DELEGATE_BUSY ? G_VOLUME_BUSY : G_VOLUME_IO_ERROR;
	mount->storage = nullptr;
	mount->id = G_FS_VIRT_ID_NONE;
	fatMountRelease(mount);
	mount->nextInode = 1;
	mount->nextFreeCluster = 2;
	fatCacheInvalidate(mount);
	auto finished = fatMountFinishUnmount(mount);
	return finished == G_VOLUME_OK && flushed ? G_VOLUME_OK : G_VOLUME_IO_ERROR;
}

struct g_fat_stat_request
{
	g_tid client;
	g_message_transaction transaction;
	g_volume_response response;
};

static void fatStatProgress(void* context, uint64_t done, uint64_t total)
{
	auto query = static_cast<g_fat_stat_request*>(context);
	query->response.completed = done;
	query->response.total = total;
	g_send_message_tm(query->client, &query->response, sizeof(query->response), query->transaction,
					  G_MESSAGE_SEND_MODE_NON_BLOCKING);
}

static void fatVolumeControl(g_fat_mount* mount, const g_volume_request& request, g_volume_response* response,
							 g_volume_progress_callback progress = nullptr, void* context = nullptr)
{
	response->command = request.command;
	response->info = mount->source;
	if(request.command == G_VOLUME_STAT)
	{
		if(!mount->storage)
			response->status = G_VOLUME_NOT_FOUND;
		else
			response->status =
					fatVolumeStat(mount, &response->filesystem, progress, context) ? G_VOLUME_OK : G_VOLUME_IO_ERROR;
		return;
	}
	if(request.command == G_VOLUME_UNMOUNT && request.claim && request.claim != mount->claim)
	{
		response->status = G_VOLUME_CONFLICT;
		return;
	}
	switch(request.command)
	{
		case G_VOLUME_MOUNT:
			response->status = fatMountStart(mount, request.name[0] ? request.name : nullptr);
			break;
		case G_VOLUME_FLUSH:
			if(!mount->storage)
				response->status = G_VOLUME_NOT_FOUND;
			else
				response->status = fatMountFlush(mount) ? G_VOLUME_OK : G_VOLUME_IO_ERROR;
			break;
		case G_VOLUME_UNMOUNT:
			response->status = fatMountUnmount(mount);
			break;
		default:
			response->status = G_VOLUME_INVALID;
			break;
	}
}

bool fatMountReceiveControl(g_fat_mount* mount, const g_message_header* message, const void* content)
{
	if(message->sender != g_task_get_by_name(G_STORAGE_MANAGER_NAME) || message->length != sizeof(g_volume_request))
		return false;
	g_volume_request request;
	memcpy(&request, content, sizeof(request));
	if(request.magic != G_VOLUME_MAGIC || request.version != G_VOLUME_VERSION || request.volume != mount->source.id)
		return true;
	switch(request.command)
	{
		case G_VOLUME_MOUNT:
		case G_VOLUME_UNMOUNT:
		case G_VOLUME_FLUSH:
		case G_VOLUME_STAT:
			break;
		default:
			return true;
	}
	g_volume_response response;
	g_fat_stat_request query{message->sender, message->transaction, {}};
	query.response.command = request.command;
	query.response.info = mount->source;
	query.response.status = G_VOLUME_OK;
	fatVolumeControl(mount, request, &response, fatStatProgress, &query);
	while(g_send_message_tm(message->sender, &response, sizeof(response), message->transaction,
							G_MESSAGE_SEND_MODE_NON_BLOCKING) == G_MESSAGE_SEND_STATUS_FULL &&
		  g_get_pid_for_tid(message->sender) != G_PID_NONE)
		g_sleep(1);
	return true;
}
