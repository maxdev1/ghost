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

#include "controls.hpp"
#include "administration/power.hpp"
#include "registry/discovery.hpp"
#include <ghost/tasks.h>
#include <libstorage/volume.hpp>
#include <stdio.h>
#include <string.h>

bool volumeControlHasPending(const g_volume_controls* controls)
{
	return !controls->pending.empty();
}

static g_volume_pending* volumeControlFindPending(g_volume_controls* controls, g_volume_id volume)
{
	for(auto& entry : controls->pending)
		if(entry.second.volume == volume)
			return &entry.second;
	return NULL;
}

static g_volume_response volumeControlPrepareResponse(const g_volume_controls* controls, g_tid sender,
											 g_volume_id id, g_volume_command command)
{
	g_volume_response response = {};
	response.command = command;
	if(sender == G_TID_NONE)
		return response;

	g_volume_record* volume = registryVolumeFind(controls->registry, id);
	if(!volume || !volume->active)
	{
		response.status = G_VOLUME_NOT_FOUND;
		return response;
	}
	response.info = volume->info;
	g_volume_disk* disk = registryDiskFind(controls->registry, volume->info.device);
	if(adminPowerIsQuiescing() || (disk && (disk->job ||
	   (disk->owner != G_TID_NONE && (disk->owner != sender || disk->allowedVolume != id)))))
		response.status = G_VOLUME_BUSY;
	else if(!disk)
		response.status = G_VOLUME_NOT_FOUND;
	else
		response.status = G_VOLUME_OK;
	return response;
}

static void volumeControlForward(g_volume_controls* controls, g_tid sender, g_message_transaction tx,
								 const void* request, size_t size, g_volume_response* response, const char* name)
{
	if(response->status == G_VOLUME_OK && volumeControlFindPending(controls, response->info.id))
		response->status = G_VOLUME_BUSY;
	if(response->status == G_VOLUME_OK && response->info.handler == G_TID_NONE)
		response->status = G_VOLUME_NOT_FOUND;
	if(response->status != G_VOLUME_OK)
	{
		g_send_message_tm(sender, response, sizeof(*response), tx, G_MESSAGE_SEND_MODE_NON_BLOCKING);
		return;
	}

	g_volume_pending pending = {};
	g_volume_pending* control = &pending;
	control->client = sender;
	control->transaction = tx;
	control->driverTransaction = g_get_message_tx_id();
	control->volume = response->info.id;
	control->handler = response->info.handler;
	control->command = response->command;
	if(name)
		strcpy(control->name, name);

	if(g_send_message_tm(control->handler, (void*) request, size, control->driverTransaction,
						 G_MESSAGE_SEND_MODE_NON_BLOCKING) != G_MESSAGE_SEND_STATUS_SUCCESSFUL)
	{
		response->status = G_VOLUME_UNAVAILABLE;
		g_send_message_tm(sender, response, sizeof(*response), tx, G_MESSAGE_SEND_MODE_NON_BLOCKING);
		return;
	}
	controls->pending[control->driverTransaction] = pending;
	registryVolumeFind(controls->registry, control->volume)->controlPending = true;
}

void volumeControlHandleMount(g_volume_controls* controls, g_tid sender, g_message_transaction tx, const g_volume_mount_request* request)
{
	g_volume_response response = volumeControlPrepareResponse(controls, sender, request->volume, G_VOLUME_MOUNT);
	if(response.status == G_VOLUME_OK && request->name[0])
	{
		if(!volumeMountNameValid(request->name))
			response.status = G_VOLUME_INVALID;
		else
		{
			char path[64];
			snprintf(path, sizeof(path), "/mount/%s", request->name);
			if((response.info.state != G_VOLUME_AVAILABLE && strcmp(response.info.path, path)) ||
			   registryVolumeIsPathClaimed(controls->registry, path, request->volume))
				response.status = G_VOLUME_CONFLICT;
		}
	}
	if(response.status != G_VOLUME_OK || response.info.state == G_VOLUME_MOUNTED)
	{
		g_send_message_tm(sender, &response, sizeof(response), tx, G_MESSAGE_SEND_MODE_NON_BLOCKING);
		return;
	}
	volumeControlForward(controls, sender, tx, request, sizeof(*request), &response, request->name);
}

void volumeControlHandleUnmount(g_volume_controls* controls, g_tid sender, g_message_transaction tx, const g_volume_unmount_request* request)
{
	g_volume_response response = volumeControlPrepareResponse(controls, sender, request->volume, G_VOLUME_UNMOUNT);
	if(response.status == G_VOLUME_OK && request->claim && request->claim != response.info.claim)
		response.status = G_VOLUME_CONFLICT;
	if(response.status != G_VOLUME_OK || response.info.state == G_VOLUME_AVAILABLE)
	{
		g_send_message_tm(sender, &response, sizeof(response), tx, G_MESSAGE_SEND_MODE_NON_BLOCKING);
		return;
	}
	volumeControlForward(controls, sender, tx, request, sizeof(*request), &response, NULL);
}

void volumeControlHandleFlush(g_volume_controls* controls, g_tid sender, g_message_transaction tx, const g_volume_flush_request* request)
{
	g_volume_response response = volumeControlPrepareResponse(controls, sender, request->volume, G_VOLUME_FLUSH);
	volumeControlForward(controls, sender, tx, request, sizeof(*request), &response, NULL);
}

void volumeControlHandleStat(g_volume_controls* controls, g_tid sender, g_message_transaction tx, const g_volume_stat_request* request)
{
	g_volume_response response = volumeControlPrepareResponse(controls, sender, request->volume, G_VOLUME_STAT);
	volumeControlForward(controls, sender, tx, request, sizeof(*request), &response, NULL);
}

void volumeControlHandleResponse(g_volume_controls* controls, const g_message_header* message, const g_volume_response* incoming)
{
	auto entry = controls->pending.find(message->transaction);
	if(entry == controls->pending.end())
		return;
	g_volume_pending* control = &entry->second;
	if(message->sender != control->handler || message->length != sizeof(*incoming))
		return;
	if(incoming->magic != G_VOLUME_RESPONSE_MAGIC || incoming->version != G_VOLUME_VERSION ||
	   incoming->command != control->command || incoming->info.id != control->volume ||
	   incoming->status > G_VOLUME_PROTOCOL_ERROR)
		return;

	g_volume_response response = *incoming;
	g_volume_record* volume = registryVolumeFind(controls->registry, control->volume);
	response.info = {};
	if(volume && volume->active)
		response.info = volume->info;

	if(response.total || response.completed)
	{
		if(response.command != G_VOLUME_STAT || response.status != G_VOLUME_OK || !response.total ||
		   response.completed > response.total ||
		   (control->total && (response.total != control->total || response.completed <= control->completed)))
			return;
		control->completed = response.completed;
		control->total = response.total;
		g_send_message_tm(control->client, &response, sizeof(response), control->transaction,
						  G_MESSAGE_SEND_MODE_NON_BLOCKING);
		return;
	}

	if(response.status == G_VOLUME_OK)
	{
		if((response.command == G_VOLUME_MOUNT && response.info.state != G_VOLUME_MOUNTED) ||
		   (response.command == G_VOLUME_UNMOUNT && response.info.state != G_VOLUME_AVAILABLE))
			response.status = G_VOLUME_PROTOCOL_ERROR;
		if(control->name[0])
		{
			char path[64];
			snprintf(path, sizeof(path), "/mount/%s", control->name);
			if(strcmp(response.info.path, path))
				response.status = G_VOLUME_PROTOCOL_ERROR;
		}
	}
	g_send_message_tm(control->client, &response, sizeof(response), control->transaction,
					  G_MESSAGE_SEND_MODE_NON_BLOCKING);
	if(volume)
		volume->controlPending = false;
	controls->pending.erase(entry);
}

g_volume_response volumeControlHandleClaim(g_volume_controls* controls, g_tid sender, const g_volume_claim_request* request)
{
	g_volume_response response = {};
	response.command = G_VOLUME_CLAIM;
	g_volume_record* volume = registryVolumeFind(controls->registry, request->volume);
	g_volume_pending* control = volumeControlFindPending(controls, request->volume);
	bool requestedMount = control && control->handler == sender && control->command == G_VOLUME_MOUNT;
	if(!requestedMount && (adminPowerIsQuiescing() || (volume && volume->manual)))
	{
		response.status = G_VOLUME_BUSY;
		if(volume)
			response.info = volume->info;
		return response;
	}
	if(control && control->handler == sender && control->name[0] &&
	   (!memchr(request->name, 0, sizeof(request->name)) || strcmp(control->name, request->name)))
	{
		response.status = G_VOLUME_CONFLICT;
		return response;
	}
	return registryVolumeHandleClaim(controls->registry, sender, request);
}

void volumeControlReapPending(g_volume_controls* controls)
{
	auto entry = controls->pending.begin();
	while(entry != controls->pending.end())
	{
		g_volume_pending* control = &entry->second;
		if(g_get_pid_for_tid(control->handler) != G_PID_NONE)
		{
			++entry;
			continue;
		}
		g_volume_response response = {};
		response.command = control->command;
		response.status = G_VOLUME_UNAVAILABLE;
		response.info.id = control->volume;
		g_volume_record* volume = registryVolumeFind(controls->registry, control->volume);
		if(volume)
			volume->controlPending = false;
		g_send_message_tm(control->client, &response, sizeof(response), control->transaction,
						  G_MESSAGE_SEND_MODE_NON_BLOCKING);
		entry = controls->pending.erase(entry);
	}
}

g_volume_response volumeControlHandleRescan(g_volume_controls* controls, g_tid sender, const g_volume_rescan_request* request)
{
	g_volume_response response = {};
	response.command = G_VOLUME_RESCAN;
	response.info.device = request->device;

	g_volume_disk* disk = registryDiskFind(controls->registry, request->device);
	if(adminPowerIsQuiescing())
		response.status = G_VOLUME_BUSY;
	else if(!disk)
		response.status = G_VOLUME_NOT_FOUND;
	else
		response.status = registryDiscoveryScanDisk(controls->registry, &disk->info, sender, 0, false);

	return response;
}
