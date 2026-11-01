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

#include <cstdio>
#include <cstring>
#include <ghost/filesystem.h>
#include <ghost/messages.h>
#include <ghost/system.h>
#include <ghost/tasks.h>
#include <libblockdevice/driver.hpp>
#include <libstorage/volume.hpp>
#include <set>
#include "administration.hpp"
#include "registry.hpp"

struct g_volume_pending
{
	g_tid client;
	g_message_transaction transaction;
	g_volume_id volume;
	g_tid handler;
	g_volume_command command;
	char name[32]{};
	uint64_t completed = 0;
	uint64_t total = 0;
};

struct g_volume_reader
{
	g_device_id device;
	g_block_buffer buffer;
};

static g_volume_registry storageManagerRegistry;
static std::map<g_message_transaction, g_volume_pending> volumePending;

static void storageManagerReply(g_tid client, g_message_transaction tx, g_volume_response response)
{
	g_send_message_tm(client, &response, sizeof(response), tx, G_MESSAGE_SEND_MODE_NON_BLOCKING);
}

static bool diskReadBlocks(void* context, uint64_t lba, uint8_t* bytes, uint32_t count)
{
	auto reader = static_cast<g_volume_reader*>(context);
	if(blockDeviceReadBuffer(&reader->buffer, reader->device, lba, count) != G_BLOCK_SUCCESS)
		return false;
	memcpy(bytes, reader->buffer.data, count * G_VOLUME_SECTOR_SIZE);
	return true;
}

static g_volume_status diskScan(g_volume_info disk, g_tid requester, g_volume_job_id job = 0, bool manual = false)
{
	if(storageManagerRegistry.disks.count(disk.device) && storageManagerRegistry.diskBusy(disk.device, requester, job))
		return G_VOLUME_BUSY;

	g_block_device_info info{};
	if(blockDeviceGetInfo(disk.driver, disk.device, &info) != G_BLOCK_SUCCESS)
		return G_VOLUME_IO_ERROR;

	disk.sectorCount = info.sectorCount;
	disk.sectorSize = info.sectorSize;

	g_volume_reader reader;
	reader.device = disk.device;
	if(blockDeviceOpenBuffer(disk.driver, &reader.buffer) != G_BLOCK_SUCCESS)
		return G_VOLUME_IO_ERROR;

	g_disk_info details;
	details.device = disk.device;
	details.driver = disk.driver;
	details.sectorSize = info.sectorSize;
	details.sectorCount = info.sectorCount;
	details.flags = info.flags;
	memcpy(details.model, info.model, sizeof(details.model));
	memcpy(details.serial, info.serial, sizeof(details.serial));
	details.generation = storageManagerRegistry.disks[disk.device].snapshot.disk.generation + 1;

	g_volume_storage_io io{&reader, diskReadBlocks, nullptr, nullptr, nullptr};
	g_volume_disk_snapshot snapshot;
	std::vector<g_volume_info> volumes;
	auto status = diskInspect(details, io, &snapshot, &volumes);

	auto closed = blockDeviceCloseBuffer(&reader.buffer);
	if(closed != G_BLOCK_SUCCESS)
		closed = blockDeviceCloseBuffer(&reader.buffer);
	if(closed != G_BLOCK_SUCCESS)
		return G_VOLUME_IO_ERROR;

	storageManagerRegistry.disks[disk.device].snapshot = snapshot;
	if(status != G_VOLUME_OK)
		volumes.clear();
	if(status == G_VOLUME_IO_ERROR && !manual && storageManagerRegistry.disks[disk.device].info.sectorCount)
		return status;

	auto events = storageManagerRegistry.diskReplace(disk, volumes);
	for(auto& event: events)
	{
		if(event.type == G_VOLUME_DISCOVERED && manual)
		{
			auto& record = storageManagerRegistry.volumes[event.info.id];
			record.manual = true;
			record.info.handler = G_TID_NONE;
			record.info.filesystem[0] = 0;
			event.info = record.info;
			event.type = G_VOLUME_PROBE;
		}
		if(g_send_topic_message(G_VOLUME_EVENT_TOPIC, &event, sizeof(event)) != G_MESSAGE_SEND_STATUS_SUCCESSFUL)
		{
			return G_VOLUME_UNAVAILABLE;
		}
		if(event.type == G_VOLUME_DISCOVERED || event.type == G_VOLUME_PROBE)
		{
			klog("volume: volume %lu disk %u partition %u sectors %lu+%lu", event.info.id, unsigned(disk.device),
				 event.info.partition, event.info.firstSector, event.info.sectorCount);
		}
	}
	return status;
}

static g_volume_status volumeValidateControl(g_tid sender, const g_volume_request& request, const g_volume_info& info)
{
	if(storageManagerAdminQuiescing())
		return G_VOLUME_BUSY;

	const auto& disk = storageManagerRegistry.disks[info.device];
	if(disk.job || (disk.owner != G_TID_NONE && (disk.owner != sender || disk.allowedVolume != request.volume)))
		return G_VOLUME_BUSY;

	bool namedMount = request.command == G_VOLUME_MOUNT && request.name[0];
	if(namedMount && !volumeMountNameValid(request.name))
		return G_VOLUME_INVALID;

	if(request.command == G_VOLUME_UNMOUNT && request.claim && request.claim != info.claim)
		return G_VOLUME_CONFLICT;

	if(namedMount)
	{
		char path[64];
		snprintf(path, sizeof(path), "/mount/%s", request.name);

		if(info.state != G_VOLUME_AVAILABLE && strcmp(info.path, path))
			return G_VOLUME_CONFLICT;

		if(storageManagerRegistry.volumePathClaimed(path, request.volume))
			return G_VOLUME_CONFLICT;
	}
	return G_VOLUME_OK;
}

static void volumeHandleControl(g_tid sender, g_message_transaction tx, const g_volume_request& request)
{
	g_volume_request get;
	get.volume = request.volume;
	auto response = storageManagerRegistry.storageManagerHandleRequest(sender, get);
	response.command = request.command;

	if(response.status == G_VOLUME_OK)
		response.status = volumeValidateControl(sender, request, response.info);

	if(response.status != G_VOLUME_OK)
	{
		storageManagerReply(sender, tx, response);
		return;
	}

	if((request.command == G_VOLUME_MOUNT && response.info.state == G_VOLUME_MOUNTED) ||
	   (request.command == G_VOLUME_UNMOUNT && response.info.state == G_VOLUME_AVAILABLE))
	{
		storageManagerReply(sender, tx, response);
		return;
	}

	for(const auto& entry: volumePending)
	{
		if(entry.second.volume == request.volume)
		{
			response.status = G_VOLUME_BUSY;
			storageManagerReply(sender, tx, response);
			return;
		}
	}

	if(response.info.handler == G_TID_NONE)
	{
		response.status = G_VOLUME_NOT_FOUND;
		storageManagerReply(sender, tx, response);
		return;
	}

	auto forwarded = request;
	auto driverTx = g_get_message_tx_id();
	if(g_send_message_tm(response.info.handler, &forwarded, sizeof(forwarded), driverTx,
						 G_MESSAGE_SEND_MODE_NON_BLOCKING) != G_MESSAGE_SEND_STATUS_SUCCESSFUL)
	{
		response.status = G_VOLUME_UNAVAILABLE;
		storageManagerReply(sender, tx, response);
		return;
	}

	g_volume_pending pending{sender, tx, request.volume, response.info.handler, request.command};
	if(request.command == G_VOLUME_MOUNT && request.name[0])
		strcpy(pending.name, request.name);
	volumePending[driverTx] = pending;
	storageManagerRegistry.volumes[request.volume].controlPending = true;
}

static void volumeReceiveControlResponse(const g_message_header* message, const void* content)
{
	auto pending = volumePending.find(message->transaction);
	if(pending == volumePending.end() || message->sender != pending->second.handler)
		return;

	auto& control = pending->second;
	g_volume_response response;
	memcpy(&response, content, sizeof(response));

	if(response.magic != G_VOLUME_MAGIC || response.version != G_VOLUME_VERSION ||
	   response.command != control.command || response.info.id != control.volume ||
	   response.status > G_VOLUME_PROTOCOL_ERROR)
		return;

	g_volume_request get;
	get.volume = control.volume;
	response.info = storageManagerRegistry.storageManagerHandleRequest(message->sender, get).info;
	if(response.total || response.completed)
	{
		if(response.command != G_VOLUME_STAT || response.status != G_VOLUME_OK || !response.total ||
		   response.completed > response.total ||
		   (control.total && (response.total != control.total || response.completed <= control.completed)))
			return;
		control.completed = response.completed;
		control.total = response.total;
		storageManagerReply(control.client, control.transaction, response);
		return;
	}

	if(response.status == G_VOLUME_OK &&
	   ((response.command == G_VOLUME_MOUNT && response.info.state != G_VOLUME_MOUNTED) ||
		(response.command == G_VOLUME_UNMOUNT && response.info.state != G_VOLUME_AVAILABLE)))
		response.status = G_VOLUME_PROTOCOL_ERROR;

	if(response.status == G_VOLUME_OK && control.name[0])
	{
		char path[64];
		snprintf(path, sizeof(path), "/mount/%s", control.name);
		if(strcmp(response.info.path, path))
			response.status = G_VOLUME_PROTOCOL_ERROR;
	}

	storageManagerReply(control.client, control.transaction, response);
	storageManagerRegistry.volumes[control.volume].controlPending = false;
	volumePending.erase(pending);
}

static g_volume_response volumeHandleClaim(g_tid sender, const g_volume_request& request)
{
	g_volume_response response;
	response.command = request.command;
	auto record = storageManagerRegistry.volumes.find(request.volume);
	bool commanded = false;

	for(const auto& entry: volumePending)
	{
		const auto& pending = entry.second;
		commanded |= pending.volume == request.volume && pending.handler == sender && pending.command == G_VOLUME_MOUNT;
	}

	if(!commanded &&
	   (storageManagerAdminQuiescing() || (record != storageManagerRegistry.volumes.end() && record->second.manual)))
	{
		response.status = G_VOLUME_BUSY;
		if(record != storageManagerRegistry.volumes.end())
			response.info = record->second.info;
		return response;
	}

	for(const auto& entry: volumePending)
	{
		const auto& pending = entry.second;
		if(pending.volume != request.volume || pending.handler != sender || !pending.name[0])
			continue;
		if(!memchr(request.name, 0, sizeof(request.name)) || strcmp(pending.name, request.name))
		{
			response.status = G_VOLUME_CONFLICT;
			return response;
		}
	}

	return storageManagerRegistry.storageManagerHandleRequest(sender, request);
}

static void storageManagerReceive(const g_message_header* message, const void* content)
{
	if(storageManagerAdminReceive(&storageManagerRegistry, message, content))
		return;

	if(message->length == sizeof(g_volume_response))
	{
		volumeReceiveControlResponse(message, content);
		return;
	}

	if(message->length != sizeof(g_volume_request))
		return;

	g_volume_request request;
	memcpy(&request, content, sizeof(request));
	if(request.magic != G_VOLUME_MAGIC || request.version != G_VOLUME_VERSION || request.reserved)
		return;

	switch(request.command)
	{
		case G_VOLUME_CLAIM:
			storageManagerReply(message->sender, message->transaction, volumeHandleClaim(message->sender, request));
			break;
		case G_VOLUME_MOUNT:
		case G_VOLUME_UNMOUNT:
		case G_VOLUME_FLUSH:
		case G_VOLUME_STAT:
			volumeHandleControl(message->sender, message->transaction, request);
			break;
		case G_VOLUME_RESCAN:
		{
			g_volume_response response;
			response.command = request.command;
			response.info.device = request.device;
			auto disk = storageManagerRegistry.disks.find(request.device);
			if(storageManagerAdminQuiescing())
				response.status = G_VOLUME_BUSY;
			else if(disk == storageManagerRegistry.disks.end())
				response.status = G_VOLUME_NOT_FOUND;
			else
				response.status = diskScan(disk->second.info, message->sender);
			storageManagerReply(message->sender, message->transaction, response);
			break;
		}
		default:
			storageManagerReply(message->sender, message->transaction,
								storageManagerRegistry.storageManagerHandleRequest(message->sender, request));
			break;
	}
}

static void diskReceiveDevice(g_tid manager, const g_message_header* message, const void* content)
{
	if(message->sender != manager || message->length != sizeof(g_device_event_device_registered))
		return;

	g_device_event_device_registered event;
	memcpy(&event, content, sizeof(event));
	if(event.header.event != G_DEVICE_EVENT_DEVICE_REGISTERED || event.type != G_DEVICE_TYPE_BLOCK || !event.id ||
	   event.driver == G_TID_NONE || storageManagerRegistry.disks.count(event.id))
		return;

	g_volume_info disk;
	disk.device = event.id;
	disk.driver = event.driver;
	diskScan(disk, G_TID_NONE);
}

static bool volumeMountDetached(const g_volume_info& info)
{
	g_fs_stat_data stat{};
	auto status = g_fs_stat(info.path, &stat);
	if(status == G_FS_STAT_NOT_FOUND)
		return true;
	if(status == G_FS_STAT_SUCCESS)
		return info.mount >= 0 && stat.virtual_id != info.mount;

	if(strncmp(info.path, "/mount/", 7) || !info.path[7])
		return false;

	auto directory = g_open_directory_s("/mount", nullptr);
	if(!directory)
		return false;

	g_fs_read_directory_status readStatus;
	while(auto entry = g_read_directory_s(directory, &readStatus))
	{
		if(strcmp(entry->name, info.path + 7))
			continue;
		bool detached = info.mount >= 0 && entry->node_id != info.mount;
		g_close_directory(directory);
		return detached;
	}

	bool detached = readStatus == G_FS_READ_DIRECTORY_EOD;
	g_close_directory(directory);
	return detached;
}

/**
 * Kernel cleanup detaches a dead serving thread's VFS mount. Reclaim only after
 * the namespace confirms its absence; directory enumeration also handles stat
 * reporting a generic lookup error for a removed mount.
 */
static void storageManagerReapOwners()
{
	std::set<g_tid> dead;
	std::set<g_tid> attached;
	for(const auto& entry: storageManagerRegistry.volumes)
	{
		const auto& info = entry.second.info;
		if(info.owner == G_TID_NONE)
		{
			if(info.handler != G_TID_NONE && g_get_pid_for_tid(info.handler) == G_PID_NONE)
				dead.insert(info.handler);
			continue;
		}
		if(g_get_pid_for_tid(info.owner) != G_PID_NONE)
			continue;
		if(volumeMountDetached(info))
			dead.insert(info.owner);
		else
			attached.insert(info.owner);
	}

	for(const auto& entry: storageManagerRegistry.disks)
	{
		if(entry.second.owner != G_TID_NONE && g_get_pid_for_tid(entry.second.owner) == G_PID_NONE)
			dead.insert(entry.second.owner);
	}

	for(auto owner: dead)
	{
		if(!attached.count(owner))
			storageManagerRegistry.storageManagerForgetOwner(owner);
	}

	for(auto it = volumePending.begin(); it != volumePending.end();)
	{
		if(g_get_pid_for_tid(it->second.handler) != G_PID_NONE)
		{
			++it;
			continue;
		}
		g_volume_response response;
		response.command = it->second.command;
		response.status = G_VOLUME_UNAVAILABLE;
		response.info.id = it->second.volume;
		storageManagerRegistry.volumes[it->second.volume].controlPending = false;
		storageManagerReply(it->second.client, it->second.transaction, response);
		it = volumePending.erase(it);
	}
}

static bool storageManagerRegisterPower()
{
	for(;;)
	{
		auto status = g_power_register(G_POWER_STORAGE_PRODUCER);
		if(status != G_POWER_BUSY)
			return status == G_POWER_SUCCESSFUL;
		g_sleep(1);
	}
}

int main()
{
	if(!g_task_register_name(G_STORAGE_MANAGER_NAME))
		return 1;

	storageManagerAdminInitialize(diskScan);
	if(!storageManagerRegisterPower())
		return 1;

	auto manager = g_task_await_by_name(G_DEVICE_MANAGER_NAME);
	auto cursor = G_MESSAGE_TOPIC_TRANSACTION_START;
	uint64_t reaped = 0;
	uint8_t incoming[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];

	klog("manager ready");
	for(;;)
	{
		bool worked = false;
		if(g_receive_message_m(incoming, sizeof(incoming), G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) ==
		   G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			storageManagerReceive(reinterpret_cast<g_message_header*>(incoming), G_MESSAGE_CONTENT(incoming));
			worked = true;
		}

		if(!storageManagerAdminQuiescing() &&
		   g_receive_topic_message_m(G_DEVICE_EVENT_TOPIC, incoming, sizeof(incoming), cursor,
									 G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			auto message = reinterpret_cast<g_message_header*>(incoming);
			cursor = message->transaction;
			diskReceiveDevice(manager, message, G_MESSAGE_CONTENT(incoming));
			worked = true;
		}

		if(g_millis() - reaped >= 100)
		{
			storageManagerReapOwners();
			reaped = g_millis();
		}

		storageManagerAdminPoll(&storageManagerRegistry);
		storageManagerAdminPollPower(!volumePending.empty());

		if(!worked)
			g_sleep(1);
	}
}
