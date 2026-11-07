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

	if(!fatDeviceReadBuffer(device, sector, 1))
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
	if(fatRead16(sector + G_FAT_BOOT_SIGNATURE_OFFSET) != G_FAT_BOOT_SIGNATURE ||
		!((sector[G_FAT_BOOT_JUMP_OFFSET] == G_FAT_BOOT_SHORT_JUMP &&
		   sector[G_FAT_BOOT_NOP_OFFSET] == G_FAT_BOOT_NOP) ||
		  sector[G_FAT_BOOT_JUMP_OFFSET] == G_FAT_BOOT_NEAR_JUMP) ||
		fatRead16(sector + G_FAT_BPB_BYTES_PER_SECTOR_OFFSET) != G_BLOCK_SECTOR_SIZE)
		return false;

	g_fat_volume parsed;
	parsed.firstSector = first;
	parsed.sectorsPerCluster = sector[G_FAT_BPB_SECTORS_PER_CLUSTER_OFFSET];
	parsed.reservedSectors = fatRead16(sector + G_FAT_BPB_RESERVED_SECTORS_OFFSET);
	parsed.fatCount = sector[G_FAT_BPB_TABLE_COUNT_OFFSET];
	parsed.rootEntries = fatRead16(sector + G_FAT_BPB_ROOT_ENTRIES_OFFSET);
	parsed.sectorCount = fatRead16(sector + G_FAT_BPB_SECTOR_COUNT16_OFFSET);
	if(!parsed.sectorCount)
		parsed.sectorCount = fatRead32(sector + G_FAT_BPB_SECTOR_COUNT32_OFFSET);
	auto fat16 = fatRead16(sector + G_FAT_BPB_SECTORS_PER_FAT16_OFFSET);
	parsed.sectorsPerFat = fat16 ? fat16 : fatRead32(sector + G_FAT_BPB_SECTORS_PER_FAT32_OFFSET);
	if(!parsed.sectorsPerCluster || parsed.sectorsPerCluster > G_FAT_MAX_SECTORS_PER_CLUSTER ||
		(parsed.sectorsPerCluster & (parsed.sectorsPerCluster - 1)) || !parsed.reservedSectors ||
		!parsed.fatCount || parsed.fatCount > G_FAT_MAX_TABLE_COUNT || !parsed.sectorsPerFat ||
		!parsed.sectorCount || parsed.sectorCount > available ||
		first > UINT64_MAX - parsed.sectorCount)
		return false;

	auto rootSectors = (uint64_t(parsed.rootEntries) * G_FAT_DIRECTORY_ENTRY_SIZE + G_BLOCK_SECTOR_SIZE - 1) /
					   G_BLOCK_SECTOR_SIZE;
	auto metadata = parsed.reservedSectors + uint64_t(parsed.fatCount) * parsed.sectorsPerFat + rootSectors;
	if(metadata >= parsed.sectorCount)
		return false;

	parsed.clusterCount = (parsed.sectorCount - metadata) / parsed.sectorsPerCluster;
	if(!parsed.clusterCount)
		return false;
	if(parsed.clusterCount < G_FAT16_MIN_DATA_CLUSTERS)
		parsed.fatBits = G_FAT12_ENTRY_BITS;
	else if(parsed.clusterCount < G_FAT32_MIN_DATA_CLUSTERS)
		parsed.fatBits = G_FAT16_ENTRY_BITS;
	else
		parsed.fatBits = G_FAT32_ENTRY_BITS;
	if(parsed.fatBits == G_FAT32_ENTRY_BITS)
	{
		auto flags = fatRead16(sector + G_FAT_BPB_FLAGS_OFFSET);
		parsed.activeFat = flags & G_FAT_BPB_NO_MIRRORING ? flags & G_FAT_BPB_ACTIVE_TABLE_MASK : 0;
		parsed.mirrored = !(flags & G_FAT_BPB_NO_MIRRORING);
		parsed.fsInfoSector = fatRead16(sector + G_FAT_BPB_FS_INFO_OFFSET);
		parsed.backupBootSector = fatRead16(sector + G_FAT_BPB_BACKUP_BOOT_OFFSET);
		parsed.rootCluster = fatRead32(sector + G_FAT_BPB_ROOT_CLUSTER_OFFSET);
		if(fat16 || parsed.rootEntries || fatRead16(sector + G_FAT_BPB_VERSION_OFFSET) ||
			parsed.clusterCount > G_FAT32_MAX_DATA_CLUSTERS || parsed.rootCluster < G_FAT_FIRST_DATA_CLUSTER ||
			parsed.rootCluster > parsed.clusterCount + 1 || parsed.activeFat >= parsed.fatCount)
			return false;
	}
	else if(!fat16 || !parsed.rootEntries || uint32_t(parsed.rootEntries) * G_FAT_DIRECTORY_ENTRY_SIZE % G_BLOCK_SECTOR_SIZE)
		return false;

	auto entries = uint64_t(parsed.clusterCount) + G_FAT_RESERVED_ENTRY_COUNT;
	auto requiredFatBytes = (entries * parsed.fatBits + G_FAT_BITS_PER_BYTE - 1) / G_FAT_BITS_PER_BYTE;
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
	if(!fatDeviceReadSector(device, device->firstSector, boot))
		return false;

	return fatVolumeParse(boot, device->firstSector, available, volume);
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
	if(mount->freeClusters == G_FAT_ALLOCATION_HINT_UNKNOWN)
	{
		uint32_t free = 0;
		auto reported = g_millis();
		if(progress)
			progress(context, 0, mount->volume.clusterCount);
		for(uint32_t done = 0; done < mount->volume.clusterCount;)
		{
			uint32_t entry;
			if(!fatTableGet(mount, done + G_FAT_FIRST_DATA_CLUSTER, &entry))
				return false;
			if(!entry && fatClusterValid(mount->volume, done + G_FAT_FIRST_DATA_CLUSTER))
				++free;
			++done;
			if(!(done % G_FAT_STAT_CHECK_CLUSTERS))
			{
				if(!(done % G_FAT_SCAN_YIELD_CLUSTERS))
					g_sleep(G_FAT_SCAN_YIELD_MS);
				if(progress && g_millis() - reported >= G_FAT_PROGRESS_INTERVAL_MS)
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
