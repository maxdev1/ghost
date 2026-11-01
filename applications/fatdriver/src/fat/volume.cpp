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

#include "filesystem/mutation.hpp"
#include <ghost/tasks.h>
#include <ghost/system.h>
#include <cstdio>
#include <cstring>

static bool fatDeviceReadSector(g_fat_device* device, uint64_t sector, uint8_t* target)
{
	if(sector >= device->info.sectorCount)
		return false;
	device->status = blockDeviceReadBuffer(&device->buffer, device->id, sector, 1);
	if(device->status != G_BLOCK_SUCCESS)
		return false;
	memcpy(target, device->buffer.data, G_BLOCK_SECTOR_SIZE);
	return true;
}

/**
 * Determines FAT type from the data-cluster count, never the descriptive label.
 * Rejects inconsistent BPBs and FATs too small to address their declared data.
 */
bool fatVolumeParse(const uint8_t* sector, uint64_t first, uint64_t available, g_fat_volume* volume)
{
	if(sector[510] != 0x55 || sector[511] != 0xaa ||
		!((sector[0] == 0xeb && sector[2] == 0x90) || sector[0] == 0xe9) ||
		fatRead16(sector + 11) != G_BLOCK_SECTOR_SIZE)
		return false;
	g_fat_volume parsed;
	parsed.firstSector = first;
	parsed.sectorsPerCluster = sector[13];
	parsed.reservedSectors = fatRead16(sector + 14);
	parsed.fatCount = sector[16];
	parsed.rootEntries = fatRead16(sector + 17);
	parsed.sectorCount = fatRead16(sector + 19);
	if(!parsed.sectorCount)
		parsed.sectorCount = fatRead32(sector + 32);
	auto fat16 = fatRead16(sector + 22);
	parsed.sectorsPerFat = fat16 ? fat16 : fatRead32(sector + 36);
	if(!parsed.sectorsPerCluster || parsed.sectorsPerCluster > 128 ||
		(parsed.sectorsPerCluster & (parsed.sectorsPerCluster - 1)) || !parsed.reservedSectors ||
		!parsed.fatCount || parsed.fatCount > 2 || !parsed.sectorsPerFat ||
		!parsed.sectorCount || parsed.sectorCount > available ||
		first > UINT64_MAX - parsed.sectorCount)
		return false;
	auto rootSectors = (uint64_t(parsed.rootEntries) * 32 + G_BLOCK_SECTOR_SIZE - 1) / G_BLOCK_SECTOR_SIZE;
	auto metadata = parsed.reservedSectors + uint64_t(parsed.fatCount) * parsed.sectorsPerFat + rootSectors;
	if(metadata >= parsed.sectorCount)
		return false;
	parsed.clusterCount = (parsed.sectorCount - metadata) / parsed.sectorsPerCluster;
	if(!parsed.clusterCount)
		return false;
	parsed.fatBits = parsed.clusterCount < 4085 ? 12 : parsed.clusterCount < 65525 ? 16 : 32;
	if(parsed.fatBits == 32)
	{
		auto flags = fatRead16(sector + 40);
		parsed.activeFat = flags & 0x80 ? flags & 0x0f : 0;
		parsed.mirrored = !(flags & 0x80);
		parsed.fsInfoSector = fatRead16(sector + 48);
		parsed.backupBootSector = fatRead16(sector + 50);
		parsed.rootCluster = fatRead32(sector + 44);
		if(fat16 || parsed.rootEntries || fatRead16(sector + 42) ||
			parsed.clusterCount > 0x0ffffff3 || parsed.rootCluster < 2 ||
			parsed.rootCluster > parsed.clusterCount + 1 || parsed.activeFat >= parsed.fatCount)
			return false;
	}
	else if(!fat16 || !parsed.rootEntries || uint32_t(parsed.rootEntries) * 32 % G_BLOCK_SECTOR_SIZE)
		return false;
	auto entries = uint64_t(parsed.clusterCount) + 2;
	auto requiredFatBytes = (entries * parsed.fatBits + 7) / 8;
	if(requiredFatBytes > uint64_t(parsed.sectorsPerFat) * G_BLOCK_SECTOR_SIZE)
		return false;

	parsed.rootSector = parsed.reservedSectors + uint64_t(parsed.fatCount) * parsed.sectorsPerFat;
	parsed.rootSectors = rootSectors;
	parsed.dataSector = metadata;
	*volume = parsed;
	return true;
}

bool fatVolumeProbe(g_fat_device* device, g_fat_volume* volume)
{
	auto available = device->sectorCount ? device->sectorCount : device->info.sectorCount;
	if(device->firstSector >= device->info.sectorCount || available > device->info.sectorCount - device->firstSector)
		return false;
	uint8_t boot[G_BLOCK_SECTOR_SIZE];
	return fatDeviceReadSector(device, device->firstSector, boot) &&
		fatVolumeParse(boot, device->firstSector, available, volume);
}

/**
 * Counts through the common FAT reader and maintains the exact result across
 * allocation changes until caches are invalidated. Serialized serving requests
 * prevent concurrent mutations; progressing scans periodically renew the client.
 */
bool fatVolumeStat(g_fat_mount* mount, g_volume_filesystem_info* info,
	g_volume_progress_callback progress, void* context)
{
	*info = {};
	snprintf(info->type, sizeof(info->type), "fat%u", mount->volume.fatBits);
	info->allocationUnit = uint32_t(mount->volume.sectorsPerCluster) * G_BLOCK_SECTOR_SIZE;
	info->totalBytes = uint64_t(mount->volume.clusterCount) * info->allocationUnit;
	info->writable = fatVolumeWritable(mount);
	if(mount->freeClusters == UINT32_MAX)
	{
		uint32_t free = 0;
		auto reported = g_millis();
		if(progress) progress(context, 0, mount->volume.clusterCount);
		for(uint32_t done = 0; done < mount->volume.clusterCount;)
		{
			uint32_t entry;
			if(!fatTableGet(mount, done + 2, &entry)) return false;
			if(!entry) ++free;
			++done;
			if(!(done % 128))
			{
				if(!(done % 4096)) g_sleep(1);
				if(progress && g_millis() - reported >= 100)
				{
					progress(context, done, mount->volume.clusterCount);
					reported = g_millis();
				}
			}
		}
		mount->freeClusters = free;
	}
	info->freeBytes = uint64_t(mount->freeClusters) * info->allocationUnit;
	return true;
}
