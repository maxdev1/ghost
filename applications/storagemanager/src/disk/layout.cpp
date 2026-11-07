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
#include "layout.hpp"
#include "storage.hpp"

static uint32_t diskLayoutUpdateCrc32(uint32_t crc, const uint8_t* bytes, size_t count)
{
	for(size_t i = 0; i < count; ++i)
	{
		crc ^= bytes[i];
		for(unsigned bit = 0; bit < 8; ++bit)
		{
			if(crc & 1)
				crc = (crc >> 1) ^ 0xedb88320u;
			else
				crc >>= 1;
		}
	}

	return crc;
}

static bool diskLayoutAddPartition(g_volume_disk_snapshot* snapshot, uint64_t first, uint64_t count, uint32_t number,
							 g_volume_scheme scheme)
{
	if(!first || !count || first >= snapshot->disk.sectorCount || count > snapshot->disk.sectorCount - first ||
	   snapshot->disk.partitions >= G_DISK_MAX_PARTITIONS)
		return false;
	for(uint32_t i = 0; i < snapshot->disk.partitions; ++i)
	{
		const g_disk_partition_info* part = &snapshot->partitions[i];
		if(!part->container && first < part->firstSector + part->sectorCount && part->firstSector < first + count)
			return false;
	}
	snapshot->partitions.push_back({});
	snapshot->disk.partitions = snapshot->partitions.size();
	g_disk_partition_info* part = &snapshot->partitions.back();
	part->device = snapshot->disk.device;
	part->number = number;
	part->firstSector = first;
	part->sectorCount = count;
	part->scheme = scheme;
	return true;
}

static g_volume_status diskLayoutReadGpt(g_volume_disk_snapshot* snapshot, const g_volume_storage_io* io)
{
	uint8_t header[G_VOLUME_SECTOR_SIZE];
	if(!diskIoRead(io, 1, header, 1))
		return G_VOLUME_IO_ERROR;

	uint32_t revision = diskLayoutReadLe32(header + 8);
	uint32_t headerSize = diskLayoutReadLe32(header + 12);
	uint32_t reserved = diskLayoutReadLe32(header + 20);
	uint64_t primarySector = diskLayoutReadLe64(header + 24);
	uint64_t backupSector = diskLayoutReadLe64(header + 32);
	if(memcmp(header, "EFI PART", 8) || revision != 0x10000 || headerSize < 92 || headerSize > G_VOLUME_SECTOR_SIZE ||
	   reserved || primarySector != 1 || backupSector != snapshot->disk.sectorCount - 1)
		return G_VOLUME_INVALID;

	uint32_t crc = diskLayoutReadLe32(header + 16);
	memset(header + 16, 0, 4);
	if(~diskLayoutUpdateCrc32(UINT32_MAX, header, headerSize) != crc)
		return G_VOLUME_INVALID;

	uint64_t firstUsable = diskLayoutReadLe64(header + 40);
	uint64_t lastUsable = diskLayoutReadLe64(header + 48);
	uint64_t table = diskLayoutReadLe64(header + 72);
	uint32_t entries = diskLayoutReadLe32(header + 80);
	uint32_t entrySize = diskLayoutReadLe32(header + 84);
	if(!entries || entries > G_DISK_GPT_MAX_PARTITIONS || entrySize != G_DISK_GPT_PARTITION_ENTRY_SIZE)
		return G_VOLUME_UNSUPPORTED;

	uint64_t tableSectors = ((uint64_t) entries * entrySize + G_VOLUME_SECTOR_SIZE - 1) / G_VOLUME_SECTOR_SIZE;
	if(table < 2 || firstUsable < table || tableSectors > firstUsable - table || firstUsable > lastUsable ||
	   lastUsable >= snapshot->disk.sectorCount - 1 || tableSectors > snapshot->disk.sectorCount - 2 - lastUsable)
		return G_VOLUME_INVALID;

	snapshot->disk.firstUsable = firstUsable;
	snapshot->disk.lastUsable = lastUsable;
	uint32_t tableCrc = UINT32_MAX;
	uint8_t sector[G_VOLUME_SECTOR_SIZE];
	for(uint32_t i = 0; i < entries; ++i)
	{
		if(i % G_DISK_GPT_ENTRIES_PER_SECTOR == 0)
		{
			uint64_t tableSector = table + i / G_DISK_GPT_ENTRIES_PER_SECTOR;
			if(!diskIoRead(io, tableSector, sector, 1))
				return G_VOLUME_IO_ERROR;
		}

		const uint8_t* entry = sector + (i % G_DISK_GPT_ENTRIES_PER_SECTOR) * G_DISK_GPT_PARTITION_ENTRY_SIZE;
		tableCrc = diskLayoutUpdateCrc32(tableCrc, entry, G_DISK_GPT_PARTITION_ENTRY_SIZE);
		bool empty = true;
		for(unsigned j = 0; j < 16; ++j)
		{
			if(entry[j])
			{
				empty = false;
				break;
			}
		}
		if(empty)
			continue;

		uint64_t first = diskLayoutReadLe64(entry + 32);
		uint64_t last = diskLayoutReadLe64(entry + 40);
		if(first < firstUsable || first > last || last > lastUsable ||
		   !diskLayoutAddPartition(snapshot, first, last - first + 1, i + 1, G_VOLUME_GPT))
			return G_VOLUME_INVALID;
		g_disk_partition_info* part = &snapshot->partitions[snapshot->disk.partitions - 1];
		memcpy(part->typeGuid, entry, sizeof(part->typeGuid));
	}

	return ~tableCrc == diskLayoutReadLe32(header + 88) ? G_VOLUME_OK : G_VOLUME_INVALID;
}

static g_volume_status diskLayoutReadMbr(g_volume_disk_snapshot* snapshot, const g_volume_storage_io* io)
{
	const uint8_t* mbr = snapshot->mbr;
	uint64_t extendedFirst = 0;
	uint64_t extendedCount = 0;
	uint32_t extendedNumber = 0;

	for(unsigned i = 0; i < G_DISK_MBR_PARTITION_COUNT; ++i)
	{
		const uint8_t* entry = diskLayoutGetMbrEntry(mbr, i);
		uint64_t first = diskLayoutReadLe32(entry + 8);
		uint32_t count = diskLayoutReadLe32(entry + 12);
		if(!entry[4] && !first && !count)
			continue;
		if((entry[0] != 0 && entry[0] != 0x80) || !entry[4] || entry[4] == 0xee || !first || !count ||
		   first >= snapshot->disk.sectorCount || count > snapshot->disk.sectorCount - first)
			return G_VOLUME_INVALID;

		if(diskLayoutIsExtendedMbrType(entry[4]))
		{
			if(extendedFirst)
				return G_VOLUME_INVALID;

			extendedFirst = first;
			extendedCount = count;
			extendedNumber = i + 1;
		}
		else
		{
			if(!diskLayoutAddPartition(snapshot, first, count, i + 1, G_VOLUME_MBR))
				return G_VOLUME_INVALID;
			g_disk_partition_info* part = &snapshot->partitions[snapshot->disk.partitions - 1];
			part->type = entry[4];
			part->bootable = entry[0] == 0x80;
		}
	}

	if(!extendedFirst)
		return G_VOLUME_OK;

	for(uint32_t i = 0; i < snapshot->disk.partitions; ++i)
	{
		const g_disk_partition_info* part = &snapshot->partitions[i];
		if(extendedFirst < part->firstSector + part->sectorCount && part->firstSector < extendedFirst + extendedCount)
			return G_VOLUME_INVALID;
	}
	snapshot->partitions.push_back({});
	snapshot->disk.partitions = snapshot->partitions.size();
	g_disk_partition_info* container = &snapshot->partitions.back();
	container->device = snapshot->disk.device;
	container->number = extendedNumber;
	container->type = diskLayoutGetMbrEntry(mbr, extendedNumber - 1)[4];
	container->firstSector = extendedFirst;
	container->sectorCount = extendedCount;
	container->scheme = G_VOLUME_MBR;
	container->container = 1;
	snapshot->disk.extended = 1;
	uint64_t visited[G_DISK_MBR_MAX_LOGICAL_PARTITIONS];
	uint64_t current = extendedFirst;
	for(unsigned logical = 0; logical < G_DISK_MBR_MAX_LOGICAL_PARTITIONS; ++logical)
	{
		unsigned number = G_DISK_MBR_PARTITION_COUNT + logical + 1;
		if(current < extendedFirst || current >= extendedFirst + extendedCount)
			return G_VOLUME_INVALID;
		for(unsigned i = 0; i < logical; ++i)
			if(visited[i] == current)
				return G_VOLUME_INVALID;
		visited[logical] = current;
		for(uint32_t i = 0; i < snapshot->disk.partitions; ++i)
		{
			const g_disk_partition_info* part = &snapshot->partitions[i];
			if(!part->container && current >= part->firstSector && current < part->firstSector + part->sectorCount)
				return G_VOLUME_INVALID;
		}

		uint8_t sector[G_VOLUME_SECTOR_SIZE];
		if(!diskIoRead(io, current, sector, 1))
			return G_VOLUME_IO_ERROR;
		if(!diskLayoutHasBootSignature(sector))
			return G_VOLUME_INVALID;

		const uint8_t* entry = diskLayoutGetMbrEntry(sector, 0);
		uint32_t offset = diskLayoutReadLe32(entry + 8);
		uint32_t count = diskLayoutReadLe32(entry + 12);
		if(!entry[4] || diskLayoutIsExtendedMbrType(entry[4]) || (entry[0] != 0 && entry[0] != 0x80) || !offset ||
		   offset >= extendedFirst + extendedCount - current ||
		   count > extendedFirst + extendedCount - current - offset ||
		   !diskLayoutAddPartition(snapshot, current + offset, count, number, G_VOLUME_MBR))
			return G_VOLUME_INVALID;

		g_disk_partition_info* part = &snapshot->partitions[snapshot->disk.partitions - 1];
		part->type = entry[4];
		part->bootable = entry[0] == 0x80;
		const uint8_t* link = entry + G_DISK_MBR_PARTITION_ENTRY_SIZE;
		for(unsigned i = 2; i < G_DISK_MBR_PARTITION_COUNT; ++i)
		{
			const uint8_t* extra = diskLayoutGetMbrEntry(sector, i);
			if(extra[4] || diskLayoutReadLe32(extra + 8) || diskLayoutReadLe32(extra + 12))
				return G_VOLUME_INVALID;
		}

		if(!link[4] && !diskLayoutReadLe32(link + 8) && !diskLayoutReadLe32(link + 12))
			return G_VOLUME_OK;

		if(!diskLayoutIsExtendedMbrType(link[4]) || (link[0] != 0 && link[0] != 0x80) || diskLayoutReadLe32(link + 8) >= extendedCount ||
		   !diskLayoutReadLe32(link + 12) || diskLayoutReadLe32(link + 12) > extendedCount - diskLayoutReadLe32(link + 8))
			return G_VOLUME_INVALID;

		current = extendedFirst + diskLayoutReadLe32(link + 8);
	}

	return G_VOLUME_UNSUPPORTED;
}

g_volume_status diskLayoutInspect(const g_disk_info* disk, const g_volume_storage_io* io, g_volume_disk_snapshot* snapshot)
{
	*snapshot = {};
	snapshot->disk = *disk;
	snapshot->disk.partitions = 0;
	snapshot->disk.extended = 0;
	snapshot->disk.wholeFilesystem = 0;
	snapshot->disk.firstUsable = 1;
	snapshot->disk.lastUsable = disk->sectorCount ? disk->sectorCount - 1 : 0;

	if(!disk->sectorCount || disk->sectorSize != G_VOLUME_SECTOR_SIZE)
	{
		snapshot->disk.layout = G_DISK_UNSUPPORTED;
		return G_VOLUME_UNSUPPORTED;
	}
	if(!diskIoRead(io, 0, snapshot->mbr, 1))
	{
		snapshot->disk.layout = G_DISK_INVALID;
		return G_VOLUME_IO_ERROR;
	}

	snapshot->disk.layout = G_DISK_NONE;
	snapshot->disk.wholeFilesystem = diskLayoutHasFatGeometry(snapshot->mbr);

	g_volume_status status = G_VOLUME_OK;
	if(diskLayoutHasBootSignature(snapshot->mbr) && !snapshot->disk.wholeFilesystem)
	{
		if(diskLayoutHasProtectiveMbrEntry(snapshot->mbr))
		{
			snapshot->disk.layout = G_DISK_GPT;
			status = diskLayoutReadGpt(snapshot, io);
		}
		else
		{
			snapshot->disk.layout = G_DISK_MBR;
			status = diskLayoutReadMbr(snapshot, io);
		}
	}

	if(status != G_VOLUME_OK)
	{
		snapshot->partitions.clear();
		snapshot->disk.partitions = 0;
		snapshot->disk.layout = status == G_VOLUME_UNSUPPORTED ? G_DISK_UNSUPPORTED : G_DISK_INVALID;
		return status;
	}

	for(uint32_t i = 1; i < snapshot->disk.partitions; ++i)
	{
		g_disk_partition_info part = snapshot->partitions[i];
		uint32_t j = i;
		while(j && snapshot->partitions[j - 1].number > part.number)
		{
			snapshot->partitions[j] = snapshot->partitions[j - 1];
			--j;
		}
		snapshot->partitions[j] = part;
	}
	return G_VOLUME_OK;
}
