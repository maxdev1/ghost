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

#include <libstorage/admin.hpp>

g_volume_status diskBootGet(g_device_id device, uint64_t claim, g_volume_id volume, uint64_t sectors, uint32_t identity,
							g_disk_boot_info* info)
{
	if(!device || !claim || !volume || !sectors || !info)
		return G_VOLUME_INVALID;

	g_disk_boot_get_request request;
	request.device = device;
	request.claim = claim;
	request.volume = volume;
	request.sectorCount = sectors;
	request.diskIdentity = identity;

	g_volume_admin_response response;
	auto status = storageManagerAdminRequest(request, &response);

	if(status == G_VOLUME_OK)
		*info = response.boot;
	return status;
}

g_volume_status diskBootWrite(const g_disk_boot_info& boot, uint64_t first, const void* bytes, size_t size,
							  g_volume_job_info* job)
{
	if(!bytes || !size || size > G_DISK_BOOT_CHUNK_SIZE || size % 512 || !first || first >= boot.bootSectors ||
	   size / 512 > boot.bootSectors - first)
		return G_VOLUME_INVALID;

	g_disk_boot_write_request request;
	request.boot = boot;
	request.firstSector = first;
	request.dataSize = size;
	memcpy(request.data, bytes, size);
	return storageManagerAdminStart(request, job);
}

g_volume_status diskBootCommit(const g_disk_boot_info& boot, const void* code, size_t size, bool activate,
							   g_volume_job_info* job)
{
	if(!code || size != G_DISK_BOOT_CODE_SIZE)
		return G_VOLUME_INVALID;

	g_disk_boot_commit_request request;
	request.boot = boot;
	request.flags = activate ? G_DISK_BOOT_ACTIVATE : 0;
	memcpy(request.code, code, size);
	return storageManagerAdminStart(request, job);
}
