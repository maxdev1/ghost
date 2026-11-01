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

static char fatAliasUpper(char c) { return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c; }

static bool fatAliasAvailable(const std::vector<std::array<uint8_t, 11>>& aliases,
							  const std::vector<std::string>& names, const uint8_t* candidate)
{
	for(auto& alias: aliases)
		if(!memcmp(alias.data(), candidate, 11))
			return false;

	char name[40];
	if(!fatShortNameDecode(candidate, name))
		return false;
	for(auto& existing: names)
		if(fatNameEquals(name, existing.c_str()))
			return false;
	return true;
}

static bool fatChooseAlias(const char* name, const std::vector<std::array<uint8_t, 11>>& aliases,
						   const std::vector<std::string>& names, uint8_t* raw)
{
	memset(raw, 0, 32);
	memset(raw, ' ', 11);
	auto dot = strrchr(name, '.');
	auto baseLength = dot ? size_t(dot - name) : strlen(name);
	auto extensionLength = dot ? strlen(dot + 1) : 0;
	bool plain = baseLength && baseLength <= 8 && extensionLength <= 3;
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
		raw[8 + i] = c;
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
			raw[12] |= 0x08;
		if(extLower && !extUpper)
			raw[12] |= 0x10;
		return true;
	}

	char prefix[9]{};
	size_t prefixLength = 0;
	for(size_t i = 0; i < baseLength && prefixLength < 8; ++i)
		if(fatAliasCharacter(fatAliasUpper(name[i])))
			prefix[prefixLength++] = fatAliasUpper(name[i]);
	if(!prefixLength)
		strcpy(prefix, "FILE");
	for(unsigned serial = 1; serial <= 999999; ++serial)
	{
		char tail[8];
		snprintf(tail, sizeof(tail), "~%u", serial);
		memset(raw, ' ', 11);
		raw[12] = 0;
		auto count = std::min(strlen(prefix), size_t(8 - strlen(tail)));
		memcpy(raw, prefix, count);
		memcpy(raw + count, tail, strlen(tail));
		for(size_t i = 0; i < extensionLength && i < 3; ++i)
			raw[8 + i] = fatAliasCharacter(fatAliasUpper(dot[i + 1])) ? fatAliasUpper(dot[i + 1]) : '_';
		if(fatAliasAvailable(aliases, names, raw))
			return true;
	}
	return false;
}

static g_fs_mutation_status fatFindDirectorySlots(g_fat_mount* mount, const g_fat_node& parent, unsigned needed,
												  uint64_t* slots, uint64_t* next, bool* consumesEnd)
{
	for(unsigned attempt = 0; attempt < 2; ++attempt)
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
		if(parent.id == G_FAT_ROOT && mount->volume.fatBits != 32)
			return G_FS_MUTATION_NO_SPACE;
		if(attempt)
			return G_FS_MUTATION_NO_SPACE;

		std::vector<uint32_t> chain, added;
		if(!fatCollectChain(mount, parent.firstCluster, &chain) || chain.empty())
			return G_FS_MUTATION_ERROR;
		auto entriesPerCluster = unsigned(mount->volume.sectorsPerCluster) * G_BLOCK_SECTOR_SIZE / 32;
		auto status = fatAllocateClusters(mount, (needed - count + entriesPerCluster - 1) / entriesPerCluster, &added);
		if(status != G_FS_MUTATION_SUCCESSFUL)
			return status;
		if(!fatTableSet(mount, chain.back(), added.front(), true) || !fatVolumeFlush(mount))
			return G_FS_MUTATION_ERROR;
		if(!fatChainRemember(mount, parent.firstCluster, chain.size() + added.size(), added.back()))
			return G_FS_MUTATION_ERROR;
	}
	return G_FS_MUTATION_NO_SPACE;
}

static void fatInitializeDirectoryEntry(uint8_t* raw, uint32_t cluster)
{
	raw[11] = G_FAT_ATTRIBUTE_DIRECTORY;
	fatWrite16(raw + 20, cluster >> 16);
	fatWrite16(raw + 26, cluster);
	fatWrite16(raw + 16, 0x21);
	fatWrite16(raw + 18, 0x21);
	fatWrite16(raw + 24, 0x21);
}

static g_fs_mutation_status fatDirectoryCollectNames(g_fat_mount* mount, const g_fat_node& parent, const char* name,
													 std::vector<std::array<uint8_t, 11>>* aliases,
													 std::vector<std::string>* names)
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
		if(fatNameEquals(name, entry.name) || fatNameEquals(name, entry.alias))
			return G_FS_MUTATION_ALREADY_EXISTS;
		std::array<uint8_t, 11> alias;
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
	uint8_t dots[2 * G_FAT_DIRECTORY_ENTRY_SIZE]{};
	memset(dots, ' ', 11);
	memset(dots + G_FAT_DIRECTORY_ENTRY_SIZE, ' ', 11);
	dots[0] = dots[G_FAT_DIRECTORY_ENTRY_SIZE] = dots[G_FAT_DIRECTORY_ENTRY_SIZE + 1] = '.';
	fatInitializeDirectoryEntry(dots, *firstCluster);
	fatInitializeDirectoryEntry(dots + G_FAT_DIRECTORY_ENTRY_SIZE, parent.id == G_FAT_ROOT ? 0 : parent.firstCluster);
	if(!fatVolumeWrite(mount, fatClusterOffset(mount->volume, *firstCluster), dots, sizeof(dots)) ||
	   !fatVolumeFlush(mount))
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
		memset(lfn, 0xff, sizeof(lfn));
		lfn[0] = part | (i == 0 ? 0x40 : 0);
		lfn[11] = G_FAT_ATTRIBUTE_LONG_NAME;
		lfn[12] = 0;
		lfn[13] = checksum;
		fatWrite16(lfn + 26, 0);
		for(unsigned j = 0; j < G_FAT_LONG_NAME_UNITS_PER_ENTRY; ++j)
		{
			auto index = (part - 1) * G_FAT_LONG_NAME_UNITS_PER_ENTRY + j;
			fatWrite16(lfn + G_FAT_LONG_NAME_OFFSETS[j], index < unitCount	  ? units[index]
														 : index == unitCount ? 0
																			  : 0xffff);
		}
		if(!fatVolumeWrite(mount, slots[i], lfn, sizeof(lfn)))
			return false;
	}
	return !longCount || fatVolumeFlush(mount);
}

/**
 * Initialize a new directory's contents and long-name entries before publishing
 * its short entry. The final short entry is the point where the name becomes
 * visible, and its flush makes the complete directory update durable.
 */
g_fs_mutation_status fatDirectoryCreate(g_fat_mount* mount, g_fs_phys_id parentId, const char* name, bool isDirectory,
										g_fat_directory_entry* created)
{
	g_fat_node parent;
	if(!fatNodeRead(mount, parentId, &parent) || !parent.directory)
		return G_FS_MUTATION_NOT_FOUND;
	auto permission = fatMutationAllowed(mount, fatInodeFind(mount, parentId));
	if(permission != G_FS_MUTATION_SUCCESSFUL)
		return permission;
	uint16_t units[255];
	size_t unitCount;
	if(!fatNameEncode(name, units, &unitCount))
		return G_FS_MUTATION_INVALID_ARGUMENT;
	if(mount->inodeCount >= G_FAT_MAX_INODES)
		return G_FS_MUTATION_NO_SPACE;

	std::unique_ptr<g_fat_inode> inode(new(std::nothrow) g_fat_inode);
	if(!inode)
		return G_FS_MUTATION_ERROR;
	std::vector<std::array<uint8_t, 11>> aliases;
	std::vector<std::string> names;
	auto scanStatus = fatDirectoryCollectNames(mount, parent, name, &aliases, &names);
	if(scanStatus != G_FS_MUTATION_SUCCESSFUL)
		return scanStatus;

	if(mount->inodeCount >= G_FAT_MAX_INODES)
		return G_FS_MUTATION_NO_SPACE;

	uint8_t raw[G_FAT_DIRECTORY_ENTRY_SIZE];
	if(!fatChooseAlias(name, aliases, names, raw))
		return G_FS_MUTATION_NO_SPACE;
	char alias[40];
	if(!fatShortNameDecode(raw, alias))
		return G_FS_MUTATION_INVALID_ARGUMENT;
	auto longCount = strcmp(name, alias)
							 ? (unitCount + G_FAT_LONG_NAME_UNITS_PER_ENTRY - 1) / G_FAT_LONG_NAME_UNITS_PER_ENTRY
							 : 0;
	uint64_t slots[G_FAT_LONG_NAME_MAX_ENTRIES + 1];
	uint64_t next;
	bool consumesEnd;
	auto status = fatFindDirectorySlots(mount, parent, longCount + 1, slots, &next, &consumesEnd);
	if(status != G_FS_MUTATION_SUCCESSFUL)
		return status;

	uint32_t firstCluster = 0;
	if(isDirectory)
	{
		status = fatDirectoryInitializeCluster(mount, parent, &firstCluster);
		if(status != G_FS_MUTATION_SUCCESSFUL)
			return status;
	}
	raw[11] = isDirectory ? G_FAT_ATTRIBUTE_DIRECTORY : G_FAT_ATTRIBUTE_ARCHIVE;
	fatWrite16(raw + 20, mount->volume.fatBits == 32 ? firstCluster >> 16 : 0);
	fatWrite16(raw + 26, firstCluster);
	fatWrite16(raw + 16, 0x21);
	fatWrite16(raw + 18, 0x21);
	fatWrite16(raw + 24, 0x21);
	auto checksum = fatShortNameChecksum(raw);
	if(consumesEnd && next != UINT64_MAX)
	{
		uint8_t end = 0;
		if(!fatVolumeWrite(mount, next, &end, 1))
			return G_FS_MUTATION_ERROR;
	}

	if(!fatDirectoryWriteLongNames(mount, units, unitCount, checksum, longCount, slots))
		return G_FS_MUTATION_ERROR;
	if(!fatVolumeWrite(mount, slots[longCount], raw, sizeof(raw)) || !fatVolumeFlush(mount))
		return G_FS_MUTATION_ERROR;

	inode->node = {mount->nextInode++, firstCluster, 0, isDirectory};
	inode->offset = slots[longCount];
	inode->parent = parentId;
	inode->attributes = raw[11];
	inode->valid = true;
	inode->slotCount = longCount + 1;
	inode->slots[0] = slots[longCount];
	for(size_t i = 0; i < longCount; ++i)
		inode->slots[i + 1] = slots[i];
	*created = {};
	created->node = inode->node;
	strcpy(created->name, name);
	strcpy(created->alias, alias);
	memcpy(created->shortRaw, raw, 11);
	inode->next = mount->inodes;
	mount->inodes = inode.release();
	++mount->inodeCount;
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

	uint8_t deleted = G_FAT_ENTRY_DELETED;
	if(!fatVolumeWrite(mount, inode->slots[0], &deleted, 1) || !fatVolumeFlush(mount))
		return G_FS_MUTATION_ERROR;
	inode->unlinked = true;
	for(unsigned i = 1; i < inode->slotCount; ++i)
		if(!fatVolumeWrite(mount, inode->slots[i], &deleted, 1))
			return G_FS_MUTATION_ERROR;
	if(!fatVolumeFlush(mount))
		return G_FS_MUTATION_ERROR;
	if(!inode->opens && !fatFreeClusters(mount, chain))
		return G_FS_MUTATION_ERROR;
	return G_FS_MUTATION_SUCCESSFUL;
}
