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
#include "mutation.hpp"

g_fs_mutation_status fatMutationAllowed(g_fat_mount* mount, const g_fat_inode* inode)
{
	if(!fatVolumeWritable(mount) || (inode && (inode->attributes & G_FAT_ATTRIBUTE_READ_ONLY)))
		return G_FS_MUTATION_READ_ONLY;

	return G_FS_MUTATION_SUCCESSFUL;
}

/**
 * Writes bytes within an allocated chain. A null source fills the range with
 * zeros, including newly exposed bytes inside an existing final cluster.
 */
static bool fatWriteAllocatedRange(g_fat_mount* mount, uint32_t first, uint64_t offset, const uint8_t* source,
								   size_t length)
{
	auto cached = fatChainGet(mount, first);
	if(!cached)
		return false;

	uint8_t zero[G_BLOCK_DMA_BUFFER_BYTES]{};
	auto clusterBytes = uint64_t(mount->volume.sectorsPerCluster) * G_BLOCK_SECTOR_SIZE;
	while(length)
	{
		uint32_t cluster;
		uint64_t count;
		auto skip = offset % clusterBytes;
		auto limit = source ? uint64_t(length) : std::min(uint64_t(length), uint64_t(sizeof(zero)));
		if(!fatChainGetRun(mount, cached, offset, limit, &cluster, &count))
			return false;
		if(!fatVolumeWrite(mount, fatClusterOffset(mount->volume, cluster) + skip, source ? source : zero, count))
			return false;
		if(source)
			source += count;
		offset += count;
		length -= count;
	}

	return true;
}

/**
 * Growing files initialize and link their new clusters before publishing size.
 * Shrinking files publish the smaller size before detaching and freeing clusters,
 * so a failure cannot leave a live directory entry pointing into freed storage.
 */
g_fs_mutation_status fatFileResize(g_fat_mount* mount, g_fs_phys_id id, uint64_t length)
{
	g_fat_node node;
	if(!fatNodeRead(mount, id, &node))
		return G_FS_MUTATION_NOT_FOUND;

	auto inode = fatInodeFind(mount, id);
	auto permission = fatMutationAllowed(mount, inode);
	if(permission != G_FS_MUTATION_SUCCESSFUL)
		return permission;
	if(node.directory || length > G_FAT_FILE_SIZE_MAX)
		return G_FS_MUTATION_INVALID_ARGUMENT;
	if(node.size == length && (length || !node.firstCluster))
		return G_FS_MUTATION_SUCCESSFUL;

	std::vector<uint32_t> chain, allocated;
	uint32_t oldCount = 0;
	uint32_t oldLast = 0;
	if(node.firstCluster)
	{
		auto cached = fatChainGet(mount, node.firstCluster);
		if(!cached)
			return G_FS_MUTATION_ERROR;

		oldCount = cached->count;
		oldLast = cached->last;
	}

	auto clusterBytes = uint64_t(mount->volume.sectorsPerCluster) * G_BLOCK_SECTOR_SIZE;
	if(oldCount < (uint64_t(node.size) + clusterBytes - 1) / clusterBytes)
		return G_FS_MUTATION_ERROR;

	auto needed = (length + clusterBytes - 1) / clusterBytes;
	if(needed < oldCount)
	{
		if(!fatCollectChain(mount, node.firstCluster, &chain))
			return G_FS_MUTATION_ERROR;
	}

	if(needed > oldCount)
	{
		auto status = fatAllocateClusters(mount, needed - oldCount, &allocated);
		if(status != G_FS_MUTATION_SUCCESSFUL)
			return status;
		if(oldCount)
		{
			if(!fatTableSet(mount, oldLast, allocated.front(), true))
				return G_FS_MUTATION_ERROR;
			if(!fatVolumeFlush(mount))
				return G_FS_MUTATION_ERROR;
		}
		else
		{
			node.firstCluster = allocated.front();
		}

		if(!fatChainRemember(mount, node.firstCluster, needed, allocated.back()))
			return G_FS_MUTATION_ERROR;
	}

	if(length > node.size)
	{
		auto oldCapacity = uint64_t(oldCount) * clusterBytes;
		auto zeroEnd = std::min(length, oldCapacity);
		if(zeroEnd > node.size)
		{
			if(!fatWriteAllocatedRange(mount, node.firstCluster, node.size, nullptr, zeroEnd - node.size))
				return G_FS_MUTATION_ERROR;
		}
	}

	if(!length)
		node.firstCluster = 0;
	node.size = length;
	inode->node = node;
	if(!fatInodeStore(mount, inode))
		return G_FS_MUTATION_ERROR;

	if(needed < oldCount)
	{
		if(needed)
		{
			if(!fatTableSet(mount, chain[needed - 1], G_FAT_CLUSTER_END))
				return G_FS_MUTATION_ERROR;
			if(!fatVolumeFlush(mount))
				return G_FS_MUTATION_ERROR;
		}

		if(!fatFreeClusters(mount, chain, needed))
			return G_FS_MUTATION_ERROR;
		if(needed)
		{
			if(!fatChainRemember(mount, node.firstCluster, needed, chain[needed - 1]))
				return G_FS_MUTATION_ERROR;
		}

		inode->valid = true;
	}

	return G_FS_MUTATION_SUCCESSFUL;
}

g_fs_write_status fatFileWrite(g_fat_mount* mount, g_fs_phys_id id, uint64_t offset, const void* source, size_t length,
							   size_t* written)
{
	*written = 0;
	g_fat_node node;
	if(!fatNodeRead(mount, id, &node))
		return G_FS_WRITE_INVALID_FD;
	if(fatMutationAllowed(mount, fatInodeFind(mount, id)) != G_FS_MUTATION_SUCCESSFUL)
		return G_FS_WRITE_NOT_SUPPORTED;
	if(node.directory || offset > G_FAT_FILE_SIZE_MAX || length > G_FAT_FILE_SIZE_MAX - offset)
		return G_FS_WRITE_ERROR;
	if(!length)
		return G_FS_WRITE_SUCCESSFUL;

	if(node.size)
	{
		auto chain = fatChainGet(mount, node.firstCluster);
		auto clusterBytes = uint64_t(mount->volume.sectorsPerCluster) * G_BLOCK_SECTOR_SIZE;
		if(!chain || chain->count < (uint64_t(node.size) + clusterBytes - 1) / clusterBytes)
			return G_FS_WRITE_ERROR;
	}

	if(offset + length > node.size)
	{
		auto status = fatFileResize(mount, id, offset + length);
		if(status != G_FS_MUTATION_SUCCESSFUL)
			return status == G_FS_MUTATION_NO_SPACE ? G_FS_WRITE_NO_SPACE : G_FS_WRITE_ERROR;
		if(!fatNodeRead(mount, id, &node))
			return G_FS_WRITE_ERROR;
	}

	if(!fatWriteAllocatedRange(mount, node.firstCluster, offset, static_cast<const uint8_t*>(source), length))
		return G_FS_WRITE_ERROR;

	auto inode = fatInodeFind(mount, id);
	if(!(inode->attributes & G_FAT_ATTRIBUTE_ARCHIVE))
	{
		if(!fatInodeStore(mount, inode))
			return G_FS_WRITE_ERROR;
	}

	*written = length;
	return G_FS_WRITE_SUCCESSFUL;
}
