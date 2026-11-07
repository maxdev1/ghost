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
#include "filesystem/mutation.hpp"

static const uint8_t* fatTableSector(g_fat_mount* mount, uint32_t sector)
{
	auto current = &mount->sectors[mount->tableCacheIndex];
	if(current->stamp && current->sector == sector)
	{
		current->stamp = ++mount->cacheClock;
		return current->bytes;
	}

	for(auto& cached: mount->sectors)
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
	if(sector < first || sector >= first + volume.sectorsPerFat)
		return nullptr;

	uint8_t bytes[G_BLOCK_DMA_BUFFER_BYTES];
	auto count = std::min(uint64_t(sizeof(bytes) / G_BLOCK_SECTOR_SIZE), first + volume.sectorsPerFat - sector);
	count = std::min(count, uint64_t(mount->device.info.maxTransferSectors));
	count = std::min(count, uint64_t(mount->device.buffer.size / G_BLOCK_SECTOR_SIZE));
	if(!count)
		return nullptr;

	if(!fatVolumeRead(&mount->device, volume, uint64_t(sector) * G_BLOCK_SECTOR_SIZE, bytes,
					  count * G_BLOCK_SECTOR_SIZE))
		return nullptr;

	for(uint32_t i = 0; i < count; ++i)
	{
		auto oldest = &mount->sectors[0];
		for(auto& cached: mount->sectors)
		{
			if(cached.stamp && cached.sector == sector + i)
			{
				oldest = &cached;
				break;
			}

			if(cached.stamp < oldest->stamp)
				oldest = &cached;
		}

		oldest->sector = sector + i;
		oldest->stamp = ++mount->cacheClock;
		memcpy(oldest->bytes, bytes + i * G_BLOCK_SECTOR_SIZE, G_BLOCK_SECTOR_SIZE);
		if(!i)
			mount->tableCacheIndex = oldest - mount->sectors;
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
	auto offset = bits == G_FAT12_ENTRY_BITS ? uint64_t(cluster) + cluster / G_FAT12_ENTRIES_PER_PAIR :
				  uint64_t(cluster) * (bits / G_FAT_BITS_PER_BYTE);
	uint8_t raw[G_FAT32_ENTRY_SIZE]{};
	if(!fatTableRead(mount, offset, raw, bits == G_FAT32_ENTRY_BITS ? G_FAT32_ENTRY_SIZE : G_FAT16_ENTRY_SIZE))
		return false;

	*value = bits == G_FAT32_ENTRY_BITS ? fatRead32(raw) & G_FAT32_ENTRY_MASK : fatRead16(raw);
	if(bits == G_FAT12_ENTRY_BITS)
		*value = cluster & 1 ? *value >> G_FAT_NIBBLE_BITS : *value & G_FAT12_ENTRY_MASK;
	return true;
}

bool fatTableSet(g_fat_mount* mount, uint32_t cluster, uint32_t value, bool preserveChains)
{
	auto& volume = mount->volume;
	auto free = mount->freeClusters;
	uint32_t previous = 0;
	mount->freeClusters = G_FAT_ALLOCATION_HINT_UNKNOWN;
	bool counted = fatClusterValid(volume, cluster);
	if(free != G_FAT_ALLOCATION_HINT_UNKNOWN && counted)
	{
		if(!fatTableGet(mount, cluster, &previous))
		{
			mount->failed = true;
			return false;
		}
	}

	auto bits = volume.fatBits;
	auto entryOffset = bits == G_FAT12_ENTRY_BITS ? uint64_t(cluster) + cluster / G_FAT12_ENTRIES_PER_PAIR :
					   uint64_t(cluster) * (bits / G_FAT_BITS_PER_BYTE);
	unsigned first = volume.mirrored ? 0 : volume.activeFat;
	unsigned end = volume.mirrored ? volume.fatCount : first + 1;
	for(unsigned copy = first; copy < end; ++copy)
	{
		auto offset =
				(volume.reservedSectors + uint64_t(copy) * volume.sectorsPerFat) * G_BLOCK_SECTOR_SIZE + entryOffset;
		uint8_t raw[G_FAT32_ENTRY_SIZE];
		auto length = bits == G_FAT32_ENTRY_BITS ? G_FAT32_ENTRY_SIZE : G_FAT16_ENTRY_SIZE;
		if(!fatVolumeRead(&mount->device, volume, offset, raw, length))
		{
			mount->failed = true;
			return false;
		}

		if(bits == G_FAT12_ENTRY_BITS)
		{
			auto previous = fatRead16(raw);
			uint16_t packed;
			if(cluster & 1)
				packed = (previous & G_FAT12_LOW_NIBBLE_MASK) | ((value & G_FAT12_ENTRY_MASK) << G_FAT_NIBBLE_BITS);
			else
				packed = (previous & G_FAT12_HIGH_NIBBLE_MASK) | (value & G_FAT12_ENTRY_MASK);
			fatWrite16(raw, packed);
		}
		else if(bits == G_FAT16_ENTRY_BITS)
			fatWrite16(raw, value);
		else
			fatWrite32(raw, (fatRead32(raw) & G_FAT32_RESERVED_BITS_MASK) | (value & G_FAT32_ENTRY_MASK));
		if(!fatVolumeWrite(mount, offset, raw, length))
			return false;
	}

	if(!preserveChains)
		for(auto& chain: mount->chains)
			chain.stamp = 0;
	if(free != G_FAT_ALLOCATION_HINT_UNKNOWN)
	{
		uint32_t mask = G_FAT32_ENTRY_MASK;
		if(bits == G_FAT12_ENTRY_BITS)
			mask = G_FAT12_ENTRY_MASK;
		else if(bits == G_FAT16_ENTRY_BITS)
			mask = G_FAT16_ENTRY_MASK;
		if(counted && !previous && (value & mask))
			--free;
		if(counted && previous && !(value & mask))
			++free;
		mount->freeClusters = free;
	}

	return true;
}
