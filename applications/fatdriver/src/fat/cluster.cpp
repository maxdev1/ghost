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

bool fatClusterValid(const g_fat_volume& volume, uint32_t cluster)
{
	uint32_t reserved = G_FAT32_CLUSTER_RESERVED_MIN;
	if(volume.fatBits == G_FAT12_ENTRY_BITS)
		reserved = G_FAT12_CLUSTER_RESERVED_MIN;
	else if(volume.fatBits == G_FAT16_ENTRY_BITS)
		reserved = G_FAT16_CLUSTER_RESERVED_MIN;
	return cluster >= G_FAT_FIRST_DATA_CLUSTER && cluster <= volume.clusterCount + 1 && cluster < reserved;
}

uint64_t fatClusterOffset(const g_fat_volume& volume, uint32_t cluster)
{
	return (uint64_t(volume.dataSector) + uint64_t(cluster - G_FAT_FIRST_DATA_CLUSTER) * volume.sectorsPerCluster) * G_BLOCK_SECTOR_SIZE;
}

g_fat_result fatClusterNext(g_fat_mount* mount, uint32_t cluster, uint32_t* next)
{
	if(!fatClusterValid(mount->volume, cluster))
		return G_FAT_ERROR;

	uint32_t value;
	if(!fatTableGet(mount, cluster, &value))
		return G_FAT_ERROR;

	auto bits = mount->volume.fatBits;
	uint32_t end = G_FAT32_CLUSTER_END_MIN;
	if(bits == G_FAT12_ENTRY_BITS)
		end = G_FAT12_CLUSTER_END_MIN;
	else if(bits == G_FAT16_ENTRY_BITS)
		end = G_FAT16_CLUSTER_END_MIN;
	if(value >= end)
		return G_FAT_END;
	if(!fatClusterValid(mount->volume, value))
		return G_FAT_ERROR;

	*next = value;
	return G_FAT_SUCCESS;
}

/**
 * Validates a complete allocation chain before exposing file bytes. Brent's
 * cycle detector bounds corrupt chains without allocating a volume-sized bitmap.
 * Cached cursors keep sequential reads from walking again from the first cluster.
 */
g_fat_chain_cache* fatChainGet(g_fat_mount* mount, uint32_t first)
{
	if(!fatClusterValid(mount->volume, first))
		return nullptr;

	auto oldest = &mount->chains[0];
	for(auto& cached : mount->chains)
	{
		if(cached.stamp && cached.first == first)
		{
			cached.stamp = ++mount->cacheClock;
			return &cached;
		}

		if(cached.stamp < oldest->stamp)
			oldest = &cached;
	}

	uint32_t current = first, anchor = first, count = 1;
	uint64_t power = 1, distance = 0;
	for(;;)
	{
		uint32_t next;
		auto result = fatClusterNext(mount, current, &next);
		if(result == G_FAT_END)
			break;
		if(result != G_FAT_SUCCESS)
			return nullptr;

		++count;
		if(count > mount->volume.clusterCount || next == anchor)
			return nullptr;

		current = next;
		++distance;
		if(distance == power)
		{
			anchor = current;
			power *= 2;
			distance = 0;
		}
	}

	*oldest = {++mount->cacheClock, first, count, first, 0, current};
	return oldest;
}

/**
 * Remembers a chain assembled from a validated prefix and newly allocated
 * clusters. Existing sequential cursors remain valid when that chain grows.
 */
g_fat_chain_cache* fatChainRemember(g_fat_mount* mount, uint32_t first, uint32_t count, uint32_t last)
{
	if(!count || count > mount->volume.clusterCount || !fatClusterValid(mount->volume, first) ||
	   !fatClusterValid(mount->volume, last))
		return nullptr;

	auto oldest = &mount->chains[0];
	for(auto& cached : mount->chains)
	{
		if(cached.stamp && cached.first == first)
		{
			cached.count = count;
			cached.last = last;
			cached.stamp = ++mount->cacheClock;
			if(cached.index >= count)
			{
				cached.current = first;
				cached.index = 0;
			}

			return &cached;
		}

		if(cached.stamp < oldest->stamp)
			oldest = &cached;
	}

	*oldest = {++mount->cacheClock, first, count, first, 0, last};
	return oldest;
}

bool fatChainSeek(g_fat_mount* mount, g_fat_chain_cache* chain, uint32_t index, uint32_t* cluster)
{
	if(index >= chain->count)
		return false;
	if(index < chain->index)
	{
		chain->current = chain->first;
		chain->index = 0;
	}

	while(chain->index < index)
	{
		uint32_t next;
		if(fatClusterNext(mount, chain->current, &next) != G_FAT_SUCCESS)
			return false;

		chain->current = next;
		++chain->index;
	}

	*cluster = chain->current;
	return true;
}

bool fatCollectChain(g_fat_mount* mount, uint32_t first, std::vector<uint32_t>* chain)
{
	chain->clear();
	if(!first)
		return true;

	auto cached = fatChainGet(mount, first);
	if(!cached)
		return false;

	chain->reserve(cached->count);
	for(uint32_t i = 0; i < cached->count; ++i)
	{
		chain->push_back(first);
		if(i + 1 < cached->count)
		{
			if(fatClusterNext(mount, first, &first) != G_FAT_SUCCESS)
				return false;
		}
	}

	return true;
}

bool fatChainGetRun(g_fat_mount* mount, g_fat_chain_cache* chain, uint64_t offset, uint64_t limit,
	uint32_t* cluster, uint64_t* count)
{
	auto clusterBytes = uint64_t(mount->volume.sectorsPerCluster) * G_BLOCK_SECTOR_SIZE;
	auto index = offset / clusterBytes;
	if(!fatChainSeek(mount, chain, index, cluster))
		return false;

	*count = std::min(limit, clusterBytes - offset % clusterBytes);
	auto previous = *cluster;
	while(*count < limit)
	{
		uint32_t next;
		++index;
		if(!fatChainSeek(mount, chain, index, &next))
			return false;
		if(next != previous + 1)
			break;

		*count += std::min(limit - *count, clusterBytes);
		previous = next;
	}

	return true;
}
