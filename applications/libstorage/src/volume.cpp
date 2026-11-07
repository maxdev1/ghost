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
static g_volume_status storageManagerExchange(const Request& request, g_volume_info* info)
{
	g_volume_response response;
	auto status = storageManagerRequest(request, &response);
	if constexpr(Request::COMMAND != G_VOLUME_RESCAN && Request::COMMAND != G_VOLUME_RELEASE_DISK)
	{
		if(info && (status == G_VOLUME_OK || (request.volume && response.info.id == request.volume)))
			*info = response.info;
	}

	return status;
}

g_volume_status volumeGet(g_volume_id volume, g_volume_info* info)
{
	if(!volume || !info)
		return G_VOLUME_INVALID;

	g_volume_get_request request;
	request.volume = volume;
	return storageManagerExchange(request, info);
}

g_volume_status volumeNext(g_volume_id after, g_device_id device, g_volume_info* info)
{
	if(!info)
		return G_VOLUME_INVALID;

	g_volume_list_request request;
	request.volume = after;
	request.device = device;
	return storageManagerExchange(request, info);
}

g_volume_status volumeClaim(g_volume_id volume, const char* filesystem, const char* name, g_volume_info* info)
{
	g_volume_claim_request request;
	if(!volume || !info || !filesystem || !name || !*filesystem || !*name ||
	   strlen(filesystem) >= sizeof(request.filesystem) || strlen(name) >= sizeof(request.name))
		return G_VOLUME_INVALID;

	request.volume = volume;
	strcpy(request.filesystem, filesystem);
	strcpy(request.name, name);
	return storageManagerExchange(request, info);
}

g_volume_status volumeCommit(g_volume_id volume, uint64_t claim, int32_t mount)
{
	g_volume_commit_request request;
	request.volume = volume;
	request.claim = claim;
	request.mount = mount;
	return storageManagerExchange(request, nullptr);
}

g_volume_status volumeRelease(g_volume_id volume, uint64_t claim)
{
	g_volume_release_request request;
	request.volume = volume;
	request.claim = claim;
	return storageManagerExchange(request, nullptr);
}

g_volume_status volumeControl(g_volume_id volume, bool mount, g_volume_info* info)
{
	return mount ? volumeMount(volume, nullptr, info) : volumeUnmount(volume, info);
}

g_volume_status volumeMount(g_volume_id volume, const char* name, g_volume_info* info)
{
	if(!volume || (name && name[0] && !volumeMountNameValid(name)))
		return G_VOLUME_INVALID;

	g_volume_mount_request request;
	request.volume = volume;
	if(name)
		strcpy(request.name, name);
	return storageManagerExchange(request, info);
}

g_volume_status volumeUnmount(g_volume_id volume, g_volume_info* info, uint64_t expectedClaim)
{
	if(!volume)
		return G_VOLUME_INVALID;

	g_volume_unmount_request request;
	request.volume = volume;
	request.claim = expectedClaim;
	return storageManagerExchange(request, info);
}

g_volume_status volumeFlush(g_volume_id volume, g_volume_info* info)
{
	g_volume_flush_request request;
	request.volume = volume;
	return storageManagerExchange(request, info);
}

g_volume_status volumeOffer(g_volume_id volume, const char* filesystem, g_volume_info* info)
{
	if(!volume || !filesystem || !*filesystem || strlen(filesystem) >= sizeof(g_volume_offer_request{}.filesystem))
		return G_VOLUME_INVALID;

	g_volume_offer_request request;
	request.volume = volume;
	strcpy(request.filesystem, filesystem);
	return storageManagerExchange(request, info);
}

g_volume_status volumeStat(g_volume_id volume, g_volume_filesystem_info* info, g_volume_progress_callback progress,
						   void* context)
{
	if(!volume || !info)
		return G_VOLUME_INVALID;

	g_volume_stat_request request;
	request.volume = volume;
	g_volume_response response;
	auto status = storageManagerRequestProgress(&request, sizeof(request), &response, progress, context);
	if(status == G_VOLUME_OK)
	{
		const auto& fs = response.filesystem;
		if(!memchr(fs.type, 0, sizeof(fs.type)) || !fs.type[0] || !fs.allocationUnit || fs.freeBytes > fs.totalBytes ||
		   fs.writable > 1)
			return G_VOLUME_PROTOCOL_ERROR;

		*info = fs;
	}

	return status;
}

g_volume_status diskRescan(g_device_id device)
{
	g_volume_rescan_request request;
	request.device = device;
	return storageManagerExchange(request, nullptr);
}

g_volume_status diskClaim(g_device_id device, uint64_t* claim, g_volume_id allowedVolume)
{
	if(!claim)
		return G_VOLUME_INVALID;

	g_volume_claim_disk_request request;
	request.device = device;
	request.volume = allowedVolume;
	g_volume_response response;
	auto status = storageManagerRequest(request, &response);
	if(status == G_VOLUME_OK)
		*claim = response.info.claim;
	return status;
}

g_volume_status diskRelease(g_device_id device, uint64_t claim)
{
	g_volume_release_disk_request request;
	request.device = device;
	request.claim = claim;
	return storageManagerExchange(request, nullptr);
}
