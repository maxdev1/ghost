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

#include <ghost/system.h>
#include <ghost/tasks.h>
#include <stdio.h>
#include "administration/jobs.hpp"
#include "administration/requests.hpp"
#include "administration/power.hpp"
#include "volume/controls.hpp"
#include "registry/discovery.hpp"
#include "registry/owners.hpp"

static g_volume_registry storageManagerRegistry = {};
static g_volume_controls storageManagerControls = {&storageManagerRegistry, {}};

static void storageManagerHandleMessage(const g_message_header* message, const void* content)
{
	auto header = (const g_volume_header*) content;
	if(message->length < sizeof(*header))
		return;

	switch(header->magic)
	{
		case G_POWER_EVENT_MAGIC:
			adminPowerHandleEvent(message, (const g_power_event*) content);
			return;
		case G_VOLUME_ADMIN_MAGIC:
			adminRequestHandle(&storageManagerRegistry, message, (const g_volume_admin_header*) content);
			return;
		case G_VOLUME_RESPONSE_MAGIC:
			volumeControlHandleResponse(&storageManagerControls, message, (const g_volume_response*) content);
			return;
		case G_VOLUME_MAGIC:
			break;
		default:
			return;
	}

	g_volume_response response = {};
	response.command = header->command;
	response.status = G_VOLUME_PROTOCOL_ERROR;
	if(header->version != G_VOLUME_VERSION || storageManagerRequestSize(header->command) != message->length)
	{
		g_send_message_tm(message->sender, &response, sizeof(response), message->transaction,
						  G_MESSAGE_SEND_MODE_NON_BLOCKING);
		return;
	}

	if(message->sender == G_TID_NONE)
		return;

	switch(header->command)
	{
		case G_VOLUME_GET:
		{
			auto request = (const g_volume_get_request*) content;
			response = registryVolumeHandleGet(&storageManagerRegistry, request);
			break;
		}
		case G_VOLUME_LIST:
		{
			auto request = (const g_volume_list_request*) content;
			response = registryVolumeHandleList(&storageManagerRegistry, request);
			break;
		}
		case G_VOLUME_CLAIM:
		{
			auto request = (const g_volume_claim_request*) content;
			response = volumeControlHandleClaim(&storageManagerControls, message->sender, request);
			break;
		}
		case G_VOLUME_COMMIT:
		{
			auto request = (const g_volume_commit_request*) content;
			response = registryVolumeHandleCommit(&storageManagerRegistry, message->sender, request);
			break;
		}
		case G_VOLUME_RELEASE:
		{
			auto request = (const g_volume_release_request*) content;
			response = registryVolumeHandleRelease(&storageManagerRegistry, message->sender, request);
			break;
		}
		case G_VOLUME_OFFER:
		{
			auto request = (const g_volume_offer_request*) content;
			response = registryVolumeHandleOffer(&storageManagerRegistry, message->sender, request);
			break;
		}
		case G_VOLUME_CLAIM_DISK:
		{
			auto request = (const g_volume_claim_disk_request*) content;
			response = registryDiskHandleClaim(&storageManagerRegistry, message->sender, request);
			break;
		}
		case G_VOLUME_RELEASE_DISK:
		{
			auto request = (const g_volume_release_disk_request*) content;
			response = registryDiskHandleRelease(&storageManagerRegistry, message->sender, request);
			break;
		}
		case G_VOLUME_RESCAN:
		{
			auto request = (const g_volume_rescan_request*) content;
			response = volumeControlHandleRescan(&storageManagerControls, message->sender, request);
			break;
		}
		case G_VOLUME_MOUNT:
			volumeControlHandleMount(&storageManagerControls, message->sender, message->transaction,
							   (const g_volume_mount_request*) content);
			return;
		case G_VOLUME_UNMOUNT:
			volumeControlHandleUnmount(&storageManagerControls, message->sender, message->transaction,
								 (const g_volume_unmount_request*) content);
			return;
		case G_VOLUME_FLUSH:
			volumeControlHandleFlush(&storageManagerControls, message->sender, message->transaction,
							   (const g_volume_flush_request*) content);
			return;
		case G_VOLUME_STAT:
			volumeControlHandleStat(&storageManagerControls, message->sender, message->transaction,
							  (const g_volume_stat_request*) content);
			return;
		default:
			return;
	}
	g_send_message_tm(message->sender, &response, sizeof(response), message->transaction,
					  G_MESSAGE_SEND_MODE_NON_BLOCKING);
}

int main()
{
	if(!g_task_register_name(G_STORAGE_MANAGER_NAME))
		return 1;

	for(;;)
	{
		g_power_status status = g_power_register(G_POWER_STORAGE_PRODUCER);
		if(status == G_POWER_SUCCESSFUL)
			break;
		if(status != G_POWER_BUSY)
			return 1;
		g_sleep(1);
	}

	g_tid deviceManager = g_task_await_by_name(G_DEVICE_MANAGER_NAME);
	g_message_transaction deviceCursor = G_MESSAGE_TOPIC_TRANSACTION_START;
	uint64_t lastOwnerReap = 0;
	uint8_t incoming[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH] __attribute__((aligned(8)));

	klog("manager ready");
	for(;;)
	{
		bool received = false;
		if(g_receive_message_m(incoming, sizeof(incoming), G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) ==
		   G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			storageManagerHandleMessage((g_message_header*) incoming, G_MESSAGE_CONTENT(incoming));
			received = true;
		}

		if(!adminPowerIsQuiescing() &&
		   g_receive_topic_message_m(G_DEVICE_EVENT_TOPIC, incoming, sizeof(incoming), deviceCursor,
									 G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			auto message = (g_message_header*) incoming;
			deviceCursor = message->transaction;
			registryDiscoveryHandleDevice(&storageManagerRegistry, deviceManager, message,
							  (const g_device_event_device_registered*) G_MESSAGE_CONTENT(incoming));
			received = true;
		}

		if(g_millis() - lastOwnerReap >= 100)
		{
			registryOwnerReap(&storageManagerRegistry);
			volumeControlReapPending(&storageManagerControls);
			lastOwnerReap = g_millis();
		}

		adminJobPoll(&storageManagerRegistry);
		adminPowerPoll(volumeControlHasPending(&storageManagerControls));
		if(!received)
			g_sleep(1);
	}
}
