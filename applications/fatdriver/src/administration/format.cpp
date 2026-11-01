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
#include "fatdriver.hpp"

static constexpr uint32_t G_FAT_FORMAT_RESERVED_SECTORS = 32;
static constexpr uint32_t G_FAT_FORMAT_TABLE_COUNT = 2;
static constexpr uint32_t G_FAT_FORMAT_INFO_SECTOR = 1;
static constexpr uint32_t G_FAT_FORMAT_BACKUP_SECTOR = 6;

struct g_fat_format_layout
{
	uint32_t sectorsPerCluster;
	uint32_t sectorsPerFat;
	uint64_t dataSector;
};

static bool fatFormatTransfer(g_fat_device* device, uint64_t lba, const uint8_t* bytes, uint32_t count)
{
	while(count)
	{
		auto sectors =
				std::min(count, std::min(device->info.maxTransferSectors, device->buffer.size / G_BLOCK_SECTOR_SIZE));
		if(!sectors || lba >= device->info.sectorCount || sectors > device->info.sectorCount - lba)
			return false;
		memcpy(device->buffer.data, bytes, sectors * G_BLOCK_SECTOR_SIZE);
		device->status = blockDeviceWriteBuffer(&device->buffer, device->id, lba, sectors);
		if(device->status != G_BLOCK_SUCCESS)
			return false;
		device->status = blockDeviceReadBuffer(&device->buffer, device->id, lba, sectors);
		if(device->status != G_BLOCK_SUCCESS || memcmp(device->buffer.data, bytes, sectors * G_BLOCK_SECTOR_SIZE))
			return false;
		bytes += sectors * G_BLOCK_SECTOR_SIZE;
		count -= sectors;
		lba += sectors;
	}
	return true;
}

static bool fatFormatEncodeLabel(const char* label, char* name)
{
	memset(name, ' ', 11);
	if(!*label)
		memcpy(name, "NO NAME", 7);
	for(size_t i = 0; label[i]; ++i)
	{
		unsigned char c = label[i];
		if(c < 32 || c > 126 || strchr("\"*+,./:;<=>?[\\]|", c))
			return false;
		name[i] = c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
	}
	return true;
}

static bool fatFormatChooseLayout(uint64_t count, g_fat_format_layout* layout)
{
	if(count < 65525 + G_FAT_FORMAT_RESERVED_SECTORS)
		return false;
	uint32_t sectorsPerCluster = 1;
	while(count / sectorsPerCluster > 262144 && sectorsPerCluster < 128)
		sectorsPerCluster *= 2;
	const auto entriesPerSector = G_BLOCK_SECTOR_SIZE / sizeof(uint32_t);
	layout->sectorsPerCluster = sectorsPerCluster;
	layout->sectorsPerFat =
			(count - G_FAT_FORMAT_RESERVED_SECTORS + 2 * sectorsPerCluster + entriesPerSector * sectorsPerCluster + 1) /
			(entriesPerSector * sectorsPerCluster + G_FAT_FORMAT_TABLE_COUNT);
	layout->dataSector = G_FAT_FORMAT_RESERVED_SECTORS + uint64_t(G_FAT_FORMAT_TABLE_COUNT) * layout->sectorsPerFat;
	if(layout->dataSector + sectorsPerCluster > count)
		return false;
	auto clusters = (count - layout->dataSector) / sectorsPerCluster;
	return clusters >= 65525 && clusters <= 0x0ffffff3;
}

/**
 * Invalidate the old primary boot sector before replacing metadata. The cleared
 * area includes both FATs and the root cluster, never the rest of the volume.
 */
static bool fatFormatClearMetadata(g_fat_device* device, uint64_t first, const g_fat_format_layout& layout,
								   const uint8_t* zero, g_fat_format_progress progress, void* context)
{
	if(!fatFormatTransfer(device, first, zero, 1) || blockDeviceFlush(device->handler, device->id) != G_BLOCK_SUCCESS)
		return false;
	auto total = layout.dataSector + layout.sectorsPerCluster;
	for(uint64_t done = 0; done < total;)
	{
		auto sectors = uint32_t(std::min(uint64_t(G_BLOCK_DMA_BUFFER_BYTES / G_BLOCK_SECTOR_SIZE), total - done));
		if(!fatFormatTransfer(device, first + done, zero, sectors))
			return false;
		done += sectors;
		if(progress)
			progress(context, done, total);
	}
	return true;
}

static bool fatFormatInitializeTables(g_fat_device* device, uint64_t first, const g_fat_format_layout& layout,
									  uint8_t* bytes)
{
	fatWrite32(bytes, 0x0ffffff8);
	fatWrite32(bytes + 4, 0x0fffffff);
	fatWrite32(bytes + 8, 0x0fffffff);
	for(unsigned copy = 0; copy < G_FAT_FORMAT_TABLE_COUNT; ++copy)
	{
		auto sector = first + G_FAT_FORMAT_RESERVED_SECTORS + uint64_t(copy) * layout.sectorsPerFat;
		if(!fatFormatTransfer(device, sector, bytes, 1))
			return false;
	}
	return true;
}

static void fatFormatBuildBoot(uint8_t* boot, const g_fat_format_layout& layout, uint32_t first, uint32_t count,
							   uint32_t serial, const char* name)
{
	boot[0] = 0xeb;
	boot[1] = 0x58;
	boot[2] = 0x90;
	memcpy(boot + 3, "GHOST   ", 8);
	fatWrite16(boot + 11, G_BLOCK_SECTOR_SIZE);
	boot[13] = layout.sectorsPerCluster;
	fatWrite16(boot + 14, G_FAT_FORMAT_RESERVED_SECTORS);
	boot[16] = G_FAT_FORMAT_TABLE_COUNT;
	boot[21] = 0xf8;
	fatWrite16(boot + 24, 63);
	fatWrite16(boot + 26, 255);
	fatWrite32(boot + 28, first);
	fatWrite32(boot + 32, count);
	fatWrite32(boot + 36, layout.sectorsPerFat);
	fatWrite32(boot + 44, 2);
	fatWrite16(boot + 48, G_FAT_FORMAT_INFO_SECTOR);
	fatWrite16(boot + 50, G_FAT_FORMAT_BACKUP_SECTOR);
	boot[64] = 0x80;
	boot[66] = 0x29;
	fatWrite32(boot + 67, serial);
	memcpy(boot + 71, name, 11);
	memcpy(boot + 82, "FAT32   ", 8);
	fatWrite16(boot + 510, 0xaa55);

	auto info = boot + G_BLOCK_SECTOR_SIZE;
	fatWrite32(info, 0x41615252);
	fatWrite32(info + 484, 0x61417272);
	fatWrite32(info + 488, UINT32_MAX);
	fatWrite32(info + 492, UINT32_MAX);
	fatWrite32(info + 508, 0xaa550000);
}

/**
 * Flush the allocation tables, FSInfo and backup boot sector before publishing
 * the primary boot sector. A failure must not expose a partly initialized FAT.
 */
static bool fatFormatPublishBoot(g_fat_device* device, uint64_t first, const uint8_t* bytes)
{
	return fatFormatTransfer(device, first + G_FAT_FORMAT_INFO_SECTOR, bytes + G_BLOCK_SECTOR_SIZE, 1) &&
		   fatFormatTransfer(device, first + G_FAT_FORMAT_BACKUP_SECTOR, bytes, 2) &&
		   blockDeviceFlush(device->handler, device->id) == G_BLOCK_SUCCESS &&
		   fatFormatTransfer(device, first, bytes, 1) &&
		   blockDeviceFlush(device->handler, device->id) == G_BLOCK_SUCCESS;
}

g_volume_status fatFormatVolume(g_fat_device* device, uint64_t first, uint64_t count, uint32_t serial,
								const char* label, g_fat_format_progress progress, void* context)
{
	if(!serial || !label || strlen(label) > 11 || !count || first >= device->info.sectorCount ||
	   count > device->info.sectorCount - first || first > UINT32_MAX || count > UINT32_MAX)
		return G_VOLUME_INVALID;
	char name[11];
	if(!fatFormatEncodeLabel(label, name))
		return G_VOLUME_INVALID;
	if(device->info.sectorSize != G_BLOCK_SECTOR_SIZE || !device->info.maxTransferSectors ||
	   (device->info.flags & (G_BLOCK_WRITABLE | G_BLOCK_FLUSH_SUPPORTED)) !=
			   (G_BLOCK_WRITABLE | G_BLOCK_FLUSH_SUPPORTED))
		return G_VOLUME_UNSUPPORTED;
	if(!device->buffer.data || device->buffer.size < G_BLOCK_SECTOR_SIZE)
		return G_VOLUME_IO_ERROR;
	g_fat_format_layout layout;
	if(!fatFormatChooseLayout(count, &layout))
		return G_VOLUME_INVALID;

	uint8_t bytes[G_BLOCK_DMA_BUFFER_BYTES]{};
	if(!fatFormatClearMetadata(device, first, layout, bytes, progress, context) ||
	   !fatFormatInitializeTables(device, first, layout, bytes))
		return G_VOLUME_IO_ERROR;
	memset(bytes, 0, sizeof(bytes));
	if(*label)
	{
		memcpy(bytes, name, 11);
		bytes[11] = G_FAT_ATTRIBUTE_VOLUME;
		if(!fatFormatTransfer(device, first + layout.dataSector, bytes, 1))
			return G_VOLUME_IO_ERROR;
		memset(bytes, 0, G_BLOCK_SECTOR_SIZE);
	}
	fatFormatBuildBoot(bytes, layout, first, count, serial, name);
	return fatFormatPublishBoot(device, first, bytes) ? G_VOLUME_OK : G_VOLUME_IO_ERROR;
}
