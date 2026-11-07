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

#include "storage.hpp"
#include <string.h>

g_volume_status diskPartitionWriteTable(const uint8_t* expected, const uint8_t* replacement, const g_volume_storage_io* io)
{
	uint8_t current[G_VOLUME_SECTOR_SIZE];
	if(!diskIoRead(io, 0, current, 1))
		return G_VOLUME_IO_ERROR;
	if(memcmp(current, expected, sizeof(current)))
		return G_VOLUME_CONFLICT;

	diskIoReportProgress(io, G_VOLUME_JOB_EDITING, 0, 1);
	if(!diskIoWrite(io, 0, replacement, 1) || !diskIoFlush(io) || !diskIoRead(io, 0, current, 1) ||
	   memcmp(current, replacement, sizeof(current)))
		return G_VOLUME_IO_ERROR;
	diskIoReportProgress(io, G_VOLUME_JOB_EDITING, 1, 1);
	return G_VOLUME_OK;
}

g_volume_status diskPartitionCreateTable(const g_volume_disk_snapshot* snapshot, const g_volume_storage_io* io, bool replace)
{
	if(snapshot->disk.sectorSize != G_VOLUME_SECTOR_SIZE || !snapshot->disk.sectorCount)
		return G_VOLUME_UNSUPPORTED;
	if(!io->maxSectors || io->maxSectors > G_VOLUME_MAX_TRANSFER_SECTORS)
		return G_VOLUME_INVALID;

	uint64_t leading = snapshot->disk.sectorCount;
	if(leading > 2048)
		leading = 2048;
	uint64_t trailing = snapshot->disk.sectorCount - leading;
	if(trailing < leading)
		trailing = leading;
	uint8_t mbr[G_VOLUME_SECTOR_SIZE] = {};
	mbr[510] = 0x55;
	mbr[511] = 0xaa;
	if(replace)
	{
		uint8_t empty[G_VOLUME_SECTOR_SIZE] = {};
		g_volume_status status = diskPartitionWriteTable(snapshot->mbr, empty, io);
		if(status != G_VOLUME_OK)
			return status;
		status = diskWipeRange(&snapshot->disk, 0, leading, io);
		if(status != G_VOLUME_OK)
			return status;
		if(trailing < snapshot->disk.sectorCount)
		{
			status = diskWipeRange(&snapshot->disk, trailing, snapshot->disk.sectorCount - trailing, io);
			if(status != G_VOLUME_OK)
				return status;
		}
		return diskPartitionWriteTable(empty, mbr, io);
	}
	if(snapshot->disk.layout == G_DISK_MBR)
		return snapshot->disk.partitions ? G_VOLUME_CONFLICT : G_VOLUME_OK;
	if(snapshot->disk.layout != G_DISK_NONE || snapshot->disk.wholeFilesystem)
		return G_VOLUME_CONFLICT;

	uint8_t bytes[G_VOLUME_SECTOR_SIZE * G_VOLUME_MAX_TRANSFER_SECTORS];
	uint64_t starts[2] = {0, trailing};
	uint64_t counts[2] = {leading, snapshot->disk.sectorCount - trailing};
	uint64_t total = counts[0] + counts[1];
	uint64_t done = 0;
	for(unsigned range = 0; range < 2; ++range)
	{
		for(uint64_t offset = 0; offset < counts[range];)
		{
			uint32_t count = io->maxSectors;
			if(count > counts[range] - offset)
				count = (uint32_t) (counts[range] - offset);
			if(!diskIoRead(io, starts[range] + offset, bytes, count))
				return G_VOLUME_IO_ERROR;
			for(uint32_t i = 0; i < count * G_VOLUME_SECTOR_SIZE; ++i)
				if(bytes[i])
					return G_VOLUME_CONFLICT;
			offset += count;
			done += count;
			diskIoReportProgress(io, G_VOLUME_JOB_CHECKING, done, total);
		}
	}
	return diskPartitionWriteTable(snapshot->mbr, mbr, io);
}

static bool diskPartitionAlignSector(uint64_t value, uint64_t alignment, uint64_t* result)
{
	if(!alignment)
		return false;
	uint64_t remainder = value % alignment;
	uint64_t extra = remainder ? alignment - remainder : 0;
	if(value > UINT64_MAX - extra)
		return false;
	*result = value + extra;
	return true;
}

g_volume_status diskPartitionCreate(const g_volume_disk_snapshot* snapshot, const g_disk_partition_options* options,
								   const g_volume_storage_io* io, uint32_t* created)
{
	if(snapshot->disk.layout != G_DISK_MBR || snapshot->disk.extended)
		return G_VOLUME_UNSUPPORTED;
	bool explicitStart = options->flags & G_DISK_PARTITION_HAS_START;
	bool remaining = options->flags & G_DISK_PARTITION_REMAINING;
	if(options->slot > G_DISK_MBR_PARTITION_COUNT || !options->type || options->type > 255 ||
	   diskLayoutIsExtendedMbrType(options->type) || options->type == 0xee || !options->alignment ||
	   (options->flags & ~(G_DISK_PARTITION_HAS_START | G_DISK_PARTITION_REMAINING)) ||
	   (!remaining && !options->sectorCount))
		return G_VOLUME_INVALID;

	uint32_t slot = options->slot;
	if(!slot)
		for(uint32_t i = 0; i < G_DISK_MBR_PARTITION_COUNT; ++i)
			if(!diskLayoutGetMbrEntry(snapshot->mbr, i)[4])
			{
				slot = i + 1;
				break;
			}
	if(!slot || diskLayoutGetMbrEntry(snapshot->mbr, slot - 1)[4])
		return G_VOLUME_CONFLICT;

	uint64_t limit = snapshot->disk.sectorCount;
	if(limit > (uint64_t) UINT32_MAX + 1)
		limit = (uint64_t) UINT32_MAX + 1;
	uint64_t first;
	if(!diskPartitionAlignSector(explicitStart ? options->firstSector : 1, options->alignment, &first) || !first ||
	   (explicitStart && first != options->firstSector))
		return G_VOLUME_INVALID;

	uint64_t available = 0;
	for(;;)
	{
		const g_disk_partition_info* next = NULL;
		for(uint32_t i = 0; i < snapshot->disk.partitions; ++i)
		{
			const g_disk_partition_info* part = &snapshot->partitions[i];
			if(part->firstSector + part->sectorCount <= first)
				continue;
			if(!next || part->firstSector < next->firstSector)
				next = part;
		}
		uint64_t end = limit;
		if(next && next->firstSector < end)
			end = next->firstSector;
		available = first < end ? end - first : 0;
		if(available && (remaining || options->sectorCount <= available))
			break;
		if(!next || explicitStart)
			return G_VOLUME_CONFLICT;
		if(!diskPartitionAlignSector(next->firstSector + next->sectorCount, options->alignment, &first))
			return G_VOLUME_INVALID;
	}
	uint64_t count = remaining ? available : options->sectorCount;
	if(!count || first > UINT32_MAX || count > UINT32_MAX || count > available)
		return G_VOLUME_INVALID;

	uint8_t mbr[G_VOLUME_SECTOR_SIZE];
	memcpy(mbr, snapshot->mbr, sizeof(mbr));
	uint8_t* entry = diskLayoutGetMutableMbrEntry(mbr, slot - 1);
	memset(entry, 0, G_DISK_MBR_PARTITION_ENTRY_SIZE);
	entry[1] = entry[5] = 0xfe;
	entry[2] = entry[3] = entry[6] = entry[7] = 0xff;
	entry[4] = options->type;
	diskLayoutWriteLe32(entry + 8, first);
	diskLayoutWriteLe32(entry + 12, count);
	g_volume_status status = diskPartitionWriteTable(snapshot->mbr, mbr, io);
	if(status == G_VOLUME_OK)
		*created = slot;
	return status;
}

g_volume_status diskPartitionDelete(const g_volume_disk_snapshot* snapshot, uint32_t number, const g_volume_storage_io* io)
{
	if(snapshot->disk.layout != G_DISK_MBR || snapshot->disk.extended)
		return G_VOLUME_UNSUPPORTED;
	if(number < 1 || number > G_DISK_MBR_PARTITION_COUNT)
		return G_VOLUME_INVALID;
	if(!diskLayoutGetMbrEntry(snapshot->mbr, number - 1)[4])
		return G_VOLUME_NOT_FOUND;

	uint8_t mbr[G_VOLUME_SECTOR_SIZE];
	memcpy(mbr, snapshot->mbr, sizeof(mbr));
	memset(diskLayoutGetMutableMbrEntry(mbr, number - 1), 0, G_DISK_MBR_PARTITION_ENTRY_SIZE);
	return diskPartitionWriteTable(snapshot->mbr, mbr, io);
}
