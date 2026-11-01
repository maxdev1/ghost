/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "ahcidriver.hpp"
#include <cstddef>
#include <cstring>

struct g_ahci_response
{
	g_block_response header;
	uint8_t payload[sizeof(g_block_device_info)];
	size_t payloadBytes = 0;
};

static_assert(offsetof(g_ahci_response, payload) == sizeof(g_block_response), "block response payload ABI");

static bool ahciIsBufferCommand(g_block_command command)
{
	return command == G_BLOCK_OPEN_BUFFER || command == G_BLOCK_CLOSE_BUFFER || command == G_BLOCK_RELEASE_BUFFER;
}

static g_block_status ahciHandleBufferCommand(const g_block_request& request, g_tid sender, g_ahci_response& response)
{
	g_block_buffer_mapping mapping{};
	auto status = ahciHandleBufferRequest(request, sender, &mapping);
	if(status == G_BLOCK_SUCCESS && request.command != G_BLOCK_RELEASE_BUFFER)
	{
		memcpy(response.payload, &mapping, sizeof(mapping));
		response.payloadBytes = sizeof(mapping);
	}
	return status;
}

static g_block_status ahciHandleDeviceCommand(const g_block_request& request, g_tid sender, g_ahci_response& response)
{
	auto disk = ahciFindDisk(request.device);
	if(!disk)
		return G_BLOCK_DEVICE_NOT_FOUND;
	if(!disk->online)
		return G_BLOCK_UNAVAILABLE;

	switch(request.command)
	{
		case G_BLOCK_GET_INFO:
		case G_BLOCK_FLUSH:
			if(request.lba || request.sectorCount || request.buffer || request.offset)
				return G_BLOCK_INVALID_REQUEST;
			if(request.command == G_BLOCK_GET_INFO)
			{
				memcpy(response.payload, &disk->info, sizeof(disk->info));
				response.payloadBytes = sizeof(disk->info);
				return G_BLOCK_SUCCESS;
			}
			if(!disk->flushCommand)
				return G_BLOCK_UNSUPPORTED;
			return ahciExecuteCommand(disk, disk->flushCommand, 0, 0, 0, false);
		case G_BLOCK_READ:
		case G_BLOCK_WRITE:
		{
			auto status = ahciTransfer(disk, request, sender);
			if(status == G_BLOCK_SUCCESS)
				response.header.sectorsTransferred = request.sectorCount;
			return status;
		}
		default:
			return G_BLOCK_UNSUPPORTED;
	}
}

[[noreturn]] void ahciReceiveMessages()
{
	uint8_t incoming[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	for(;;)
	{
		if(g_receive_message(incoming, sizeof(incoming)) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			ahciHandleRequest(reinterpret_cast<g_message_header*>(incoming), G_MESSAGE_CONTENT(incoming));
	}
}

void ahciHandleRequest(const g_message_header* message, const uint8_t* content)
{
	if(message->sender == G_TID_NONE && message->length == sizeof(g_power_event))
	{
		g_power_event event;
		memcpy(&event, content, sizeof(event));
		if(event.magic == G_POWER_EVENT_MAGIC && event.type == G_POWER_EVENT_PREPARE)
			g_power_acknowledge(event.request, ahciPowerFlush());
		return;
	}
	if(message->length < sizeof(g_block_request) || message->length > G_MESSAGE_MAXIMUM_MESSAGE_LENGTH)
		return;
	g_block_request request;
	memcpy(&request, content, sizeof(request));
	g_ahci_response response{};
	response.header = {G_BLOCK_MAGIC, G_BLOCK_VERSION, request.command, request.device, 0, G_BLOCK_INVALID_REQUEST, 0,
					   request.buffer};

	if(message->length == sizeof(request) && request.magic == G_BLOCK_MAGIC && request.version == G_BLOCK_VERSION &&
	   !request.reserved)
	{
		response.header.status = ahciIsBufferCommand(request.command)
										 ? ahciHandleBufferCommand(request, message->sender, response)
										 : ahciHandleDeviceCommand(request, message->sender, response);
	}

	g_send_message_tm(message->sender, &response, sizeof(response.header) + response.payloadBytes, message->transaction,
					  G_MESSAGE_SEND_MODE_NON_BLOCKING);
}
