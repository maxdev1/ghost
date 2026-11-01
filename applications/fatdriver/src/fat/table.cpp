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

static const uint8_t* fatTableSector(g_fat_mount* mount, uint32_t sector)
{
	auto current = &mount->sectors[mount->tableCacheIndex];
	if(current->stamp && current->sector == sector)
	{
		current->stamp = ++mount->cacheClock;
		return current->bytes;
	}
	for(auto& cached : mount->sectors)
	{
		if(cached.stamp && cached.sector == sector)
		{
			cached.stamp = ++mount->cacheClock;
			mount->tableCacheIndex = &cached - mount->sectors;
			return cached.bytes;
		}
	}

	auto& volume = mount->volume;
	auto first = volume.reservedSectors + uint64_t(volume.activeFat) * volume.sectorsPerFat;
	if(sector < first || sector >= first + volume.sectorsPerFat) return nullptr;
	uint8_t bytes[G_BLOCK_DMA_BUFFER_BYTES];
	auto count = std::min(uint64_t(sizeof(bytes) / G_BLOCK_SECTOR_SIZE), first + volume.sectorsPerFat - sector);
	count = std::min(count, uint64_t(mount->device.info.maxTransferSectors));
	count = std::min(count, uint64_t(mount->device.buffer.size / G_BLOCK_SECTOR_SIZE));
	if(!count || !fatVolumeRead(&mount->device, volume, uint64_t(sector) * G_BLOCK_SECTOR_SIZE, bytes, count * G_BLOCK_SECTOR_SIZE)) return nullptr;
	for(uint32_t i = 0; i < count; ++i)
	{
		auto oldest = &mount->sectors[0];
		for(auto& cached : mount->sectors)
		{
			if(cached.stamp && cached.sector == sector + i)
			{
				oldest = &cached;
				break;
			}
			if(cached.stamp < oldest->stamp) oldest = &cached;
		}
		oldest->sector = sector + i;
		oldest->stamp = ++mount->cacheClock;
		memcpy(oldest->bytes, bytes + i * G_BLOCK_SECTOR_SIZE, G_BLOCK_SECTOR_SIZE);
		if(!i) mount->tableCacheIndex = oldest - mount->sectors;
	}
	return mount->sectors[mount->tableCacheIndex].bytes;
}

static bool fatTableRead(g_fat_mount* mount, uint64_t offset, uint8_t* target, size_t length)
{
	auto& volume = mount->volume;
	auto tableBytes = uint64_t(volume.sectorsPerFat) * G_BLOCK_SECTOR_SIZE;
	if(offset > tableBytes || length > tableBytes - offset)
		return false;

	offset += (volume.reservedSectors + uint64_t(volume.activeFat) * volume.sectorsPerFat) * G_BLOCK_SECTOR_SIZE;
	while(length)
	{
		auto bytes = fatTableSector(mount, offset / G_BLOCK_SECTOR_SIZE);
		if(!bytes)
			return false;

		auto skip = offset % G_BLOCK_SECTOR_SIZE;
		auto count = G_BLOCK_SECTOR_SIZE - skip;
		if(count > length)
			count = length;

		memcpy(target, bytes + skip, count);
		target += count;
		offset += count;
		length -= count;
	}
	return true;
}

bool fatTableGet(g_fat_mount* mount, uint32_t cluster, uint32_t* value)
{
	auto bits = mount->volume.fatBits;
	auto offset = bits == 12 ? uint64_t(cluster) + cluster / 2 : uint64_t(cluster) * (bits / 8);
	uint8_t raw[4]{};
	if(!fatTableRead(mount, offset, raw, bits == 32 ? 4 : 2))
		return false;
	*value = bits == 32 ? fatRead32(raw) & 0x0fffffff : fatRead16(raw);
	if(bits == 12)
		*value = cluster & 1 ? *value >> 4 : *value & 0x0fff;
	return true;
}

bool fatTableSet(g_fat_mount* mount, uint32_t cluster, uint32_t value, bool preserveChains)
{
	auto& volume = mount->volume;
	auto free = mount->freeClusters;
	uint32_t previous = 0;
	mount->freeClusters = UINT32_MAX;
	bool counted = cluster >= 2 && cluster < volume.clusterCount + 2;
	if(free != UINT32_MAX && counted && !fatTableGet(mount, cluster, &previous))
	{
		mount->failed = true;
		return false;
	}
	auto bits = volume.fatBits;
	auto entryOffset = bits == 12 ? uint64_t(cluster) + cluster / 2 : uint64_t(cluster) * (bits / 8);
	unsigned first = volume.mirrored ? 0 : volume.activeFat;
	unsigned end = volume.mirrored ? volume.fatCount : first + 1;
	for(unsigned copy = first; copy < end; ++copy)
	{
		auto offset = (volume.reservedSectors + uint64_t(copy) * volume.sectorsPerFat) * G_BLOCK_SECTOR_SIZE + entryOffset;
		uint8_t raw[4];
		auto length = bits == 32 ? 4 : 2;
		if(!fatVolumeRead(&mount->device, volume, offset, raw, length))
		{
			mount->failed = true;
			return false;
		}
		if(bits == 12)
		{
			auto previous = fatRead16(raw);
			fatWrite16(raw, cluster & 1 ? (previous & 0x000f) | ((value & 0x0fff) << 4) :
				(previous & 0xf000) | (value & 0x0fff));
		}
		else if(bits == 16)
			fatWrite16(raw, value);
		else
			fatWrite32(raw, (fatRead32(raw) & 0xf0000000) | (value & 0x0fffffff));
		if(!fatVolumeWrite(mount, offset, raw, length))
			return false;
	}
	if(!preserveChains)
		for(auto& chain : mount->chains) chain.stamp = 0;
	if(free != UINT32_MAX)
	{
		auto mask = bits == 12 ? 0x0fffU : bits == 16 ? 0xffffU : 0x0fffffffU;
		if(counted && !previous && (value & mask)) --free;
		if(counted && previous && !(value & mask)) ++free;
		mount->freeClusters = free;
	}
	return true;
}
