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

#include <new>
#include "mutation.hpp"

g_fat_inode* fatInodeFind(g_fat_mount* mount, g_fs_phys_id id)
{
	for(auto inode = mount->inodes; inode; inode = inode->next)
		if(inode->node.id == id)
			return inode;

	return nullptr;
}

/**
 * Mount-local identities outlive directory slots and are never reused. An
 * unlinked record remains available until its last open descriptor closes.
 */
bool fatInodeObserve(g_fat_mount* mount, g_fat_node* node, g_fs_phys_id parent, uint64_t offset,
					 const uint64_t* longOffsets, uint8_t longCount, uint8_t attributes)
{
	g_fat_inode* inode = mount->inodes;
	while(inode && (inode->unlinked || inode->offset != offset))
		inode = inode->next;
	if(!inode)
	{
		if(mount->inodeCount >= G_FAT_MAX_INODES)
			return false;

		inode = new(std::nothrow) g_fat_inode;
		if(!inode)
			return false;

		inode->node.id = mount->nextInode++;
		inode->offset = offset;
		inode->next = mount->inodes;
		mount->inodes = inode;
		++mount->inodeCount;
	}

	node->id = inode->node.id;
	inode->node = *node;
	inode->parent = parent;
	inode->attributes = attributes;
	inode->valid = true;
	inode->slots[0] = offset;
	inode->slotCount = longCount + 1;
	for(unsigned i = 0; i < longCount; ++i)
		inode->slots[i + 1] = longOffsets[i];
	return true;
}

void fatInodeRelease(g_fat_mount* mount, g_fat_inode* inode)
{
	for(auto link = &mount->inodes; *link; link = &(*link)->next)
	{
		if(*link != inode)
			continue;

		*link = inode->next;
		delete inode;
		--mount->inodeCount;
		return;
	}
}

void fatMountRelease(g_fat_mount* mount)
{
	while(mount->inodes)
	{
		auto inode = mount->inodes;
		mount->inodes = inode->next;
		delete inode;
	}

	mount->inodeCount = 0;
}

bool fatNodeDecode(const g_fat_volume& volume, uint64_t offset, const uint8_t* raw, g_fat_node* node)
{
	if(!raw[0] || raw[0] == G_FAT_ENTRY_DELETED || (raw[G_FAT_ENTRY_ATTRIBUTES_OFFSET] & G_FAT_ATTRIBUTE_VOLUME))
		return false;

	g_fat_node decoded;
	decoded.id = offset + 1;
	decoded.directory = raw[G_FAT_ENTRY_ATTRIBUTES_OFFSET] & G_FAT_ATTRIBUTE_DIRECTORY;
	decoded.size = decoded.directory ? 0 : fatRead32(raw + G_FAT_ENTRY_FILE_SIZE_OFFSET);
	decoded.firstCluster = fatRead16(raw + G_FAT_ENTRY_CLUSTER_LOW_OFFSET);
	if(volume.fatBits == G_FAT32_ENTRY_BITS)
		decoded.firstCluster |=
				uint32_t(fatRead16(raw + G_FAT_ENTRY_CLUSTER_HIGH_OFFSET) & G_FAT_ENTRY_CLUSTER_HIGH_MASK) <<
				G_FAT_ENTRY_CLUSTER_HIGH_SHIFT;

	if(!fatClusterValid(volume, decoded.firstCluster) && (decoded.firstCluster || decoded.directory || decoded.size))
		return false;

	*node = decoded;
	return true;
}

bool fatNodeRead(g_fat_mount* mount, g_fs_phys_id id, g_fat_node* node)
{
	if(id == G_FAT_ROOT)
	{
		*node = {G_FAT_ROOT, mount->volume.rootCluster, 0, true};
		return true;
	}

	auto inode = fatInodeFind(mount, id);
	if(!inode || (inode->unlinked && (!inode->opens || inode->node.directory)))
		return false;
	if(inode->unlinked || inode->valid)
	{
		*node = inode->node;
		return true;
	}

	uint8_t raw[G_FAT_DIRECTORY_ENTRY_SIZE];
	if(!fatMetadataRead(mount, inode->offset, raw, sizeof(raw)))
		return false;
	if(!fatNodeDecode(mount->volume, inode->offset, raw, node))
		return false;

	node->id = id;
	inode->node = *node;
	inode->attributes = raw[G_FAT_ENTRY_ATTRIBUTES_OFFSET];
	inode->valid = true;
	return true;
}

bool fatInodeStore(g_fat_mount* mount, g_fat_inode* inode)
{
	if(inode->unlinked)
		return fatVolumeFlush(mount);

	uint8_t raw[G_FAT_DIRECTORY_ENTRY_SIZE];
	if(!fatMetadataRead(mount, inode->offset, raw, sizeof(raw)))
	{
		mount->failed = true;
		return false;
	}

	fatWrite16(raw + G_FAT_ENTRY_CLUSTER_HIGH_OFFSET,
			   mount->volume.fatBits == G_FAT32_ENTRY_BITS ? inode->node.firstCluster >> G_FAT_ENTRY_CLUSTER_HIGH_SHIFT : 0);
	fatWrite16(raw + G_FAT_ENTRY_CLUSTER_LOW_OFFSET, inode->node.firstCluster);
	fatWrite32(raw + G_FAT_ENTRY_FILE_SIZE_OFFSET, inode->node.size);
	if(!fatRead16(raw + G_FAT_ENTRY_MODIFIED_DATE_OFFSET))
		fatWrite16(raw + G_FAT_ENTRY_MODIFIED_DATE_OFFSET, G_FAT_DATE_MINIMUM);
	raw[G_FAT_ENTRY_ATTRIBUTES_OFFSET] |= G_FAT_ATTRIBUTE_ARCHIVE;
	if(!fatVolumeWrite(mount, inode->offset, raw, sizeof(raw)))
		return false;
	if(!fatVolumeFlush(mount))
		return false;

	inode->attributes = raw[G_FAT_ENTRY_ATTRIBUTES_OFFSET];
	inode->valid = true;
	return true;
}

g_fs_mutation_status fatInodeClose(g_fat_mount* mount, g_fs_phys_id id)
{
	auto inode = fatInodeFind(mount, id);
	if(!inode || !inode->opens)
		return G_FS_MUTATION_SUCCESSFUL;

	--inode->opens;
	if(inode->unlinked && !inode->opens)
	{
		std::vector<uint32_t> chain;
		if(!fatCollectChain(mount, inode->node.firstCluster, &chain))
			return G_FS_MUTATION_ERROR;
		if(!fatFreeClusters(mount, chain))
			return G_FS_MUTATION_ERROR;

		fatInodeRelease(mount, inode);
	}

	return G_FS_MUTATION_SUCCESSFUL;
}
