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

#include "client.hpp"

static bool storageManagerResponseValid(const g_volume_request& request, const g_volume_response& response)
{
	const auto& info = response.info;
	if(response.magic != G_VOLUME_MAGIC || response.version != G_VOLUME_VERSION ||
	   response.command != request.command || response.status > G_VOLUME_PROTOCOL_ERROR ||
	   info.state > G_VOLUME_MOUNTED || info.scheme > G_VOLUME_GPT || info.filesystem[sizeof(info.filesystem) - 1] ||
	   info.path[sizeof(info.path) - 1])
		return false;
	if(response.status != G_VOLUME_OK)
		return true;

	switch(request.command)
	{
		case G_VOLUME_GET:
		case G_VOLUME_CLAIM:
		case G_VOLUME_COMMIT:
		case G_VOLUME_RELEASE:
		case G_VOLUME_MOUNT:
		case G_VOLUME_UNMOUNT:
		case G_VOLUME_FLUSH:
		case G_VOLUME_OFFER:
		case G_VOLUME_STAT:
			return info.id == request.volume;
		case G_VOLUME_LIST:
			return info.id > request.volume && (!request.device || info.device == request.device);
		default:
			return info.device == request.device;
	}
}

g_volume_status storageManagerRequestProgress(const g_volume_request& request, g_volume_response* response,
											  g_volume_progress_callback progress, void* context)
{
	if(!response || request.magic != G_VOLUME_MAGIC || request.version != G_VOLUME_VERSION ||
	   request.command < G_VOLUME_GET || request.command > G_VOLUME_STAT || request.reserved)
		return G_VOLUME_INVALID;
	*response = {};

	g_volume_exchange exchange;
	auto status = storageManagerClientSend(request, exchange);
	if(status != G_VOLUME_OK)
		return status;

	uint64_t completed = 0;
	uint64_t total = 0;
	for(;;)
	{
		g_volume_response reply;
		status = storageManagerClientReceive(exchange, &reply, sizeof(reply));
		if(status != G_VOLUME_OK)
			return status;
		if(!storageManagerResponseValid(request, reply))
			return G_VOLUME_PROTOCOL_ERROR;

		if(!reply.total && !reply.completed)
		{
			*response = reply;
			return reply.status;
		}

		if(request.command != G_VOLUME_STAT || reply.status != G_VOLUME_OK || !reply.total ||
		   reply.completed > reply.total || (total && (reply.total != total || reply.completed < completed)))
			return G_VOLUME_PROTOCOL_ERROR;
		if(!total || reply.completed > completed)
		{
			exchange.started = g_millis();
			if(progress)
				progress(context, reply.completed, reply.total);
		}

		completed = reply.completed;
		total = reply.total;
	}
}

g_volume_status storageManagerRequest(const g_volume_request& request, g_volume_response* response)
{
	return storageManagerRequestProgress(request, response, nullptr, nullptr);
}
