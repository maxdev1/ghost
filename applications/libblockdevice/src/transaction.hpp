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

#ifndef LIBBLOCKDEVICE_TRANSACTION
#define LIBBLOCKDEVICE_TRANSACTION

#include <cstddef>
#include <type_traits>
#include <libblockdevice/driver.hpp>

/**
 * Sends a control request once accepted and waits for an authenticated reply.
 * A failed exchange does not establish that a shared DMA page is idle.
 */
g_block_status blockDeviceExchangePacket(g_tid handler, const void* request, size_t requestSize,
											void* response, size_t responseSize);

template<typename Request, typename Response>
g_block_status blockDeviceExchange(g_tid handler, const Request& request, Response* response)
{
	static_assert(std::is_standard_layout<Request>::value && std::is_trivially_copyable<Request>::value,
				  "block requests have a fixed wire layout");
	static_assert(std::is_standard_layout<Response>::value && std::is_trivially_copyable<Response>::value,
				  "block responses have a fixed wire layout");

	if(!response || blockDeviceRequestSize(request.header.command) != sizeof(Request) ||
	   blockDeviceResponseSize(request.header.command) != sizeof(Response))
		return G_BLOCK_INVALID_REQUEST;

	auto status = blockDeviceExchangePacket(handler, &request, sizeof(request), response, sizeof(*response));
	if(status != G_BLOCK_SUCCESS)
		return status;

	if constexpr(std::is_same<Request, g_block_device_request>::value ||
				 std::is_same<Request, g_block_transfer_request>::value)
	{
		if(response->device != request.device)
			return G_BLOCK_PROTOCOL_ERROR;
	}

	if constexpr(std::is_same<Request, g_block_buffer_request>::value ||
				 std::is_same<Request, g_block_transfer_request>::value)
	{
		if(response->buffer != request.buffer)
			return G_BLOCK_PROTOCOL_ERROR;
	}

	if constexpr(std::is_same<Request, g_block_transfer_request>::value)
	{
		auto expected = response->header.status == G_BLOCK_SUCCESS ? request.sectorCount : 0;
		if(response->sectorsTransferred != expected)
			return G_BLOCK_PROTOCOL_ERROR;
	}

	return response->header.status;
}

#endif
