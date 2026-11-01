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

#include <algorithm>
#include <cstring>
#include "storage.hpp"

g_volume_status diskBootInfo(const g_volume_disk_snapshot& snapshot, const g_volume_admin_request& request,
							 g_disk_boot_info* info)
{
	const auto& disk = snapshot.disk;
	if(disk.sectorSize != G_VOLUME_SECTOR_SIZE || disk.layout != G_DISK_MBR || !diskHasSignature(snapshot.mbr))
		return G_VOLUME_UNSUPPORTED;

	if(!request.sectorCount || request.sectorCount > disk.sectorCount)
		return G_VOLUME_INVALID;

	const g_disk_partition_info* selected = nullptr;
	for(const auto& part: snapshot.partitions)
	{
		if(part.firstSector < request.sectorCount)
			return G_VOLUME_UNSUPPORTED;
		if(part.number == request.partition)
			selected = &part;
	}

	if(!selected || selected->scheme != G_VOLUME_MBR || selected->container || selected->number < 1 ||
	   selected->number > 4 || !selected->sectorCount)
		return G_VOLUME_UNSUPPORTED;

	auto identity = diskRead32(snapshot.mbr + 440);
	if(!identity)
		identity = request.diskIdentity;
	if(!identity)
		return G_VOLUME_INVALID;

	*info = {};
	info->device = disk.device;
	info->volume = request.volume;
	info->claim = request.claim;
	info->generation = disk.generation;
	info->diskIdentity = identity;
	info->partition = selected->number;
	info->firstSector = selected->firstSector;
	info->sectorCount = selected->sectorCount;
	info->bootSectors = request.sectorCount;
	return G_VOLUME_OK;
}

static g_volume_status diskBootCheckWrite(const g_volume_disk_snapshot& snapshot, const g_volume_admin_request& request)
{
	g_disk_boot_info info;
	auto status = diskBootInfo(snapshot, request, &info);
	if(status != G_VOLUME_OK)
		return status;
	return request.diskIdentity == info.diskIdentity ? G_VOLUME_OK : G_VOLUME_CONFLICT;
}

g_volume_status diskBootWrite(const g_volume_disk_snapshot& snapshot, const g_volume_admin_request& request,
							  const g_volume_storage_io& io)
{
	auto status = diskBootCheckWrite(snapshot, request);
	if(status != G_VOLUME_OK)
		return status;
	if(request.flags || !request.dataSize || request.dataSize > sizeof(request.data) ||
	   request.dataSize % G_VOLUME_SECTOR_SIZE || !request.firstSector || request.firstSector >= request.sectorCount ||
	   request.dataSize / G_VOLUME_SECTOR_SIZE > request.sectorCount - request.firstSector || !io.maxSectors ||
	   io.maxSectors > G_VOLUME_MAX_TRANSFER_SECTORS)
		return G_VOLUME_INVALID;
	uint8_t actual[G_DISK_BOOT_CHUNK_SIZE];
	if(!io.read(io.context, 0, actual, 1))
		return G_VOLUME_IO_ERROR;
	if(memcmp(actual, snapshot.mbr, G_VOLUME_SECTOR_SIZE))
		return G_VOLUME_CONFLICT;
	auto sectors = request.dataSize / G_VOLUME_SECTOR_SIZE;
	for(uint32_t done = 0; done < sectors;)
	{
		auto count = std::min(uint32_t(io.maxSectors), uint32_t(sectors - done));
		if(!io.write(io.context, request.firstSector + done, request.data + done * G_VOLUME_SECTOR_SIZE, count))
			return G_VOLUME_IO_ERROR;
		done += count;
	}
	if(!io.flush(io.context))
		return G_VOLUME_IO_ERROR;
	for(uint32_t done = 0; done < sectors;)
	{
		auto count = std::min(uint32_t(io.maxSectors), uint32_t(sectors - done));
		if(!io.read(io.context, request.firstSector + done, actual, count) ||
		   memcmp(actual, request.data + done * G_VOLUME_SECTOR_SIZE, count * G_VOLUME_SECTOR_SIZE))
			return G_VOLUME_IO_ERROR;
		done += count;
	}
	return G_VOLUME_OK;
}

g_volume_status diskBootCommit(const g_volume_disk_snapshot& snapshot, const g_volume_admin_request& request,
							   const g_volume_storage_io& io)
{
	auto status = diskBootCheckWrite(snapshot, request);
	if(status != G_VOLUME_OK)
		return status;
	if(request.dataSize != G_DISK_BOOT_CODE_SIZE || request.firstSector || (request.flags & ~G_DISK_BOOT_ACTIVATE))
		return G_VOLUME_INVALID;
	uint8_t mbr[G_VOLUME_SECTOR_SIZE];
	memcpy(mbr, snapshot.mbr, sizeof(mbr));
	memcpy(mbr, request.data, G_DISK_BOOT_CODE_SIZE);
	diskWrite32(mbr + 440, request.diskIdentity);
	if(request.flags & G_DISK_BOOT_ACTIVATE)
		for(unsigned i = 0; i < 4; ++i)
			mbr[446 + i * 16] = i + 1 == request.partition ? 0x80 : 0;
	return diskTableWrite(snapshot, mbr, io);
}
