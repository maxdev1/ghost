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
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include "mutation.hpp"

static bool fatAliasCharacter(unsigned char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c && strchr("$%'-_@~`!(){}^#&", c));
}

static char fatAliasUpper(char c)
{
	return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
}

static bool fatAliasAvailable(const std::vector<std::array<uint8_t, G_FAT_SHORT_NAME_SIZE>>& aliases,
							  const std::vector<std::string>& names, const uint8_t* candidate)
{
	for(auto& alias: aliases)
		if(!memcmp(alias.data(), candidate, G_FAT_SHORT_NAME_SIZE))
			return false;

	char name[G_FAT_SHORT_NAME_BUFFER_SIZE];
	if(!fatShortNameDecode(candidate, name))
		return false;

	for(auto& existing: names)
		if(fatNameEquals(name, existing.c_str()))
			return false;

	return true;
}

static bool fatChooseAlias(const char* name, const std::vector<std::array<uint8_t, G_FAT_SHORT_NAME_SIZE>>& aliases,
						   const std::vector<std::string>& names, uint8_t* raw)
{
	memset(raw, 0, G_FAT_DIRECTORY_ENTRY_SIZE);
	memset(raw, ' ', G_FAT_SHORT_NAME_SIZE);
	auto dot = strrchr(name, '.');
	auto baseLength = dot ? size_t(dot - name) : strlen(name);
	auto extensionLength = dot ? strlen(dot + 1) : 0;
	bool plain = baseLength && baseLength <= G_FAT_SHORT_BASE_SIZE && extensionLength <= G_FAT_SHORT_EXTENSION_SIZE;
	for(size_t i = 0; i < baseLength && plain; ++i)
	{
		auto c = fatAliasUpper(name[i]);
		plain = fatAliasCharacter(c);
		raw[i] = c;
	}

	for(size_t i = 0; i < extensionLength && plain; ++i)
	{
		auto c = fatAliasUpper(dot[i + 1]);
		plain = fatAliasCharacter(c);
		raw[G_FAT_SHORT_BASE_SIZE + i] = c;
	}

	if(plain && fatAliasAvailable(aliases, names, raw))
	{
		bool baseLower = false, baseUpper = false, extLower = false, extUpper = false;
		for(size_t i = 0; i < baseLength; ++i)
		{
			baseLower |= name[i] >= 'a' && name[i] <= 'z';
			baseUpper |= name[i] >= 'A' && name[i] <= 'Z';
		}

		for(size_t i = 0; i < extensionLength; ++i)
		{
			extLower |= dot[i + 1] >= 'a' && dot[i + 1] <= 'z';
			extUpper |= dot[i + 1] >= 'A' && dot[i + 1] <= 'Z';
		}

		if(baseLower && !baseUpper)
			raw[G_FAT_ENTRY_CASE_FLAGS_OFFSET] |= G_FAT_SHORT_BASE_LOWER_CASE;
		if(extLower && !extUpper)
			raw[G_FAT_ENTRY_CASE_FLAGS_OFFSET] |= G_FAT_SHORT_EXTENSION_LOWER_CASE;
		return true;
	}

	char prefix[G_FAT_SHORT_BASE_SIZE + 1]{};
	size_t prefixLength = 0;
	for(size_t i = 0; i < baseLength && prefixLength < G_FAT_SHORT_BASE_SIZE; ++i)
		if(fatAliasCharacter(fatAliasUpper(name[i])))
			prefix[prefixLength++] = fatAliasUpper(name[i]);
	if(!prefixLength)
		strcpy(prefix, "FILE");
	for(unsigned serial = 1; serial <= G_FAT_ALIAS_MAX_SERIAL; ++serial)
	{
		char tail[G_FAT_SHORT_BASE_SIZE];
		snprintf(tail, sizeof(tail), "~%u", serial);
		memset(raw, ' ', G_FAT_SHORT_NAME_SIZE);
		raw[G_FAT_ENTRY_CASE_FLAGS_OFFSET] = 0;
		auto count = std::min(strlen(prefix), size_t(G_FAT_SHORT_BASE_SIZE - strlen(tail)));
		memcpy(raw, prefix, count);
		memcpy(raw + count, tail, strlen(tail));
		for(size_t i = 0; i < extensionLength && i < G_FAT_SHORT_EXTENSION_SIZE; ++i)
			raw[G_FAT_SHORT_BASE_SIZE + i] = fatAliasCharacter(fatAliasUpper(dot[i + 1])) ? fatAliasUpper(dot[i + 1]) : '_';
		if(fatAliasAvailable(aliases, names, raw))
			return true;
	}

	return false;
}

static g_fs_mutation_status fatFindDirectorySlots(g_fat_mount* mount, const g_fat_node& parent, unsigned needed,
												  uint64_t* slots, uint64_t* next, bool* consumesEnd)
{
	for(unsigned attempt = 0; attempt < G_FAT_DIRECTORY_GROW_ATTEMPTS; ++attempt)
	{
		g_fat_directory directory;
		if(!fatDirectoryOpen(mount, parent, &directory))
			return G_FS_MUTATION_ERROR;

		unsigned count = 0;
		bool ended = false, runUsesEnd = false;
		for(;;)
		{
			uint8_t raw[G_FAT_DIRECTORY_ENTRY_SIZE];
			uint64_t offset;
			auto result = fatDirectoryRawNext(mount, &directory, &offset, raw);
			if(result == G_FAT_END)
				break;
			if(result != G_FAT_SUCCESS)
				return G_FS_MUTATION_ERROR;
			if(!raw[0])
				ended = true;
			if(ended || raw[0] == G_FAT_ENTRY_DELETED)
			{
				runUsesEnd |= ended;
				slots[count++] = offset;
				if(count == needed)
				{
					*consumesEnd = runUsesEnd;
					*next = UINT64_MAX;
					if(runUsesEnd)
					{
						auto following = fatDirectoryRawNext(mount, &directory, &offset, raw);
						if(following == G_FAT_ERROR)
							return G_FS_MUTATION_ERROR;
						if(following == G_FAT_SUCCESS)
							*next = offset;
					}

					return G_FS_MUTATION_SUCCESSFUL;
				}
			}
			else
			{
				count = 0;
				runUsesEnd = false;
			}
		}

		if(parent.id == G_FAT_ROOT && mount->volume.fatBits != G_FAT32_ENTRY_BITS)
			return G_FS_MUTATION_NO_SPACE;
		if(attempt)
			return G_FS_MUTATION_NO_SPACE;

		std::vector<uint32_t> chain, added;
		if(!fatCollectChain(mount, parent.firstCluster, &chain))
			return G_FS_MUTATION_ERROR;
		if(chain.empty())
			return G_FS_MUTATION_ERROR;

		auto entriesPerCluster = unsigned(mount->volume.sectorsPerCluster) * G_BLOCK_SECTOR_SIZE / G_FAT_DIRECTORY_ENTRY_SIZE;
		auto status = fatAllocateClusters(mount, (needed - count + entriesPerCluster - 1) / entriesPerCluster, &added);
		if(status != G_FS_MUTATION_SUCCESSFUL)
			return status;
		if(!fatTableSet(mount, chain.back(), added.front(), true))
			return G_FS_MUTATION_ERROR;
		if(!fatVolumeFlush(mount))
			return G_FS_MUTATION_ERROR;
		if(!fatChainRemember(mount, parent.firstCluster, chain.size() + added.size(), added.back()))
			return G_FS_MUTATION_ERROR;
	}

	return G_FS_MUTATION_NO_SPACE;
}

static void fatInitializeDirectoryEntry(uint8_t* raw, uint32_t cluster)
{
	raw[G_FAT_ENTRY_ATTRIBUTES_OFFSET] = G_FAT_ATTRIBUTE_DIRECTORY;
	fatWrite16(raw + G_FAT_ENTRY_CLUSTER_HIGH_OFFSET, cluster >> G_FAT_ENTRY_CLUSTER_HIGH_SHIFT);
	fatWrite16(raw + G_FAT_ENTRY_CLUSTER_LOW_OFFSET, cluster);
	fatWrite16(raw + G_FAT_ENTRY_CREATED_DATE_OFFSET, G_FAT_DATE_MINIMUM);
	fatWrite16(raw + G_FAT_ENTRY_ACCESSED_DATE_OFFSET, G_FAT_DATE_MINIMUM);
	fatWrite16(raw + G_FAT_ENTRY_MODIFIED_DATE_OFFSET, G_FAT_DATE_MINIMUM);
}

static g_fs_mutation_status fatDirectoryCollectNames(g_fat_mount* mount, const g_fat_node& parent, const char* name,
													 std::vector<std::array<uint8_t, G_FAT_SHORT_NAME_SIZE>>* aliases,
													 std::vector<std::string>* names, g_fs_phys_id replaced)
{
	g_fat_directory directory;
	if(!fatDirectoryOpen(mount, parent, &directory))
		return G_FS_MUTATION_ERROR;

	for(;;)
	{
		g_fat_directory_entry entry;
		auto result = fatDirectoryNext(mount, &directory, &entry);
		if(result == G_FAT_END)
			break;
		if(result != G_FAT_SUCCESS)
			return G_FS_MUTATION_ERROR;
		if((fatNameEquals(name, entry.name) || fatNameEquals(name, entry.alias)) && entry.node.id != replaced)
			return G_FS_MUTATION_ALREADY_EXISTS;

		std::array<uint8_t, G_FAT_SHORT_NAME_SIZE> alias;
		memcpy(alias.data(), entry.shortRaw, alias.size());
		aliases->push_back(alias);
		names->emplace_back(entry.name);
	}

	return G_FS_MUTATION_SUCCESSFUL;
}

static g_fs_mutation_status fatDirectoryInitializeCluster(g_fat_mount* mount, const g_fat_node& parent,
														  uint32_t* firstCluster)
{
	std::vector<uint32_t> allocated;
	auto status = fatAllocateClusters(mount, 1, &allocated);
	if(status != G_FS_MUTATION_SUCCESSFUL)
		return status;

	*firstCluster = allocated[0];
	uint8_t dots[G_FAT_DOT_ENTRY_COUNT * G_FAT_DIRECTORY_ENTRY_SIZE]{};
	memset(dots, ' ', G_FAT_SHORT_NAME_SIZE);
	memset(dots + G_FAT_DIRECTORY_ENTRY_SIZE, ' ', G_FAT_SHORT_NAME_SIZE);
	dots[0] = dots[G_FAT_DIRECTORY_ENTRY_SIZE] = dots[G_FAT_DIRECTORY_ENTRY_SIZE + 1] = '.';
	fatInitializeDirectoryEntry(dots, *firstCluster);
	fatInitializeDirectoryEntry(dots + G_FAT_DIRECTORY_ENTRY_SIZE, parent.id == G_FAT_ROOT ? 0 : parent.firstCluster);
	if(!fatVolumeWrite(mount, fatClusterOffset(mount->volume, *firstCluster), dots, sizeof(dots)))
		return G_FS_MUTATION_ERROR;
	if(!fatVolumeFlush(mount))
		return G_FS_MUTATION_ERROR;

	return G_FS_MUTATION_SUCCESSFUL;
}

static bool fatDirectoryWriteLongNames(g_fat_mount* mount, const uint16_t* units, size_t unitCount, uint8_t checksum,
									   size_t longCount, const uint64_t* slots)
{
	for(size_t i = 0; i < longCount; ++i)
	{
		auto part = longCount - i;
		uint8_t lfn[G_FAT_DIRECTORY_ENTRY_SIZE];
		memset(lfn, G_FAT_LONG_NAME_PADDING_BYTE, sizeof(lfn));
		lfn[0] = part | (i == 0 ? G_FAT_LONG_NAME_LAST_FLAG : 0);
		lfn[G_FAT_ENTRY_ATTRIBUTES_OFFSET] = G_FAT_ATTRIBUTE_LONG_NAME;
		lfn[G_FAT_LONG_NAME_TYPE_OFFSET] = 0;
		lfn[G_FAT_LONG_NAME_CHECKSUM_OFFSET] = checksum;
		fatWrite16(lfn + G_FAT_LONG_NAME_CLUSTER_OFFSET, 0);
		for(unsigned j = 0; j < G_FAT_LONG_NAME_UNITS_PER_ENTRY; ++j)
		{
			auto index = (part - 1) * G_FAT_LONG_NAME_UNITS_PER_ENTRY + j;
			uint16_t unit = G_FAT_LONG_NAME_PADDING;
			if(index < unitCount)
				unit = units[index];
			else if(index == unitCount)
				unit = 0;
			fatWrite16(lfn + G_FAT_LONG_NAME_OFFSETS[j], unit);
		}

		if(!fatVolumeWrite(mount, slots[i], lfn, sizeof(lfn)))
			return false;
	}

	if(!longCount)
		return true;

	return fatVolumeFlush(mount);
}

/**
 * Clear every reserved slot before overwriting the old directory end. A partial
 * long-name write can expose any following slot, so each must remain an end
 * marker until its contents are published. The reserved run contains only
 * deleted entries and records beyond the old end, never live entries.
 */
static bool fatDirectoryPrepareEnd(g_fat_mount* mount, const uint64_t* slots, size_t longCount, uint64_t next)
{
	uint8_t empty[(G_FAT_LONG_NAME_MAX_ENTRIES + 1) * G_FAT_DIRECTORY_ENTRY_SIZE]{};
	if(next != UINT64_MAX && !fatVolumeWrite(mount, next, empty, 1))
		return false;

	for(size_t first = 0; first <= longCount;)
	{
		auto end = first + 1;
		while(end <= longCount && slots[end] == slots[end - 1] + G_FAT_DIRECTORY_ENTRY_SIZE)
			++end;

		if(!fatVolumeWrite(mount, slots[first], empty, (end - first) * G_FAT_DIRECTORY_ENTRY_SIZE))
			return false;
		first = end;
	}

	return fatVolumeFlush(mount);
}

/**
 * Retires a group of directory slots and makes that group durable. Callers keep
 * the short entry in a separate group so long names disappear only afterwards.
 */
static bool fatDirectoryRetireSlots(g_fat_mount* mount, const uint64_t* slots, size_t count)
{
	uint8_t deleted = G_FAT_ENTRY_DELETED;
	for(size_t i = 0; i < count; ++i)
		if(!fatVolumeWrite(mount, slots[i], &deleted, 1))
			return false;

	return fatVolumeFlush(mount);
}

/**
 * Initialize a new directory's contents and long-name entries before publishing
 * its short entry. A replacement end marker must also be durable before the
 * short entry can expose stale records beyond the previous end of the directory.
 * The final short-entry flush makes the complete directory update durable.
 */
static g_fs_mutation_status fatDirectoryPublish(g_fat_mount* mount, g_fs_phys_id parentId, const char* name, bool isDirectory,
													 g_fat_directory_entry* created, g_fat_inode* moving, g_fs_phys_id replaced)
{
	g_fat_node parent;
	if(!fatNodeRead(mount, parentId, &parent) || !parent.directory)
		return G_FS_MUTATION_NOT_FOUND;

	auto permission = fatMutationAllowed(mount, fatInodeFind(mount, parentId));
	if(permission != G_FS_MUTATION_SUCCESSFUL)
		return permission;

	uint16_t units[G_FAT_LONG_NAME_MAX_UNITS];
	size_t unitCount;
	if(!fatNameEncode(name, units, &unitCount))
		return G_FS_MUTATION_INVALID_ARGUMENT;
	if(!moving && mount->inodeCount >= G_FAT_MAX_INODES)
		return G_FS_MUTATION_NO_SPACE;

	std::unique_ptr<g_fat_inode> allocated(moving ? nullptr : new(std::nothrow) g_fat_inode);
	auto inode = moving ? moving : allocated.get();
	if(!inode)
		return G_FS_MUTATION_ERROR;

	std::vector<std::array<uint8_t, G_FAT_SHORT_NAME_SIZE>> aliases;
	std::vector<std::string> names;
	auto scanStatus = fatDirectoryCollectNames(mount, parent, name, &aliases, &names, replaced);
	if(scanStatus != G_FS_MUTATION_SUCCESSFUL)
		return scanStatus;

	if(!moving && mount->inodeCount >= G_FAT_MAX_INODES)
		return G_FS_MUTATION_NO_SPACE;

	uint8_t raw[G_FAT_DIRECTORY_ENTRY_SIZE];
	if(!fatChooseAlias(name, aliases, names, raw))
		return G_FS_MUTATION_NO_SPACE;

	char alias[G_FAT_SHORT_NAME_BUFFER_SIZE];
	if(!fatShortNameDecode(raw, alias))
		return G_FS_MUTATION_INVALID_ARGUMENT;

	size_t longCount = 0;
	if(strcmp(name, alias))
		longCount = (unitCount + G_FAT_LONG_NAME_UNITS_PER_ENTRY - 1) / G_FAT_LONG_NAME_UNITS_PER_ENTRY;
	uint64_t slots[G_FAT_LONG_NAME_MAX_ENTRIES + 1];
	uint64_t next;
	bool consumesEnd;
	auto status = fatFindDirectorySlots(mount, parent, longCount + 1, slots, &next, &consumesEnd);
	if(status != G_FS_MUTATION_SUCCESSFUL)
		return status;

	uint32_t firstCluster = moving ? moving->node.firstCluster : 0;
	if(isDirectory && !moving)
	{
		status = fatDirectoryInitializeCluster(mount, parent, &firstCluster);
		if(status != G_FS_MUTATION_SUCCESSFUL)
			return status;
	}

	raw[G_FAT_ENTRY_ATTRIBUTES_OFFSET] = isDirectory ? G_FAT_ATTRIBUTE_DIRECTORY : G_FAT_ATTRIBUTE_ARCHIVE;
	fatWrite16(raw + G_FAT_ENTRY_CLUSTER_HIGH_OFFSET,
			   mount->volume.fatBits == G_FAT32_ENTRY_BITS ? firstCluster >> G_FAT_ENTRY_CLUSTER_HIGH_SHIFT : 0);
	fatWrite16(raw + G_FAT_ENTRY_CLUSTER_LOW_OFFSET, firstCluster);
	fatWrite16(raw + G_FAT_ENTRY_CREATED_DATE_OFFSET, G_FAT_DATE_MINIMUM);
	fatWrite16(raw + G_FAT_ENTRY_ACCESSED_DATE_OFFSET, G_FAT_DATE_MINIMUM);
	fatWrite16(raw + G_FAT_ENTRY_MODIFIED_DATE_OFFSET, G_FAT_DATE_MINIMUM);
	if(moving)
	{
		uint8_t previous[G_FAT_DIRECTORY_ENTRY_SIZE];
		if(!fatVolumeRead(&mount->device, mount->volume, moving->offset, previous, sizeof(previous)))
			return G_FS_MUTATION_ERROR;

		auto caseFlags = raw[G_FAT_ENTRY_CASE_FLAGS_OFFSET];
		memcpy(raw + G_FAT_ENTRY_ATTRIBUTES_OFFSET, previous + G_FAT_ENTRY_ATTRIBUTES_OFFSET,
			   G_FAT_DIRECTORY_ENTRY_SIZE - G_FAT_ENTRY_ATTRIBUTES_OFFSET);
		raw[G_FAT_ENTRY_CASE_FLAGS_OFFSET] = caseFlags;
	}

	auto checksum = fatShortNameChecksum(raw);
	if(consumesEnd && !fatDirectoryPrepareEnd(mount, slots, longCount, next))
		return G_FS_MUTATION_ERROR;

	if(!fatDirectoryWriteLongNames(mount, units, unitCount, checksum, longCount, slots))
		return G_FS_MUTATION_ERROR;
	if(!fatVolumeWrite(mount, slots[longCount], raw, sizeof(raw)))
		return G_FS_MUTATION_ERROR;
	if(!fatVolumeFlush(mount))
		return G_FS_MUTATION_ERROR;

	if(moving)
	{
		if(isDirectory && moving->parent != parentId)
		{
			uint8_t dotdot[G_FAT_DIRECTORY_ENTRY_SIZE];
			auto offset = fatClusterOffset(mount->volume, firstCluster) + G_FAT_DIRECTORY_ENTRY_SIZE;
			if(!fatVolumeRead(&mount->device, mount->volume, offset, dotdot, sizeof(dotdot)))
			{
				mount->failed = true;
				return G_FS_MUTATION_ERROR;
			}

			auto cluster = parentId == G_FAT_ROOT ? 0 : parent.firstCluster;
			fatWrite16(dotdot + G_FAT_ENTRY_CLUSTER_HIGH_OFFSET, cluster >> G_FAT_ENTRY_CLUSTER_HIGH_SHIFT);
			fatWrite16(dotdot + G_FAT_ENTRY_CLUSTER_LOW_OFFSET, cluster);
			if(!fatVolumeWrite(mount, offset, dotdot, sizeof(dotdot)) || !fatVolumeFlush(mount))
				return G_FS_MUTATION_ERROR;
		}

		if(!fatDirectoryRetireSlots(mount, moving->slots, 1) ||
		   !fatDirectoryRetireSlots(mount, moving->slots + 1, moving->slotCount - 1))
			return G_FS_MUTATION_ERROR;
	}
	else
		inode->node = {mount->nextInode++, firstCluster, 0, isDirectory};
	inode->offset = slots[longCount];
	inode->parent = parentId;
	inode->attributes = raw[G_FAT_ENTRY_ATTRIBUTES_OFFSET];
	inode->valid = true;
	inode->slotCount = longCount + 1;
	inode->slots[0] = slots[longCount];
	for(size_t i = 0; i < longCount; ++i)
		inode->slots[i + 1] = slots[i];
	*created = {};
	created->node = inode->node;
	strcpy(created->name, name);
	strcpy(created->alias, alias);
	memcpy(created->shortRaw, raw, G_FAT_SHORT_NAME_SIZE);
	if(!moving)
	{
		inode->next = mount->inodes;
		mount->inodes = allocated.release();
		++mount->inodeCount;
	}
	return G_FS_MUTATION_SUCCESSFUL;
}

/**
 * Retire and flush the short directory entry before deleting its long-name slots
 * or freeing clusters. Open unlinked files retain their clusters until close.
 */
g_fs_mutation_status fatNodeRemove(g_fat_mount* mount, g_fs_phys_id id)
{
	if(id == G_FAT_ROOT)
		return G_FS_MUTATION_INVALID_ARGUMENT;

	g_fat_node node;
	if(!fatNodeRead(mount, id, &node))
		return G_FS_MUTATION_NOT_FOUND;

	auto inode = fatInodeFind(mount, id);
	auto permission = fatMutationAllowed(mount, inode);
	if(permission != G_FS_MUTATION_SUCCESSFUL)
		return permission;
	if(inode->unlinked)
		return G_FS_MUTATION_NOT_FOUND;

	permission = fatMutationAllowed(mount, fatInodeFind(mount, inode->parent));
	if(permission != G_FS_MUTATION_SUCCESSFUL)
		return permission;
	if(node.directory)
	{
		g_fat_directory directory;
		g_fat_directory_entry child;
		if(!fatDirectoryOpen(mount, node, &directory))
			return G_FS_MUTATION_ERROR;

		auto result = fatDirectoryNext(mount, &directory, &child);
		if(result == G_FAT_SUCCESS)
			return G_FS_MUTATION_NOT_EMPTY;
		if(result != G_FAT_END)
			return G_FS_MUTATION_ERROR;
	}

	std::vector<uint32_t> chain;
	if(!fatCollectChain(mount, node.firstCluster, &chain))
		return G_FS_MUTATION_ERROR;

	if(!fatDirectoryRetireSlots(mount, inode->slots, 1))
		return G_FS_MUTATION_ERROR;

	inode->unlinked = true;
	if(!fatDirectoryRetireSlots(mount, inode->slots + 1, inode->slotCount - 1))
		return G_FS_MUTATION_ERROR;
	if(!inode->opens)
	{
		if(!fatFreeClusters(mount, chain))
			return G_FS_MUTATION_ERROR;

		fatInodeRelease(mount, inode);
	}

	return G_FS_MUTATION_SUCCESSFUL;
}

g_fs_mutation_status fatDirectoryCreate(g_fat_mount* mount, g_fs_phys_id parent, const char* name, bool directory,
										g_fat_directory_entry* created)
{
	return fatDirectoryPublish(mount, parent, name, directory, created, nullptr, UINT64_MAX);
}

/**
 * Publish a new name for the existing inode and allocation chain, then retire
 * its old slots. FAT metadata updates are ordered and flushed, not journaled.
 */
g_fs_mutation_status fatNodeRename(g_fat_mount* mount, g_fs_phys_id id, g_fs_phys_id parentId,
									const char* name, g_fs_phys_id replaced)
{
	if(id == G_FAT_ROOT)
		return G_FS_MUTATION_BUSY;

	g_fat_node node, parent;
	if(!fatNodeRead(mount, id, &node) || !fatNodeRead(mount, parentId, &parent) || !parent.directory)
		return G_FS_MUTATION_NOT_FOUND;
	auto inode = fatInodeFind(mount, id);
	auto permission = fatMutationAllowed(mount, inode);
	if(permission != G_FS_MUTATION_SUCCESSFUL)
		return permission;
	if(!inode || inode->unlinked)
		return G_FS_MUTATION_NOT_FOUND;
	permission = fatMutationAllowed(mount, fatInodeFind(mount, inode->parent));
	if(permission != G_FS_MUTATION_SUCCESSFUL)
		return permission;
	permission = fatMutationAllowed(mount, fatInodeFind(mount, parentId));
	if(permission != G_FS_MUTATION_SUCCESSFUL)
		return permission;

	uint32_t ancestors = 0;
	for(auto ancestor = parentId; ancestor != G_FAT_ROOT;)
	{
		if(ancestor == id)
			return G_FS_MUTATION_INVALID_ARGUMENT;
		if(++ancestors > mount->inodeCount)
			return G_FS_MUTATION_ERROR;

		auto current = fatInodeFind(mount, ancestor);
		if(!current || current->parent == ancestor)
			return G_FS_MUTATION_ERROR;
		ancestor = current->parent;
	}

	uint16_t units[G_FAT_LONG_NAME_MAX_UNITS];
	size_t count;
	if(!fatNameEncode(name, units, &count))
		return G_FS_MUTATION_INVALID_ARGUMENT;

	bool replacing = false;
	g_fat_directory directory;
	if(!fatDirectoryOpen(mount, parent, &directory))
		return G_FS_MUTATION_ERROR;
	for(;;)
	{
		g_fat_directory_entry entry;
		auto result = fatDirectoryNext(mount, &directory, &entry);
		if(result == G_FAT_END)
			break;
		if(result != G_FAT_SUCCESS)
			return G_FS_MUTATION_ERROR;
		if(!fatNameEquals(name, entry.name) && !fatNameEquals(name, entry.alias))
			continue;
		if(entry.node.id == id)
			return G_FS_MUTATION_ALREADY_EXISTS;
		if(entry.node.id != replaced || node.directory || entry.node.directory)
			return G_FS_MUTATION_ALREADY_EXISTS;

		permission = fatMutationAllowed(mount, fatInodeFind(mount, replaced));
		if(permission != G_FS_MUTATION_SUCCESSFUL)
			return permission;
		replacing = true;
		break;
	}

	g_fat_directory_entry created;
	auto status = fatDirectoryPublish(mount, parentId, name, node.directory, &created, inode,
		replacing ? replaced : UINT64_MAX);
	if(status != G_FS_MUTATION_SUCCESSFUL)
		return status;
	if(replacing)
	{
		status = fatNodeRemove(mount, replaced);
		if(status != G_FS_MUTATION_SUCCESSFUL)
			mount->failed = true;
	}

	return status;
}
