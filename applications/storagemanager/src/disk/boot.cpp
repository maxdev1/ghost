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

#include <string.h>
#include "storage.hpp"

g_volume_status diskBootGetInfo(const g_volume_disk_snapshot* snapshot, const g_disk_boot_info* request,
							 g_disk_boot_info* info)
{
	const g_disk_info* disk = &snapshot->disk;
	if(disk->sectorSize != G_VOLUME_SECTOR_SIZE || disk->layout != G_DISK_MBR || !diskLayoutHasBootSignature(snapshot->mbr))
		return G_VOLUME_UNSUPPORTED;

	if(!request->bootSectors || request->bootSectors > disk->sectorCount)
		return G_VOLUME_INVALID;

	const g_disk_partition_info* selected = NULL;
	for(uint32_t i = 0; i < snapshot->disk.partitions; ++i)
	{
		const g_disk_partition_info* part = &snapshot->partitions[i];
		if(part->firstSector < request->bootSectors)
			return G_VOLUME_UNSUPPORTED;
		if(part->number == request->partition)
			selected = part;
	}

	if(!selected || selected->scheme != G_VOLUME_MBR || selected->container || selected->number < 1 ||
	   selected->number > G_DISK_MBR_PARTITION_COUNT || !selected->sectorCount)
		return G_VOLUME_UNSUPPORTED;

	uint32_t identity = diskLayoutReadLe32(snapshot->mbr + G_DISK_MBR_IDENTITY_OFFSET);
	if(!identity)
		identity = request->diskIdentity;
	if(!identity)
		return G_VOLUME_INVALID;

	*info = {};
	info->device = disk->device;
	info->volume = request->volume;
	info->claim = request->claim;
	info->generation = disk->generation;
	info->diskIdentity = identity;
	info->partition = selected->number;
	info->firstSector = selected->firstSector;
	info->sectorCount = selected->sectorCount;
	info->bootSectors = request->bootSectors;
	return G_VOLUME_OK;
}

static g_volume_status diskBootValidateWrite(const g_volume_disk_snapshot* snapshot, const g_disk_boot_info* request)
{
	g_disk_boot_info info = {};
	g_volume_status status = diskBootGetInfo(snapshot, request, &info);
	if(status != G_VOLUME_OK)
		return status;

	return request->diskIdentity == info.diskIdentity ? G_VOLUME_OK : G_VOLUME_CONFLICT;
}

g_volume_status diskBootWrite(const g_volume_disk_snapshot* snapshot, const g_disk_boot_info* boot, uint64_t firstSector,
							 const uint8_t* bytes, uint32_t size, const g_volume_storage_io* io)
{
	g_volume_status status = diskBootValidateWrite(snapshot, boot);
	if(status != G_VOLUME_OK)
		return status;

	if(!bytes || !size || size > G_DISK_BOOT_CHUNK_SIZE || size % G_VOLUME_SECTOR_SIZE ||
	   !firstSector || firstSector >= boot->bootSectors ||
	   size / G_VOLUME_SECTOR_SIZE > boot->bootSectors - firstSector || !io->maxSectors ||
	   io->maxSectors > G_VOLUME_MAX_TRANSFER_SECTORS)
		return G_VOLUME_INVALID;

	uint8_t actual[G_DISK_BOOT_CHUNK_SIZE];
	if(!diskIoRead(io, 0, actual, 1))
		return G_VOLUME_IO_ERROR;
	if(memcmp(actual, snapshot->mbr, G_VOLUME_SECTOR_SIZE))
		return G_VOLUME_CONFLICT;

	uint32_t sectors = size / G_VOLUME_SECTOR_SIZE;
	for(uint32_t done = 0; done < sectors;)
	{
		uint32_t count = io->maxSectors;
		if(count > sectors - done)
			count = sectors - done;
		if(!diskIoWrite(io, firstSector + done, bytes + done * G_VOLUME_SECTOR_SIZE, count))
			return G_VOLUME_IO_ERROR;

		done += count;
	}

	if(!diskIoFlush(io))
		return G_VOLUME_IO_ERROR;

	for(uint32_t done = 0; done < sectors;)
	{
		uint32_t count = io->maxSectors;
		if(count > sectors - done)
			count = sectors - done;
		if(!diskIoRead(io, firstSector + done, actual, count) ||
		   memcmp(actual, bytes + done * G_VOLUME_SECTOR_SIZE, count * G_VOLUME_SECTOR_SIZE))
			return G_VOLUME_IO_ERROR;

		done += count;
	}

	return G_VOLUME_OK;
}

g_volume_status diskBootCommit(const g_volume_disk_snapshot* snapshot, const g_disk_boot_info* boot,
							  const uint8_t* code, uint32_t flags, const g_volume_storage_io* io)
{
	g_volume_status status = diskBootValidateWrite(snapshot, boot);
	if(status != G_VOLUME_OK)
		return status;
	if(!code || (flags & ~G_DISK_BOOT_ACTIVATE))
		return G_VOLUME_INVALID;

	uint8_t mbr[G_VOLUME_SECTOR_SIZE];
	memcpy(mbr, snapshot->mbr, sizeof(mbr));
	memcpy(mbr, code, G_DISK_BOOT_CODE_SIZE);
	diskLayoutWriteLe32(mbr + G_DISK_MBR_IDENTITY_OFFSET, boot->diskIdentity);

	if(flags & G_DISK_BOOT_ACTIVATE)
		for(unsigned i = 0; i < G_DISK_MBR_PARTITION_COUNT; ++i)
			diskLayoutGetMutableMbrEntry(mbr, i)[0] = i + 1 == boot->partition ? 0x80 : 0;

	return diskPartitionWriteTable(snapshot->mbr, mbr, io);
}
