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

#include "disk.hpp"

void diskPrint(const g_disk_info& disk)
{
	printf("Disk: %u\nModel: %s\nSerial: %s\nDriver task: %d\nSectors: %llu\nSector size: %u\n"
		   "Table: %s\nPartitions: %u\nGeneration: %llu\nActive job: %llu\n",
		   unsigned(disk.device), disk.model, disk.serial, int(disk.driver), (unsigned long long) disk.sectorCount,
		   disk.sectorSize, storageLayoutText(disk.layout), disk.partitions, (unsigned long long) disk.generation,
		   (unsigned long long) disk.activeJob);
}

int diskList()
{
	g_device_id after = 0;
	printf("ID\tMODEL\tSECTORS\tTABLE\tPARTITIONS\tJOB\n");

	for(;;)
	{
		g_disk_info disk;
		auto status = diskNext(after, &disk);
		if(status == G_VOLUME_NOT_FOUND)
			return 0;
		if(status != G_VOLUME_OK)
			return storageError("disk", status);

		printf("%u\t%s\t%llu\t%s\t%u\t%llu\n", unsigned(disk.device), disk.model, (unsigned long long) disk.sectorCount,
			   storageLayoutText(disk.layout), disk.partitions, (unsigned long long) disk.activeJob);
		after = disk.device;
	}
}

void diskPrintPartition(const g_disk_partition_info& part)
{
	printf("%u\t%llu\t%llu\t%llu\t", part.number, (unsigned long long) part.volume,
		   (unsigned long long) part.firstSector, (unsigned long long) part.sectorCount);
	if(part.scheme == G_VOLUME_GPT)
		for(unsigned i = 0; i < 16; ++i)
			printf("%02x", unsigned(part.typeGuid[i]));
	else if(part.type)
		printf("0x%02x", part.type);
	else
		printf("unknown");
	printf("\t%s%s\n", part.container ? "container" : "", part.bootable ? " active" : "");
}

int diskPartitionList(const g_disk_info& disk)
{
	std::vector<g_disk_partition_info> parts;
	uint32_t after = 0;
	printf("NUMBER\tVOLUME\tFIRST SECTOR\tSECTORS\tTYPE\tFLAGS\n");

	for(;;)
	{
		g_disk_partition_info part;
		auto status = diskPartitionNext(disk.device, after, &part);
		if(status == G_VOLUME_NOT_FOUND)
			break;
		if(status != G_VOLUME_OK)
			return storageError("disk", status);

		diskPrintPartition(part);
		parts.push_back(part);
		after = part.number;
	}

	if(disk.layout != G_DISK_MBR && disk.layout != G_DISK_GPT)
		return 0;

	std::sort(parts.begin(), parts.end(), [](const g_disk_partition_info& a, const g_disk_partition_info& b)
			  { return a.firstSector < b.firstSector; });
	uint64_t cursor = disk.firstUsable;
	for(const auto& part: parts)
	{
		if(part.firstSector > cursor)
			printf("Free: sector %llu, %llu sectors\n", (unsigned long long) cursor,
				   (unsigned long long) (part.firstSector - cursor));
		cursor = std::max(cursor, part.firstSector + part.sectorCount);
	}

	if(cursor <= disk.lastUsable)
		printf("Free: sector %llu, %llu sectors\n", (unsigned long long) cursor,
			   (unsigned long long) (disk.lastUsable - cursor + 1));
	return 0;
}
