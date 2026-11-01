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

#include "cache.hpp"
#include <algorithm>
#include <cstring>

void fatCacheInvalidate(g_fat_mount* mount)
{
	mount->freeClusters = UINT32_MAX;
	for(auto& sector : mount->sectors) sector.stamp = 0;
	for(auto& sector : mount->metadata) sector.stamp = 0;
	for(auto& chain : mount->chains) chain.stamp = 0;
	for(auto inode = mount->inodes; inode; inode = inode->next) inode->valid = false;
	mount->allocationHintsInvalid = false;
}

template<size_t Count>
static bool fatCacheRead(const g_fat_sector_cache (&cache)[Count],
	uint64_t sector, uint8_t* bytes)
{
	for(const auto& cached : cache)
		if(cached.stamp && cached.sector == sector)
		{
			memcpy(bytes, cached.bytes, G_BLOCK_SECTOR_SIZE);
			return true;
		}
	return false;
}

template<size_t Count>
static void fatCacheUpdateEntries(g_fat_sector_cache (&cache)[Count], uint64_t offset, const uint8_t* bytes, size_t length)
{
	for(auto& cached : cache)
	{
		if(!cached.stamp) continue;
		auto start = std::max(offset, uint64_t(cached.sector) * G_BLOCK_SECTOR_SIZE);
		auto end = std::min(offset + length, (uint64_t(cached.sector) + 1) * G_BLOCK_SECTOR_SIZE);
		if(start < end) memcpy(cached.bytes + start % G_BLOCK_SECTOR_SIZE, bytes + start - offset, end - start);
	}
}

bool fatCacheReadSector(g_fat_mount* mount, uint64_t sector, uint8_t* bytes)
{
	return fatCacheRead(mount->sectors, sector, bytes) || fatCacheRead(mount->metadata, sector, bytes);
}

void fatCacheUpdate(g_fat_mount* mount, uint64_t offset, const uint8_t* bytes, size_t length)
{
	fatCacheUpdateEntries(mount->sectors, offset, bytes, length);
	fatCacheUpdateEntries(mount->metadata, offset, bytes, length);
}

/**
 * Caches directory sectors independently of file payload and allocation chains.
 * Successful filesystem writes update overlapping cache bytes before returning.
 */
bool fatMetadataRead(g_fat_mount* mount, uint64_t offset, void* target, size_t length)
{
	auto capacity = uint64_t(mount->volume.sectorCount) * G_BLOCK_SECTOR_SIZE;
	if(offset > capacity || length > capacity - offset) return false;
	auto bytes = static_cast<uint8_t*>(target);
	while(length)
	{
		auto sector = offset / G_BLOCK_SECTOR_SIZE;
		auto cached = &mount->metadata[0];
		for(auto& entry : mount->metadata)
		{
			if(entry.stamp && entry.sector == sector)
			{
				cached = &entry;
				break;
			}
			if(entry.stamp < cached->stamp) cached = &entry;
		}
		if(!cached->stamp || cached->sector != sector)
		{
			cached->stamp = 0;
			if(!fatVolumeRead(&mount->device, mount->volume, sector * G_BLOCK_SECTOR_SIZE,
				cached->bytes, sizeof(cached->bytes))) return false;
			cached->sector = sector;
		}
		cached->stamp = ++mount->cacheClock;
		auto skip = offset % G_BLOCK_SECTOR_SIZE;
		auto count = G_BLOCK_SECTOR_SIZE - skip;
		if(count > length) count = length;
		memcpy(bytes, cached->bytes + skip, count);
		offset += count;
		bytes += count;
		length -= count;
	}
	return true;
}
