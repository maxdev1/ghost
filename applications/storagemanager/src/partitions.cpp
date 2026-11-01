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

#include "partitions.hpp"
#include <cstring>
#include <set>
#include "disk.hpp"

static uint32_t diskCrc32(uint32_t crc, const uint8_t* bytes, size_t count)
{
	for(size_t i = 0; i < count; ++i)
	{
		crc ^= bytes[i];
		for(unsigned bit = 0; bit < 8; ++bit)
			crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1)));
	}
	return crc;
}

static bool diskAddPartition(const g_volume_info& disk, uint64_t first, uint64_t count, uint32_t number,
							 g_volume_scheme scheme, std::vector<g_volume_info>* volumes)
{
	if(!first || !count || first >= disk.sectorCount || count > disk.sectorCount - first)
		return false;

	for(const auto& volume: *volumes)
		if(first < volume.firstSector + volume.sectorCount && volume.firstSector < first + count)
			return false;

	auto volume = disk;
	volume.firstSector = first;
	volume.sectorCount = count;
	volume.partition = number;
	volume.scheme = scheme;
	volumes->push_back(volume);
	return true;
}

static g_volume_status diskDiscoverGpt(const g_volume_info& disk, g_volume_sector_reader read, void* context,
									   std::vector<g_volume_info>* volumes)
{
	uint8_t header[G_VOLUME_SECTOR_SIZE];
	if(!read(context, 1, header))
		return G_VOLUME_IO_ERROR;

	if(memcmp(header, "EFI PART", 8) || diskRead32(header + 8) != 0x10000 || diskRead32(header + 12) < 92 ||
	   diskRead32(header + 12) > G_VOLUME_SECTOR_SIZE || diskRead32(header + 20) || diskRead64(header + 24) != 1 ||
	   diskRead64(header + 32) != disk.sectorCount - 1)
		return G_VOLUME_INVALID;

	auto crc = diskRead32(header + 16);
	memset(header + 16, 0, 4);
	if(~diskCrc32(UINT32_MAX, header, diskRead32(header + 12)) != crc)
		return G_VOLUME_INVALID;

	auto firstUsable = diskRead64(header + 40);
	auto lastUsable = diskRead64(header + 48);
	auto table = diskRead64(header + 72);
	auto entries = diskRead32(header + 80);
	auto entrySize = diskRead32(header + 84);
	if(!entries || entries > 128 || entrySize != 128)
		return G_VOLUME_UNSUPPORTED;

	auto tableSectors = (uint64_t(entries) * entrySize + G_VOLUME_SECTOR_SIZE - 1) / G_VOLUME_SECTOR_SIZE;
	if(table < 2 || firstUsable < table || tableSectors > firstUsable - table || firstUsable > lastUsable ||
	   lastUsable >= disk.sectorCount - 1 || tableSectors > disk.sectorCount - 2 - lastUsable)
		return G_VOLUME_INVALID;

	uint32_t tableCrc = UINT32_MAX;
	uint8_t sector[G_VOLUME_SECTOR_SIZE];
	for(uint32_t i = 0; i < entries; ++i)
	{
		if(i % 4 == 0 && !read(context, table + i / 4, sector))
			return G_VOLUME_IO_ERROR;

		auto entry = sector + (i % 4) * 128;
		tableCrc = diskCrc32(tableCrc, entry, 128);
		bool empty = true;
		for(unsigned j = 0; j < 16; ++j)
			empty &= entry[j] == 0;
		if(empty)
			continue;

		auto first = diskRead64(entry + 32);
		auto last = diskRead64(entry + 40);
		if(first < firstUsable || first > last || last > lastUsable ||
		   !diskAddPartition(disk, first, last - first + 1, i + 1, G_VOLUME_GPT, volumes))
			return G_VOLUME_INVALID;
	}
	return ~tableCrc == diskRead32(header + 88) ? G_VOLUME_OK : G_VOLUME_INVALID;
}

static g_volume_status diskDiscoverMbr(const g_volume_info& disk, const uint8_t* mbr, g_volume_sector_reader read,
									   void* context, std::vector<g_volume_info>* volumes)
{
	uint64_t extendedFirst = 0;
	uint64_t extendedCount = 0;

	for(unsigned i = 0; i < 4; ++i)
	{
		auto entry = mbr + 446 + i * 16;
		auto first = diskRead32(entry + 8);
		auto count = diskRead32(entry + 12);
		if(!entry[4] && !first && !count)
			continue;
		if((entry[0] != 0 && entry[0] != 0x80) || !entry[4] || entry[4] == 0xee || !first || !count ||
		   first >= disk.sectorCount || count > disk.sectorCount - first)
			return G_VOLUME_INVALID;

		if(diskIsExtended(entry[4]))
		{
			if(extendedFirst)
				return G_VOLUME_INVALID;
			extendedFirst = first;
			extendedCount = count;
		}
		else if(!diskAddPartition(disk, first, count, i + 1, G_VOLUME_MBR, volumes))
			return G_VOLUME_INVALID;
	}

	if(!extendedFirst)
		return G_VOLUME_OK;

	for(const auto& volume: *volumes)
		if(extendedFirst < volume.firstSector + volume.sectorCount &&
		   volume.firstSector < extendedFirst + extendedCount)
			return G_VOLUME_INVALID;

	std::set<uint64_t> visited;
	uint64_t current = extendedFirst;
	for(unsigned number = 5; number < 133; ++number)
	{
		if(current < extendedFirst || current >= extendedFirst + extendedCount || !visited.insert(current).second)
			return G_VOLUME_INVALID;

		for(const auto& volume: *volumes)
			if(current >= volume.firstSector && current < volume.firstSector + volume.sectorCount)
				return G_VOLUME_INVALID;

		uint8_t sector[G_VOLUME_SECTOR_SIZE];
		if(!read(context, current, sector))
			return G_VOLUME_IO_ERROR;
		if(!diskHasSignature(sector))
			return G_VOLUME_INVALID;

		auto entry = sector + 446;
		auto offset = diskRead32(entry + 8);
		auto count = diskRead32(entry + 12);
		if(!entry[4] || diskIsExtended(entry[4]) || (entry[0] != 0 && entry[0] != 0x80) || !offset ||
		   offset >= extendedFirst + extendedCount - current ||
		   count > extendedFirst + extendedCount - current - offset ||
		   !diskAddPartition(disk, current + offset, count, number, G_VOLUME_MBR, volumes))
			return G_VOLUME_INVALID;

		auto link = entry + 16;
		for(unsigned i = 2; i < 4; ++i)
			if(sector[446 + i * 16 + 4] || diskRead32(sector + 446 + i * 16 + 8) ||
			   diskRead32(sector + 446 + i * 16 + 12))
				return G_VOLUME_INVALID;

		if(!link[4] && !diskRead32(link + 8) && !diskRead32(link + 12))
			return G_VOLUME_OK;

		if(!diskIsExtended(link[4]) || (link[0] != 0 && link[0] != 0x80) || diskRead32(link + 8) >= extendedCount ||
		   !diskRead32(link + 12) || diskRead32(link + 12) > extendedCount - diskRead32(link + 8))
			return G_VOLUME_INVALID;

		current = extendedFirst + diskRead32(link + 8);
	}
	return G_VOLUME_UNSUPPORTED;
}

g_volume_status diskDiscoverPartitions(const g_volume_info& disk, g_volume_sector_reader read, void* context,
									   std::vector<g_volume_info>* volumes)
{
	if(!read || !volumes || !disk.sectorCount || disk.sectorSize != G_VOLUME_SECTOR_SIZE)
		return G_VOLUME_UNSUPPORTED;

	volumes->clear();
	uint8_t sector[G_VOLUME_SECTOR_SIZE];
	if(!read(context, 0, sector))
		return G_VOLUME_IO_ERROR;

	g_volume_status status = G_VOLUME_OK;
	bool signature = diskHasSignature(sector);
	bool filesystem = diskHasFilesystem(sector);
	if(signature && !filesystem)
	{
		bool gpt = false;
		for(unsigned i = 0; i < 4; ++i)
			gpt |= sector[450 + i * 16] == 0xee;
		status = gpt ? diskDiscoverGpt(disk, read, context, volumes)
					 : diskDiscoverMbr(disk, sector, read, context, volumes);
	}

	if(status != G_VOLUME_OK)
	{
		volumes->clear();
		return status;
	}

	if(volumes->empty())
		volumes->push_back(disk);
	return G_VOLUME_OK;
}
