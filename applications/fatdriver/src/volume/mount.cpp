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
	if(mount->failed || mount->device.bufferUncertain)
		return false;
	if(!fatVolumeWritable(mount))
		return true;

	return fatVolumeFlush(mount);
}

bool fatMountConfigureRoot(const char* specification)
{
	if(!specification)
		return false;

	auto length = strlen(specification);
	if((length != G_FAT_BOOT_DISK_HEX_DIGITS && length != G_FAT_BOOT_ROOT_SPEC_SIZE) ||
	   strspn(specification, "0123456789abcdefABCDEF:") != length ||
	   (length == G_FAT_BOOT_ROOT_SPEC_SIZE && specification[G_FAT_BOOT_DISK_HEX_DIGITS] != ':'))
		return false;

	char disk[G_FAT_BOOT_DISK_HEX_DIGITS + 1];
	memcpy(disk, specification, G_FAT_BOOT_DISK_HEX_DIGITS);
	disk[G_FAT_BOOT_DISK_HEX_DIGITS] = 0;
	if(strspn(disk, "0123456789abcdefABCDEF") != G_FAT_BOOT_DISK_HEX_DIGITS)
		return false;

	uint32_t sector = 0;
	bool sectorSpecified = length == G_FAT_BOOT_ROOT_SPEC_SIZE;
	if(sectorSpecified)
	{
		if(strspn(specification + G_FAT_BOOT_DISK_HEX_DIGITS + 1, "0123456789abcdefABCDEF") != G_FAT_BOOT_DISK_HEX_DIGITS)
			return false;

		sector = strtoul(specification + G_FAT_BOOT_DISK_HEX_DIGITS + 1, nullptr, 16);
	}

	fatRootDisk = strtoul(disk, nullptr, 16);
	fatRootSector = sector;
	fatRootSectorSpecified = sectorSpecified;
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

	if(!fatDeviceReadBuffer(&mount->device, 0, 1))
		return false;

	return fatRead32(mount->device.buffer.data + G_FAT_MBR_DISK_SIGNATURE_OFFSET) == fatRootDisk;
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

	if(mount->claim)
	{
		auto status = volumeRelease(mount->source.id, mount->claim);
		if(status != G_VOLUME_OK)
			return G_VOLUME_IO_ERROR;
	}

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
			char path[sizeof(mount->source.path)];
			snprintf(path, sizeof(path), "/mount/%s", requestedName);
			if(strcmp(path, mount->source.path))
				return G_VOLUME_BUSY;
		}

		auto status = volumeCommit(mount->source.id, mount->claim, mount->id);
		if(status != G_VOLUME_OK)
			return G_VOLUME_IO_ERROR;

		return G_VOLUME_OK;
	}

	if(mount->claim)
	{
		if(fatMountFinishUnmount(mount) != G_VOLUME_OK)
			return G_VOLUME_IO_ERROR;
	}

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

	char name[sizeof(g_volume_mount_request{}.name)];
	if(requestedName)
		strcpy(name, requestedName);
	else if(mount->source.partition <= 1)
		snprintf(name, sizeof(name), "fat%u", mount->device.id);
	else
		snprintf(name, sizeof(name), "fat%up%u", mount->device.id, mount->source.partition);

	bool bootRoot = fatMountIsBootRoot(mount);
	if(mount->device.status != G_BLOCK_SUCCESS)
	{
		fatDeviceClose(&mount->device);
		return G_VOLUME_IO_ERROR;
	}

	if(bootRoot)
	{
		if(requestedName)
		{
			fatDeviceClose(&mount->device);
			return G_VOLUME_BUSY;
		}

		strcpy(name, "fat-root");
	}
	mount->bootRoot = bootRoot;

	g_volume_info claimed;
	auto claimStatus = volumeClaim(mount->source.id, "fat", name, &claimed);
	if(claimStatus != G_VOLUME_OK)
	{
		fatDeviceClose(&mount->device);
		if(claimStatus == G_VOLUME_CONFLICT || claimStatus == G_VOLUME_BUSY)
			return G_VOLUME_BUSY;

		return G_VOLUME_IO_ERROR;
	}

	mount->claim = claimed.claim;
	mount->source = claimed;

	auto status = g_fs_delegate_register(name, G_FAT_ROOT, &mount->id, &mount->storage);
	if(status != G_FS_DELEGATE_SUCCESSFUL)
	{
		mount->storage = nullptr;
		mount->id = G_FS_VIRT_ID_NONE;
		fatMountFinishUnmount(mount);
		if(status == G_FS_DELEGATE_BUSY)
			return G_VOLUME_BUSY;

		return G_VOLUME_IO_ERROR;
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
	{
		if(mount->claim)
			return fatMountFinishUnmount(mount);

		return G_VOLUME_NOT_FOUND;
	}

	if(mount->bootRoot)
		return G_VOLUME_BUSY;

	for(auto inode = mount->inodes; inode; inode = inode->next)
	{
		if(inode->opens)
			return G_VOLUME_BUSY;
	}

	bool flushed = fatMountFlush(mount);
	auto result = g_fs_delegate_unregister(mount->id);
	if(result != G_FS_DELEGATE_SUCCESSFUL)
	{
		if(result == G_FS_DELEGATE_BUSY)
			return G_VOLUME_BUSY;

		return G_VOLUME_IO_ERROR;
	}

	mount->storage = nullptr;
	mount->id = G_FS_VIRT_ID_NONE;
	fatMountRelease(mount);
	mount->nextInode = 1;
	mount->nextFreeCluster = G_FAT_FIRST_DATA_CLUSTER;
	fatCacheInvalidate(mount);
	auto finished = fatMountFinishUnmount(mount);
	if(finished != G_VOLUME_OK || !flushed)
		return G_VOLUME_IO_ERROR;

	return G_VOLUME_OK;
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

template<typename Request>
static void fatVolumeControl(g_fat_mount* mount, const Request& request, g_volume_response* response,
							 g_volume_progress_callback progress = nullptr, void* context = nullptr)
{
	response->command = request.header.command;
	response->info = mount->source;
	if constexpr(Request::COMMAND == G_VOLUME_MOUNT)
		response->status = fatMountStart(mount, request.name[0] ? request.name : nullptr);
	else if constexpr(Request::COMMAND == G_VOLUME_FLUSH)
	{
		if(!mount->storage)
			response->status = G_VOLUME_NOT_FOUND;
		else
			response->status = fatMountFlush(mount) ? G_VOLUME_OK : G_VOLUME_IO_ERROR;
	}
	else if constexpr(Request::COMMAND == G_VOLUME_UNMOUNT)
	{
		if(request.claim && request.claim != mount->claim)
			response->status = G_VOLUME_CONFLICT;
		else
			response->status = fatMountUnmount(mount);
	}
	else if constexpr(Request::COMMAND == G_VOLUME_STAT)
	{
		if(!mount->storage)
			response->status = G_VOLUME_NOT_FOUND;
		else
			response->status = fatVolumeStat(mount, &response->filesystem, progress, context) ? G_VOLUME_OK : G_VOLUME_IO_ERROR;
	}
}

template<typename Request>
static bool fatMountControlRequest(g_fat_mount* mount, const g_message_header* message, const void* content,
								  g_volume_response* response)
{
	Request request;
	memcpy(&request, content, sizeof(request));
	if(request.volume != mount->source.id)
		return false;

	g_fat_stat_request query{message->sender, message->transaction, {}};
	query.response.command = request.header.command;
	query.response.info = mount->source;
	query.response.status = G_VOLUME_OK;
	fatVolumeControl(mount, request, response, fatStatProgress, &query);
	return true;
}

bool fatMountReceiveControl(g_fat_mount* mount, const g_message_header* message, const void* content)
{
	if(message->sender != g_task_get_by_name(G_STORAGE_MANAGER_NAME) || message->length < sizeof(g_volume_header))
		return false;

	g_volume_header header;
	memcpy(&header, content, sizeof(header));
	if(header.magic != G_VOLUME_MAGIC)
		return false;

	g_volume_response response;
	response.command = header.command;
	response.info = mount->source;
	response.status = G_VOLUME_PROTOCOL_ERROR;
	if(header.version == G_VOLUME_VERSION && storageManagerRequestSize(header.command) == message->length)
	{
		bool addressed = true;
		switch(header.command)
		{
			case G_VOLUME_MOUNT:
				addressed = fatMountControlRequest<g_volume_mount_request>(mount, message, content, &response);
				break;
			case G_VOLUME_UNMOUNT:
				addressed = fatMountControlRequest<g_volume_unmount_request>(mount, message, content, &response);
				break;
			case G_VOLUME_FLUSH:
				addressed = fatMountControlRequest<g_volume_flush_request>(mount, message, content, &response);
				break;
			case G_VOLUME_STAT:
				addressed = fatMountControlRequest<g_volume_stat_request>(mount, message, content, &response);
				break;
			default:
				break;
		}

		if(!addressed)
			return true;
	}

	for(;;)
	{
		auto status = g_send_message_tm(message->sender, &response, sizeof(response), message->transaction,
										G_MESSAGE_SEND_MODE_NON_BLOCKING);
		if(status != G_MESSAGE_SEND_STATUS_FULL)
			break;
		if(g_get_pid_for_tid(message->sender) == G_PID_NONE)
			break;

		g_sleep(G_FAT_IPC_RETRY_MS);
	}

	return true;
}
