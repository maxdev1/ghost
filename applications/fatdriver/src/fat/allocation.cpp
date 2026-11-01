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
#include <algorithm>
#include <cstring>

static bool fatInvalidateAllocationHints(g_fat_mount* mount)
{
	if(mount->allocationHintsInvalid) return true;
	auto& volume = mount->volume;
	if(volume.fatBits != 32 || !volume.fsInfoSector || volume.fsInfoSector >= volume.reservedSectors)
		return true;
	uint32_t sectors[] = {volume.fsInfoSector, uint32_t(volume.backupBootSector) + volume.fsInfoSector};
	for(auto sector : sectors)
	{
		if(sector >= volume.reservedSectors)
			continue;
		uint8_t raw[G_BLOCK_SECTOR_SIZE];
		auto offset = uint64_t(sector) * G_BLOCK_SECTOR_SIZE;
		if(!fatVolumeRead(&mount->device, volume, offset, raw, sizeof(raw)))
		{
			mount->failed = true;
			return false;
		}
		if(fatRead32(raw) != 0x41615252 || fatRead32(raw + 484) != 0x61417272 || fatRead32(raw + 508) != 0xaa550000)
			continue;
		if(fatRead32(raw + 488) != UINT32_MAX || fatRead32(raw + 492) != UINT32_MAX)
		{
			memset(raw + 488, 0xff, 8);
			if(!fatVolumeWrite(mount, offset, raw, sizeof(raw))) return false;
		}
	}
	mount->allocationHintsInvalid = true;
	return true;
}

/**
 * Publishes a new FAT32 chain a sector at a time. No inode references these
 * clusters until all copies are flushed; failures freeze the mount.
 */
static bool fatAllocateFat32Chain(g_fat_mount* mount, const std::vector<uint32_t>& chain)
{
	const auto& volume = mount->volume;
	constexpr uint32_t entriesPerSector = G_BLOCK_SECTOR_SIZE / sizeof(uint32_t);
	unsigned first = volume.mirrored ? 0 : volume.activeFat;
	unsigned end = volume.mirrored ? volume.fatCount : first + 1;
	for(unsigned copy = first; copy < end; ++copy)
		for(size_t index = 0; index < chain.size();)
		{
			auto sector = chain[index] / entriesPerSector;
			auto offset = uint64_t(volume.reservedSectors + copy * volume.sectorsPerFat + sector) * G_BLOCK_SECTOR_SIZE;
			uint8_t bytes[G_BLOCK_SECTOR_SIZE];
			if(!fatVolumeRead(&mount->device, volume, offset, bytes, sizeof(bytes)))
			{
				mount->failed = true;
				return false;
			}
			do
			{
				auto slot = bytes + (chain[index] % entriesPerSector) * 4;
				auto next = index + 1 < chain.size() ? chain[index + 1] : 0x0fffffff;
				fatWrite32(slot, (fatRead32(slot) & 0xf0000000) | next);
				++index;
			} while(index < chain.size() && chain[index] / entriesPerSector == sector);
			if(!fatVolumeWrite(mount, offset, bytes, sizeof(bytes))) return false;
		}
	return true;
}

static bool fatAllocationZeroClusters(g_fat_mount* mount, const std::vector<uint32_t>& allocated)
{
	uint8_t zero[G_BLOCK_DMA_BUFFER_BYTES]{};
	auto clusterBytes = uint64_t(mount->volume.sectorsPerCluster) * G_BLOCK_SECTOR_SIZE;
	for(size_t index = 0; index < allocated.size();)
	{
		auto first = index++;
		while(index < allocated.size() && allocated[index] == allocated[index - 1] + 1) ++index;
		auto start = fatClusterOffset(mount->volume, allocated[first]);
		auto length = uint64_t(index - first) * clusterBytes;
		for(uint64_t offset = 0; offset < length; offset += sizeof(zero))
			if(!fatVolumeWrite(mount, start + offset, zero, std::min(uint64_t(sizeof(zero)), length - offset)))
				return false;
	}
	return true;
}

static bool fatAllocationLinkClusters(g_fat_mount* mount, const std::vector<uint32_t>& allocated)
{
	if(mount->volume.fatBits == 32)
	{
		if(!fatAllocateFat32Chain(mount, allocated)) return false;
	}
	else
	{
		for(auto cluster : allocated)
			if(!fatTableSet(mount, cluster, 0x0fffffff, true)) return false;
		for(size_t i = allocated.size(); i > 1; --i)
			if(!fatTableSet(mount, allocated[i - 2], allocated[i - 1], true)) return false;
	}

	return true;
}

/**
 * Finds all required free clusters before changing disk contents. New clusters
 * are zeroed, linked and flushed before a file or directory can reference them.
 */
g_fs_mutation_status fatAllocateClusters(g_fat_mount* mount, uint32_t count, std::vector<uint32_t>* allocated)
{
	allocated->clear();
	if(!count)
		return G_FS_MUTATION_SUCCESSFUL;
	if(count > mount->volume.clusterCount)
		return G_FS_MUTATION_NO_SPACE;
	allocated->reserve(count);

	uint32_t current = mount->nextFreeCluster;
	for(uint32_t examined = 0; examined < mount->volume.clusterCount && allocated->size() < count; ++examined)
	{
		if(current > mount->volume.clusterCount + 1) current = 2;
		uint32_t value;
		if(!fatTableGet(mount, current, &value))
			return G_FS_MUTATION_ERROR;
		if(!value && fatClusterValid(mount->volume, current))
			allocated->push_back(current);
		++current;
	}
	if(allocated->size() != count)
		return G_FS_MUTATION_NO_SPACE;

	if(!fatAllocationZeroClusters(mount, *allocated) || !fatAllocationLinkClusters(mount, *allocated))
		return G_FS_MUTATION_ERROR;

	mount->nextFreeCluster = current;
	return fatInvalidateAllocationHints(mount) && fatVolumeFlush(mount) ? G_FS_MUTATION_SUCCESSFUL : G_FS_MUTATION_ERROR;
}

bool fatFreeClusters(g_fat_mount* mount, const std::vector<uint32_t>& chain, size_t first)
{
	for(size_t i = first; i < chain.size(); ++i)
	{
		if(!fatTableSet(mount, chain[i], 0))
			return false;
		mount->nextFreeCluster = std::min(mount->nextFreeCluster, chain[i]);
	}
	return fatInvalidateAllocationHints(mount) && fatVolumeFlush(mount);
}
