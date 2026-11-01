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

#ifndef LIBSTORAGE_VOLUME
#define LIBSTORAGE_VOLUME

#include <cstring>
#include "interface.hpp"

/**
 * Accepts a single public mount-name component. The boot-root alias is reserved
 * for automatic boot mounting; public names always resolve under /mount.
 */
inline bool volumeMountNameValid(const char* name)
{
	if(!name || !name[0])
		return false;
	uint32_t length = 0;
	for(; length < sizeof(g_volume_request{}.name) && name[length]; ++length)
	{
		auto c = name[length];
		if(!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
			 c == '.'))
			return false;
	}
	return length < sizeof(g_volume_request{}.name) && strcmp(name, ".") && strcmp(name, "..") &&
		   strcmp(name, "fat-root");
}

/**
 * Executes one authenticated manager request. Timeout does not cancel accepted
 * operations or release ownership. Retry CLAIM from the same serving thread to
 * recover its token, and GET to reconcile an uncertain mount or unmount.
 */
g_volume_status storageManagerRequest(const g_volume_request& request, g_volume_response* response);

g_volume_status volumeGet(g_volume_id volume, g_volume_info* info);
g_volume_status volumeNext(g_volume_id after, g_device_id device, g_volume_info* info);
g_volume_status volumeClaim(g_volume_id volume, const char* filesystem, const char* name, g_volume_info* info);
g_volume_status volumeOffer(g_volume_id volume, const char* filesystem, g_volume_info* info);
g_volume_status volumeCommit(g_volume_id volume, uint64_t claim, int32_t mount);
g_volume_status volumeRelease(g_volume_id volume, uint64_t claim);
g_volume_status volumeControl(g_volume_id volume, bool mount, g_volume_info* info);

/**
 * Mounts at /mount/name, or uses the driver's default when name is null or empty.
 * A different name for an existing mount returns a conflict without moving it.
 */
g_volume_status volumeMount(g_volume_id volume, const char* name, g_volume_info* info);

/**
 * Unmounts the selected volume. A nonzero expectedClaim rejects a replacement
 * mount, allowing callers to act on a previously resolved mount name safely.
 */
g_volume_status volumeUnmount(g_volume_id volume, g_volume_info* info, uint64_t expectedClaim = 0);
g_volume_status volumeFlush(g_volume_id volume, g_volume_info* info);
using g_volume_progress_callback = void (*)(void*, uint64_t, uint64_t);

/**
 * Queries exact filesystem space. Advancing, authenticated scan progress renews
 * the inactivity deadline and optionally reports completed/total allocation units.
 */
g_volume_status volumeStat(g_volume_id volume, g_volume_filesystem_info* info,
						   g_volume_progress_callback progress = nullptr, void* context = nullptr);

g_volume_status diskRescan(g_device_id device);

/**
 * Reserves an unmounted disk. An optional volume may subsequently be mounted
 * only through control requests from this reservation's owning client.
 */
g_volume_status diskClaim(g_device_id device, uint64_t* claim, g_volume_id allowedVolume = 0);
g_volume_status diskRelease(g_device_id device, uint64_t claim);

#endif
