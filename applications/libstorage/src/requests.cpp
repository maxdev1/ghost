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

template<typename Request>
static bool storageManagerResponseMatches(const void* bytes, const g_volume_info& info)
{
	Request request;
	memcpy(&request, bytes, sizeof(request));
	if constexpr(Request::COMMAND == G_VOLUME_LIST)
		return info.id > request.volume && (!request.device || info.device == request.device);
	else if constexpr(Request::COMMAND == G_VOLUME_RESCAN || Request::COMMAND == G_VOLUME_CLAIM_DISK ||
					  Request::COMMAND == G_VOLUME_RELEASE_DISK)
		return info.device == request.device;
	else
		return info.id == request.volume;
}

static bool storageManagerResponseValid(const g_volume_header& header, const void* bytes,
										   const g_volume_response& response)
{
	const auto& info = response.info;
	if(response.magic != G_VOLUME_RESPONSE_MAGIC || response.version != G_VOLUME_VERSION ||
	   response.command != header.command || response.status > G_VOLUME_PROTOCOL_ERROR ||
	   info.state > G_VOLUME_MOUNTED || info.scheme > G_VOLUME_GPT || info.filesystem[sizeof(info.filesystem) - 1] ||
	   info.path[sizeof(info.path) - 1])
		return false;
	if(response.status != G_VOLUME_OK)
		return true;

	switch(header.command)
	{
		case G_VOLUME_GET:
			return storageManagerResponseMatches<g_volume_get_request>(bytes, info);
		case G_VOLUME_LIST:
			return storageManagerResponseMatches<g_volume_list_request>(bytes, info);
		case G_VOLUME_CLAIM:
			return storageManagerResponseMatches<g_volume_claim_request>(bytes, info);
		case G_VOLUME_COMMIT:
			return storageManagerResponseMatches<g_volume_commit_request>(bytes, info);
		case G_VOLUME_RELEASE:
			return storageManagerResponseMatches<g_volume_release_request>(bytes, info);
		case G_VOLUME_MOUNT:
			return storageManagerResponseMatches<g_volume_mount_request>(bytes, info);
		case G_VOLUME_UNMOUNT:
			return storageManagerResponseMatches<g_volume_unmount_request>(bytes, info);
		case G_VOLUME_RESCAN:
			return storageManagerResponseMatches<g_volume_rescan_request>(bytes, info);
		case G_VOLUME_CLAIM_DISK:
			return storageManagerResponseMatches<g_volume_claim_disk_request>(bytes, info);
		case G_VOLUME_RELEASE_DISK:
			return storageManagerResponseMatches<g_volume_release_disk_request>(bytes, info);
		case G_VOLUME_FLUSH:
			return storageManagerResponseMatches<g_volume_flush_request>(bytes, info);
		case G_VOLUME_OFFER:
			return storageManagerResponseMatches<g_volume_offer_request>(bytes, info);
		case G_VOLUME_STAT:
			return storageManagerResponseMatches<g_volume_stat_request>(bytes, info);
		default:
			return false;
	}
}

g_volume_status storageManagerRequestProgress(const void* request, size_t size, g_volume_response* response,
											  g_volume_progress_callback progress, void* context)
{
	if(!request || !response || size < sizeof(g_volume_header) || size > G_MESSAGE_MAXIMUM_MESSAGE_LENGTH)
		return G_VOLUME_INVALID;

	g_volume_header header;
	memcpy(&header, request, sizeof(header));
	if(header.magic != G_VOLUME_MAGIC || header.version != G_VOLUME_VERSION ||
	   storageManagerRequestSize(header.command) != size)
		return G_VOLUME_INVALID;

	*response = {};

	g_volume_exchange exchange;
	auto status = storageManagerClientSend(request, size, exchange);
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
		if(!storageManagerResponseValid(header, request, reply))
			return G_VOLUME_PROTOCOL_ERROR;

		if(!reply.total && !reply.completed)
		{
			*response = reply;
			return reply.status;
		}

		if(header.command != G_VOLUME_STAT || reply.status != G_VOLUME_OK || !reply.total ||
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

g_volume_status storageManagerRequest(const void* request, size_t size, g_volume_response* response)
{
	return storageManagerRequestProgress(request, size, response, nullptr, nullptr);
}
