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

#include "transaction.hpp"
#include <cstring>
#include <ghost.h>

static_assert(sizeof(g_block_transfer_request) <= G_MESSAGE_MAXIMUM_MESSAGE_LENGTH, "block request fits IPC");
static_assert(sizeof(g_block_get_info_response) <= G_MESSAGE_MAXIMUM_MESSAGE_LENGTH, "block response fits IPC");

static g_block_status blockDeviceReadResponse(const g_message_header* message, const g_block_header& request,
											  void* response, size_t responseSize)
{
	if(message->length != responseSize || responseSize < sizeof(g_block_response_header))
		return G_BLOCK_PROTOCOL_ERROR;

	auto content = reinterpret_cast<const uint8_t*>(message) + sizeof(*message);
	g_block_response_header header;
	memcpy(&header, content, sizeof(header));
	if(header.magic != G_BLOCK_MAGIC || header.version != G_BLOCK_VERSION || header.command != request.command ||
	   header.status > G_BLOCK_BUSY)
		return G_BLOCK_PROTOCOL_ERROR;

	memcpy(response, content, responseSize);
	memoryAtomicFence(G_MEMORY_ORDER_SEQ_CST);
	return G_BLOCK_SUCCESS;
}

g_block_status blockDeviceExchangePacket(g_tid handler, const void* request, size_t requestSize,
											void* response, size_t responseSize)
{
	if(!request || !response || requestSize < sizeof(g_block_header) || requestSize > G_MESSAGE_MAXIMUM_MESSAGE_LENGTH ||
	   responseSize > G_MESSAGE_MAXIMUM_MESSAGE_LENGTH)
		return G_BLOCK_INVALID_REQUEST;

	g_block_header requestHeader;
	memcpy(&requestHeader, request, sizeof(requestHeader));
	auto tx = g_get_message_tx_id();
	auto started = g_millis();
	unsigned quickWaits = 0;
	constexpr unsigned quickWaitLimit = 8;
	auto wait = [&]()
	{
		if(quickWaits < quickWaitLimit)
		{
			++quickWaits;
			g_yield_t(handler);
		}
		else
			g_sleep(1);
	};

	for(;;)
	{
		auto status = g_send_message_tm(handler, const_cast<void*>(request), requestSize, tx,
										G_MESSAGE_SEND_MODE_NON_BLOCKING);
		if(status == G_MESSAGE_SEND_STATUS_SUCCESSFUL)
			break;
		if(status != G_MESSAGE_SEND_STATUS_FULL)
			return G_BLOCK_UNAVAILABLE;

		if(g_millis() - started >= G_BLOCK_IPC_TIMEOUT_MS)
			return G_BLOCK_TIMEOUT;

		wait();
	}

	uint8_t incoming[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	quickWaits = 0;
	for(;;)
	{
		auto status = g_receive_message_tm(incoming, sizeof(incoming), tx, G_MESSAGE_RECEIVE_MODE_NON_BLOCKING);
		if(status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			auto message = reinterpret_cast<g_message_header*>(incoming);
			if(message->sender == handler)
				return blockDeviceReadResponse(message, requestHeader, response, responseSize);
		}
		else if(status != G_MESSAGE_RECEIVE_STATUS_EMPTY)
			return G_BLOCK_UNAVAILABLE;

		if(g_millis() - started >= G_BLOCK_IPC_TIMEOUT_MS)
			return G_BLOCK_TIMEOUT;

		wait();
	}
}
