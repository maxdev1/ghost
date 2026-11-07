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

#include <cstring>
#include "fatdriver.hpp"

bool fatDirectoryOpen(g_fat_mount* mount, const g_fat_node& node, g_fat_directory* directory)
{
	if(!node.directory)
		return false;

	*directory = {};
	directory->node = node;
	if(node.id != G_FAT_ROOT || mount->volume.fatBits == G_FAT32_ENTRY_BITS)
	{
		auto chain = fatChainGet(mount, node.firstCluster);
		if(!chain)
			return false;

		directory->cluster = node.firstCluster;
		directory->clustersRemaining = chain->count;
	}

	return true;
}

static g_fat_result fatDirectoryLoadSector(g_fat_mount* mount, g_fat_directory* directory)
{
	auto& volume = mount->volume;
	uint64_t sector;
	if(directory->node.id == G_FAT_ROOT && volume.fatBits != G_FAT32_ENTRY_BITS)
	{
		if(directory->fixedSector >= volume.rootSectors)
			return G_FAT_END;

		sector = volume.rootSector + directory->fixedSector;
		++directory->fixedSector;
	}
	else
	{
		if(directory->sectorInCluster == volume.sectorsPerCluster)
		{
			--directory->clustersRemaining;
			if(directory->clustersRemaining == 0)
				return G_FAT_END;

			uint32_t next;
			if(fatClusterNext(mount, directory->cluster, &next) != G_FAT_SUCCESS)
				return G_FAT_ERROR;

			directory->cluster = next;
			directory->sectorInCluster = 0;
		}

		sector = fatClusterOffset(volume, directory->cluster) / G_BLOCK_SECTOR_SIZE + directory->sectorInCluster;
		++directory->sectorInCluster;
	}

	directory->sectorOffset = sector * G_BLOCK_SECTOR_SIZE;
	if(!fatMetadataRead(mount, directory->sectorOffset, directory->bytes, sizeof(directory->bytes)))
		return G_FAT_ERROR;

	directory->position = 0;
	return G_FAT_SUCCESS;
}

static bool fatDirectoryIsDot(const uint8_t* raw)
{
	if(raw[0] != '.')
		return false;

	unsigned start = raw[1] == '.' ? 2 : 1;
	for(unsigned i = start; i < G_FAT_SHORT_NAME_SIZE; ++i)
		if(raw[i] != ' ')
			return false;

	return true;
}

g_fat_result fatDirectoryNext(g_fat_mount* mount, g_fat_directory* directory, g_fat_directory_entry* entry)
{
	while(!directory->ended)
	{
		if(directory->position == G_BLOCK_SECTOR_SIZE)
		{
			auto result = fatDirectoryLoadSector(mount, directory);
			if(result != G_FAT_SUCCESS)
			{
				directory->ended = true;
				return result;
			}
		}

		auto offset = directory->sectorOffset + directory->position;
		auto raw = directory->bytes + directory->position;
		directory->position += G_FAT_DIRECTORY_ENTRY_SIZE;
		if(!raw[0])
		{
			directory->ended = true;
			return G_FAT_END;
		}

		if(raw[0] == G_FAT_ENTRY_DELETED)
		{
			directory->longName.active = false;
			continue;
		}

		if((raw[G_FAT_ENTRY_ATTRIBUTES_OFFSET] & G_FAT_LONG_NAME_ATTRIBUTE_MASK) == G_FAT_ATTRIBUTE_LONG_NAME)
		{
			fatLongNameAppend(&directory->longName, raw);
			if(directory->longName.active)
				directory->longOffsets[(raw[0] & G_FAT_LONG_NAME_ORDINAL_MASK) - 1] = offset;
			continue;
		}

		bool hasLongName = fatLongNameDecode(directory->longName, raw, entry->name);
		directory->longName.active = false;
		if((raw[G_FAT_ENTRY_ATTRIBUTES_OFFSET] & G_FAT_ATTRIBUTE_VOLUME) || fatDirectoryIsDot(raw))
			continue;
		if(!fatShortNameDecode(raw, entry->alias))
			return G_FAT_ERROR;
		if(!hasLongName)
			strcpy(entry->name, entry->alias);
		if(!fatNodeDecode(mount->volume, offset, raw, &entry->node))
			return G_FAT_ERROR;

		memcpy(entry->shortRaw, raw, sizeof(entry->shortRaw));
		if(!fatInodeObserve(mount, &entry->node, directory->node.id, offset, directory->longOffsets,
							hasLongName ? directory->longName.length / G_FAT_LONG_NAME_UNITS_PER_ENTRY : 0, raw[G_FAT_ENTRY_ATTRIBUTES_OFFSET]))
			return G_FAT_ERROR;

		return G_FAT_SUCCESS;
	}

	return G_FAT_END;
}

g_fat_result fatDirectoryRawNext(g_fat_mount* mount, g_fat_directory* directory, uint64_t* offset, uint8_t* raw)
{
	if(directory->ended)
		return G_FAT_END;
	if(directory->position == G_BLOCK_SECTOR_SIZE)
	{
		auto result = fatDirectoryLoadSector(mount, directory);
		if(result != G_FAT_SUCCESS)
		{
			directory->ended = true;
			return result;
		}
	}

	*offset = directory->sectorOffset + directory->position;
	memcpy(raw, directory->bytes + directory->position, G_FAT_DIRECTORY_ENTRY_SIZE);
	directory->position += G_FAT_DIRECTORY_ENTRY_SIZE;
	return G_FAT_SUCCESS;
}
