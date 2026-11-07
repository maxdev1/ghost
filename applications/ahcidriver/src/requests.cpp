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

template<typename Response>
static void ahciSendResponse(const g_message_header* message, Response& response)
{
	g_send_message_tm(message->sender, &response, sizeof(response), message->transaction,
					  G_MESSAGE_SEND_MODE_NON_BLOCKING);
}

static void ahciRejectRequest(const g_message_header* message, const g_block_header& header)
{
	auto reply = [&](auto response)
	{
		response.header.command = header.command;
		ahciSendResponse(message, response);
	};

	switch(header.command)
	{
		case G_BLOCK_GET_INFO: return reply(g_block_get_info_response{});
		case G_BLOCK_FLUSH: return reply(g_block_flush_response{});
		case G_BLOCK_READ:
		case G_BLOCK_WRITE: return reply(g_block_transfer_response{});
		case G_BLOCK_OPEN_BUFFER:
		case G_BLOCK_CLOSE_BUFFER: return reply(g_block_buffer_mapping_response{});
		case G_BLOCK_RELEASE_BUFFER: return reply(g_block_release_buffer_response{});
		default:
		{
			g_block_response_header response{G_BLOCK_MAGIC, G_BLOCK_VERSION, header.command, G_BLOCK_UNSUPPORTED};
			ahciSendResponse(message, response);
		}
	}
}

template<typename Response>
static g_ahci_device* ahciResponseDevice(g_device_id device, Response& response)
{
	response.device = device;
	auto disk = ahciFindDisk(device);
	if(!disk)
		response.header.status = G_BLOCK_DEVICE_NOT_FOUND;
	else if(!disk->online)
		response.header.status = G_BLOCK_UNAVAILABLE;
	else
		return disk;

	return nullptr;
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

	if(message->length < sizeof(g_block_header))
		return;

	g_block_header header;
	memcpy(&header, content, sizeof(header));
	if(header.magic != G_BLOCK_MAGIC)
		return;
	if(header.version != G_BLOCK_VERSION || blockDeviceRequestSize(header.command) != message->length)
	{
		ahciRejectRequest(message, header);
		return;
	}

	switch(header.command)
	{
		case G_BLOCK_GET_INFO:
		{
			g_block_get_info_request request;
			memcpy(&request, content, sizeof(request));

			g_block_get_info_response response;
			response.header.command = header.command;
			if(auto disk = ahciResponseDevice(request.device, response))
			{
				response.info = disk->info;
				response.header.status = G_BLOCK_SUCCESS;
			}

			ahciSendResponse(message, response);
			break;
		}
		case G_BLOCK_FLUSH:
		{
			g_block_flush_request request;
			memcpy(&request, content, sizeof(request));

			g_block_flush_response response;
			response.header.command = header.command;
			if(auto disk = ahciResponseDevice(request.device, response))
				response.header.status = disk->flushCommand ?
					ahciExecuteCommand(disk, disk->flushCommand, 0, 0, 0, false) : G_BLOCK_UNSUPPORTED;

			ahciSendResponse(message, response);
			break;
		}
		case G_BLOCK_READ:
		case G_BLOCK_WRITE:
		{
			g_block_transfer_request request;
			memcpy(&request, content, sizeof(request));

			g_block_transfer_response response;
			response.header.command = header.command;
			response.buffer = request.buffer;
			if(auto disk = ahciResponseDevice(request.device, response))
			{
				response.header.status = ahciTransfer(disk, request, message->sender);
				if(response.header.status == G_BLOCK_SUCCESS)
					response.sectorsTransferred = request.sectorCount;
			}

			ahciSendResponse(message, response);
			break;
		}
		case G_BLOCK_OPEN_BUFFER:
		case G_BLOCK_CLOSE_BUFFER:
		{
			g_block_buffer_request request;
			memcpy(&request, content, sizeof(request));

			g_block_buffer_mapping_response response;
			response.header.command = header.command;
			response.buffer = request.buffer;
			response.header.status = ahciHandleBufferRequest(request, message->sender, &response.mapping);

			ahciSendResponse(message, response);
			break;
		}
		case G_BLOCK_RELEASE_BUFFER:
		{
			g_block_release_buffer_request request;
			memcpy(&request, content, sizeof(request));

			g_block_release_buffer_response response;
			response.header.command = header.command;
			response.buffer = request.buffer;
			g_block_buffer_mapping mapping{};
			response.header.status = ahciHandleBufferRequest(request, message->sender, &mapping);

			ahciSendResponse(message, response);
			break;
		}
		default:
			ahciRejectRequest(message, header);
			break;
	}
}
