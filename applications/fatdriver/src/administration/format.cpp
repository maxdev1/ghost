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
	memset(name, ' ', G_FAT_VOLUME_LABEL_SIZE);
	if(!*label)
		memcpy(name, G_FAT_DEFAULT_VOLUME_LABEL, sizeof(G_FAT_DEFAULT_VOLUME_LABEL) - 1);
	for(size_t i = 0; label[i]; ++i)
	{
		unsigned char c = label[i];
		if(c < G_FAT_ASCII_PRINTABLE_MIN || c > G_FAT_ASCII_PRINTABLE_MAX || strchr("\"*+,./:;<=>?[\\]|", c))
			return false;

		name[i] = c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
	}

	return true;
}

static bool fatFormatChooseLayout(uint64_t count, g_fat_format_layout* layout)
{
	if(count < G_FAT32_MIN_DATA_CLUSTERS + G_FAT_FORMAT_RESERVED_SECTORS)
		return false;

	uint32_t sectorsPerCluster = 1;
	while(count / sectorsPerCluster > G_FAT_FORMAT_TARGET_CLUSTERS && sectorsPerCluster < G_FAT_MAX_SECTORS_PER_CLUSTER)
		sectorsPerCluster *= 2;
	const auto entriesPerSector = G_BLOCK_SECTOR_SIZE / sizeof(uint32_t);
	layout->sectorsPerCluster = sectorsPerCluster;
	layout->sectorsPerFat =
			(count - G_FAT_FORMAT_RESERVED_SECTORS + G_FAT_RESERVED_ENTRY_COUNT * sectorsPerCluster +
			 entriesPerSector * sectorsPerCluster + 1) /
			(entriesPerSector * sectorsPerCluster + G_FAT_FORMAT_TABLE_COUNT);
	layout->dataSector = G_FAT_FORMAT_RESERVED_SECTORS + uint64_t(G_FAT_FORMAT_TABLE_COUNT) * layout->sectorsPerFat;
	if(layout->dataSector + sectorsPerCluster > count)
		return false;

	auto clusters = (count - layout->dataSector) / sectorsPerCluster;
	return clusters >= G_FAT32_MIN_DATA_CLUSTERS && clusters <= G_FAT32_MAX_DATA_CLUSTERS;
}

/**
 * Invalidate the old primary boot sector before replacing metadata. The cleared
 * area includes both FATs and the root cluster, never the rest of the volume.
 */
static bool fatFormatClearMetadata(g_fat_device* device, uint64_t first, const g_fat_format_layout& layout,
								   const uint8_t* zero, g_fat_format_progress progress, void* context)
{
	if(!fatFormatTransfer(device, first, zero, 1))
		return false;
	if(blockDeviceFlush(device->handler, device->id) != G_BLOCK_SUCCESS)
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
	fatWrite32(bytes, G_FAT32_MEDIA_ENTRY);
	fatWrite32(bytes + G_FAT32_ENTRY_SIZE, G_FAT_CLUSTER_END);
	fatWrite32(bytes + G_FAT_RESERVED_ENTRY_COUNT * G_FAT32_ENTRY_SIZE, G_FAT_CLUSTER_END);
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
	boot[G_FAT_BOOT_JUMP_OFFSET] = G_FAT_BOOT_SHORT_JUMP;
	boot[G_FAT_BOOT_JUMP_DISPLACEMENT_OFFSET] = G_FAT32_BOOT_JUMP_DISPLACEMENT;
	boot[G_FAT_BOOT_NOP_OFFSET] = G_FAT_BOOT_NOP;
	memcpy(boot + G_FAT_BOOT_OEM_NAME_OFFSET, "GHOST   ", G_FAT_BOOT_OEM_NAME_SIZE);
	fatWrite16(boot + G_FAT_BPB_BYTES_PER_SECTOR_OFFSET, G_BLOCK_SECTOR_SIZE);
	boot[G_FAT_BPB_SECTORS_PER_CLUSTER_OFFSET] = layout.sectorsPerCluster;
	fatWrite16(boot + G_FAT_BPB_RESERVED_SECTORS_OFFSET, G_FAT_FORMAT_RESERVED_SECTORS);
	boot[G_FAT_BPB_TABLE_COUNT_OFFSET] = G_FAT_FORMAT_TABLE_COUNT;
	boot[G_FAT_BPB_MEDIA_OFFSET] = G_FAT_MEDIA_FIXED_DISK;
	fatWrite16(boot + G_FAT_BPB_SECTORS_PER_TRACK_OFFSET, G_FAT_FORMAT_SECTORS_PER_TRACK);
	fatWrite16(boot + G_FAT_BPB_HEADS_OFFSET, G_FAT_FORMAT_HEADS);
	fatWrite32(boot + G_FAT_BPB_HIDDEN_SECTORS_OFFSET, first);
	fatWrite32(boot + G_FAT_BPB_SECTOR_COUNT32_OFFSET, count);
	fatWrite32(boot + G_FAT_BPB_SECTORS_PER_FAT32_OFFSET, layout.sectorsPerFat);
	fatWrite32(boot + G_FAT_BPB_ROOT_CLUSTER_OFFSET, G_FAT_FIRST_DATA_CLUSTER);
	fatWrite16(boot + G_FAT_BPB_FS_INFO_OFFSET, G_FAT_FORMAT_INFO_SECTOR);
	fatWrite16(boot + G_FAT_BPB_BACKUP_BOOT_OFFSET, G_FAT_FORMAT_BACKUP_SECTOR);
	boot[G_FAT32_BOOT_DRIVE_OFFSET] = G_FAT_BIOS_FIXED_DISK_DRIVE;
	boot[G_FAT32_BOOT_EXTENDED_SIGNATURE_OFFSET] = G_FAT_EXTENDED_BOOT_SIGNATURE;
	fatWrite32(boot + G_FAT32_BOOT_SERIAL_OFFSET, serial);
	memcpy(boot + G_FAT32_BOOT_LABEL_OFFSET, name, G_FAT_VOLUME_LABEL_SIZE);
	memcpy(boot + G_FAT32_BOOT_TYPE_OFFSET, "FAT32   ", G_FAT_BOOT_TYPE_SIZE);
	fatWrite16(boot + G_FAT_BOOT_SIGNATURE_OFFSET, G_FAT_BOOT_SIGNATURE);

	auto info = boot + G_BLOCK_SECTOR_SIZE;
	fatWrite32(info + G_FAT_FS_INFO_LEAD_SIGNATURE_OFFSET, G_FAT_FS_INFO_LEAD_SIGNATURE);
	fatWrite32(info + G_FAT_FS_INFO_STRUCTURE_SIGNATURE_OFFSET, G_FAT_FS_INFO_STRUCTURE_SIGNATURE);
	fatWrite32(info + G_FAT_FS_INFO_FREE_COUNT_OFFSET, G_FAT_ALLOCATION_HINT_UNKNOWN);
	fatWrite32(info + G_FAT_FS_INFO_NEXT_FREE_OFFSET, G_FAT_ALLOCATION_HINT_UNKNOWN);
	fatWrite32(info + G_FAT_FS_INFO_TRAIL_SIGNATURE_OFFSET, G_FAT_FS_INFO_TRAIL_SIGNATURE);
}

/**
 * Flush the allocation tables, FSInfo and backup boot sector before publishing
 * the primary boot sector. A failure must not expose a partly initialized FAT.
 */
static bool fatFormatPublishBoot(g_fat_device* device, uint64_t first, const uint8_t* bytes)
{
	if(!fatFormatTransfer(device, first + G_FAT_FORMAT_INFO_SECTOR, bytes + G_BLOCK_SECTOR_SIZE, 1))
		return false;
	if(!fatFormatTransfer(device, first + G_FAT_FORMAT_BACKUP_SECTOR, bytes, G_FAT_FORMAT_BACKUP_SECTOR_COUNT))
		return false;
	if(blockDeviceFlush(device->handler, device->id) != G_BLOCK_SUCCESS)
		return false;
	if(!fatFormatTransfer(device, first, bytes, 1))
		return false;

	return blockDeviceFlush(device->handler, device->id) == G_BLOCK_SUCCESS;
}

g_volume_status fatFormatVolume(g_fat_device* device, uint64_t first, uint64_t count, uint32_t serial,
								const char* label, g_fat_format_progress progress, void* context)
{
	if(!serial || !label || strlen(label) > G_FAT_VOLUME_LABEL_SIZE || !count || first >= device->info.sectorCount ||
	   count > device->info.sectorCount - first || first > UINT32_MAX || count > UINT32_MAX)
		return G_VOLUME_INVALID;

	char name[G_FAT_VOLUME_LABEL_SIZE];
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
	if(!fatFormatClearMetadata(device, first, layout, bytes, progress, context))
		return G_VOLUME_IO_ERROR;
	if(!fatFormatInitializeTables(device, first, layout, bytes))
		return G_VOLUME_IO_ERROR;

	memset(bytes, 0, sizeof(bytes));
	if(*label)
	{
		memcpy(bytes, name, G_FAT_VOLUME_LABEL_SIZE);
		bytes[G_FAT_ENTRY_ATTRIBUTES_OFFSET] = G_FAT_ATTRIBUTE_VOLUME;
		if(!fatFormatTransfer(device, first + layout.dataSector, bytes, 1))
			return G_VOLUME_IO_ERROR;

		memset(bytes, 0, G_BLOCK_SECTOR_SIZE);
	}

	fatFormatBuildBoot(bytes, layout, first, count, serial, name);
	return fatFormatPublishBoot(device, first, bytes) ? G_VOLUME_OK : G_VOLUME_IO_ERROR;
}
