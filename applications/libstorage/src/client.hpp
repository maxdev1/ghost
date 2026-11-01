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

#ifndef LIBSTORAGE_CLIENT
#define LIBSTORAGE_CLIENT

#include <cstring>
#include <ghost/messages.h>
#include <ghost/system.h>
#include <ghost/tasks.h>
#include <libstorage/volume.hpp>

struct g_volume_exchange
{
	g_tid service;
	g_message_transaction transaction;
	uint64_t started;
};

/**
 * Retries only queue admission. Once the service accepts the request, callers
 * wait for its reply without submitting the operation again.
 */
template<typename Request>
static g_volume_status storageManagerClientSend(const Request& request, g_volume_exchange& exchange)
{
	static_assert(sizeof(Request) <= G_MESSAGE_MAXIMUM_MESSAGE_LENGTH, "volume request fits IPC");
	exchange.service = g_task_get_by_name(G_STORAGE_MANAGER_NAME);
	if(exchange.service == G_TID_NONE)
		return G_VOLUME_UNAVAILABLE;

	exchange.transaction = g_get_message_tx_id();
	exchange.started = g_millis();
	auto outgoing = request;
	for(;;)
	{
		auto status = g_send_message_tm(exchange.service, &outgoing, sizeof(outgoing), exchange.transaction,
										G_MESSAGE_SEND_MODE_NON_BLOCKING);
		if(status == G_MESSAGE_SEND_STATUS_SUCCESSFUL)
			return G_VOLUME_OK;
		if(status != G_MESSAGE_SEND_STATUS_FULL)
			return G_VOLUME_UNAVAILABLE;
		if(g_millis() - exchange.started >= G_VOLUME_TIMEOUT_MS)
			return G_VOLUME_TIMED_OUT;
		g_sleep(1);
	}
}

/**
 * Receives one fixed-size reply from the same registered manager. The caller
 * validates its protocol fields and may renew started on advancing progress.
 */
static inline g_volume_status storageManagerClientReceive(g_volume_exchange& exchange, void* response, size_t size)
{
	uint8_t bytes[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	while(g_millis() - exchange.started < G_VOLUME_TIMEOUT_MS)
	{
		if(g_task_get_by_name(G_STORAGE_MANAGER_NAME) != exchange.service)
			return G_VOLUME_UNAVAILABLE;
		auto status =
				g_receive_message_tm(bytes, sizeof(bytes), exchange.transaction, G_MESSAGE_RECEIVE_MODE_NON_BLOCKING);
		if(status == G_MESSAGE_RECEIVE_STATUS_EMPTY)
		{
			g_sleep(1);
			continue;
		}
		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			return G_VOLUME_UNAVAILABLE;

		auto header = reinterpret_cast<g_message_header*>(bytes);
		if(header->sender != exchange.service)
			continue;
		if(header->length != size)
			return G_VOLUME_PROTOCOL_ERROR;
		memcpy(response, G_MESSAGE_CONTENT(bytes), size);
		return G_VOLUME_OK;
	}
	return G_VOLUME_TIMED_OUT;
}

g_volume_status storageManagerRequestProgress(const g_volume_request& request, g_volume_response* response,
											  g_volume_progress_callback progress, void* context);

#endif
