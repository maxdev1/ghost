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

#include <cstring>
#include <ghost/tasks.h>
#include <map>
#include <new>
#include "mount.hpp"

static g_tid fatDiscoveryTask = G_TID_NONE;
static std::map<g_volume_id, g_tid> fatVolumeWorkers;

/**
 * Advertises a recognized filesystem without claiming a mount. Rescans after
 * administration discard cached metadata before probing the updated volume.
 */
static void fatProbeVolume(g_fat_mount* mount)
{
	if(mount->storage || mount->claim)
		return;

	fatMountRelease(mount);
	fatCacheInvalidate(mount);
	mount->failed = false;

	if(fatDeviceOpen(&mount->device) && fatVolumeProbe(&mount->device, &mount->volume))
		volumeOffer(mount->source.id, "fat", nullptr);

	fatDeviceClose(&mount->device);
}

/**
 * Keep the serving task and its claim alive through uncertain buffer cleanup.
 * Owner reaping can safely reclaim a terminated worker after I/O is quiescent.
 */
static void fatServeVolume(void* argument)
{
	auto mount = static_cast<g_fat_mount*>(argument);
	if(mount->probeOnly)
		fatProbeVolume(mount);
	else
		fatMountStart(mount);

	uint8_t incoming[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	while(g_receive_message(incoming, sizeof(incoming)) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto message = reinterpret_cast<g_message_header*>(incoming);
		if(message->sender == fatDiscoveryTask && message->length == sizeof(g_volume_event))
		{
			g_volume_event event;
			memcpy(&event, G_MESSAGE_CONTENT(incoming), sizeof(event));
			if(event.type == G_VOLUME_REMOVED && event.info.id == mount->source.id && !mount->storage)
				break;
			if(event.type == G_VOLUME_PROBE && event.info.id == mount->source.id)
				fatProbeVolume(mount);
		}
		else if(!fatMountReceiveControl(mount, message, G_MESSAGE_CONTENT(incoming)) && mount->storage &&
				!fatMountHandleRequest(mount, message, G_MESSAGE_CONTENT(incoming)))
			break;
	}

	bool detached = !mount->storage || g_fs_delegate_unregister(mount->id) == G_FS_DELEGATE_SUCCESSFUL;
	if(detached)
		mount->storage = nullptr;

	while(mount->claim)
	{
		if(detached ? fatMountFinishUnmount(mount) == G_VOLUME_OK : fatDeviceClose(&mount->device))
			break;
		g_sleep(100);
	}

	fatMountRelease(mount);
	if(mount->device.buffer.handle)
		fatDeviceClose(&mount->device);

	delete mount;
}

static bool fatDiscoverVolume(g_tid manager, const g_message_header* message, const void* content)
{
	if(message->sender != manager || message->length != sizeof(g_volume_event))
		return false;

	g_volume_event event;
	memcpy(&event, content, sizeof(event));
	if(event.magic != G_VOLUME_MAGIC || event.version != G_VOLUME_VERSION ||
	   (event.type != G_VOLUME_DISCOVERED && event.type != G_VOLUME_PROBE && event.type != G_VOLUME_REMOVED) ||
	   !event.info.device || event.info.driver == G_TID_NONE || !event.info.sectorCount ||
	   event.info.sectorSize != G_BLOCK_SECTOR_SIZE || !event.info.id)
		return false;

	if(event.type == G_VOLUME_REMOVED)
	{
		auto worker = fatVolumeWorkers.find(event.info.id);
		if(worker != fatVolumeWorkers.end() &&
		   g_send_message_tm(worker->second, &event, sizeof(event), G_MESSAGE_TRANSACTION_NONE,
							 G_MESSAGE_SEND_MODE_NON_BLOCKING) == G_MESSAGE_SEND_STATUS_SUCCESSFUL)
			fatVolumeWorkers.erase(worker);
		return false;
	}

	auto previous = fatVolumeWorkers.find(event.info.id);
	if(previous != fatVolumeWorkers.end() && g_get_pid_for_tid(previous->second) == G_PID_NONE)
		fatVolumeWorkers.erase(previous);

	bool created = false;
	if(!fatVolumeWorkers.count(event.info.id))
	{
		auto mount = new(std::nothrow) g_fat_mount;
		if(mount)
		{
			mount->device.id = event.info.device;
			mount->device.handler = event.info.driver;
			mount->source = event.info;
			mount->probeOnly = event.type == G_VOLUME_PROBE;
			mount->device.firstSector = event.info.firstSector;
			mount->device.sectorCount = event.info.sectorCount;

			auto worker = g_create_task_d(reinterpret_cast<void*>(fatServeVolume), mount);
			if(worker == G_TID_NONE)
				delete mount;
			else
			{
				fatVolumeWorkers[event.info.id] = worker;
				created = true;
			}
		}
	}

	if(event.type == G_VOLUME_PROBE && !created && fatVolumeWorkers.count(event.info.id))
		g_send_message_tm(fatVolumeWorkers[event.info.id], &event, sizeof(event), G_MESSAGE_TRANSACTION_NONE,
						  G_MESSAGE_SEND_MODE_NON_BLOCKING);

	return created;
}

void fatDiscoverVolumes()
{
	fatDiscoveryTask = g_get_tid();

	auto manager = g_task_await_by_name(G_STORAGE_MANAGER_NAME);
	auto cursor = G_MESSAGE_TOPIC_TRANSACTION_START;
	uint8_t incoming[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	while(g_receive_topic_message(G_VOLUME_EVENT_TOPIC, incoming, sizeof(incoming), cursor) ==
		  G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto message = reinterpret_cast<g_message_header*>(incoming);
		cursor = message->transaction;
		fatDiscoverVolume(manager, message, G_MESSAGE_CONTENT(incoming));
	}
}
