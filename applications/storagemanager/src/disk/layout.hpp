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

#ifndef STORAGEMANAGER_DISK_LAYOUT
#define STORAGEMANAGER_DISK_LAYOUT

#include <stdint.h>

#define G_VOLUME_SECTOR_SIZE (512)
#define G_VOLUME_MAX_TRANSFER_SECTORS (8)
#define G_DISK_MBR_PARTITION_COUNT (4)
#define G_DISK_MBR_MAX_LOGICAL_PARTITIONS (128)
#define G_DISK_MBR_PARTITION_TABLE_OFFSET (446)
#define G_DISK_MBR_PARTITION_ENTRY_SIZE (16)
#define G_DISK_MBR_IDENTITY_OFFSET (440)
#define G_DISK_GPT_PARTITION_ENTRY_SIZE (128)
#define G_DISK_GPT_MAX_PARTITIONS (128)
#define G_DISK_GPT_ENTRIES_PER_SECTOR (G_VOLUME_SECTOR_SIZE / G_DISK_GPT_PARTITION_ENTRY_SIZE)

/**
 * Returns the zero-based MBR partition entry in sector; index must be below four.
 */
static inline const uint8_t* diskLayoutGetMbrEntry(const uint8_t* sector, unsigned index)
{
	return sector + G_DISK_MBR_PARTITION_TABLE_OFFSET + index * G_DISK_MBR_PARTITION_ENTRY_SIZE;
}

/**
 * Returns a writable MBR partition entry; index is zero-based and below four.
 */
static inline uint8_t* diskLayoutGetMutableMbrEntry(uint8_t* sector, unsigned index)
{
	return sector + G_DISK_MBR_PARTITION_TABLE_OFFSET + index * G_DISK_MBR_PARTITION_ENTRY_SIZE;
}

/**
 * Checks for an MBR entry of type 0xee, which signals a protective GPT layout
 * without validating the GPT header or entries.
 */
static inline bool diskLayoutHasProtectiveMbrEntry(const uint8_t* sector)
{
	for(unsigned i = 0; i < G_DISK_MBR_PARTITION_COUNT; ++i)
		if(diskLayoutGetMbrEntry(sector, i)[4] == 0xee)
			return true;

	return false;
}

/**
 * Decodes an unsigned 32-bit little-endian value from bytes.
 */
static inline uint32_t diskLayoutReadLe32(const uint8_t* bytes)
{
	return (uint32_t) (bytes[0]) | ((uint32_t) (bytes[1]) << 8) | ((uint32_t) (bytes[2]) << 16) |
		   ((uint32_t) (bytes[3]) << 24);
}

/**
 * Decodes an unsigned 64-bit little-endian value from bytes.
 */
static inline uint64_t diskLayoutReadLe64(const uint8_t* bytes)
{
	return diskLayoutReadLe32(bytes) | ((uint64_t) diskLayoutReadLe32(bytes + 4) << 32);
}

/**
 * Encodes value as four little-endian bytes.
 */
static inline void diskLayoutWriteLe32(uint8_t* bytes, uint32_t value)
{
	for(unsigned i = 0; i < 4; ++i)
		bytes[i] = value >> (i * 8);
}

/**
 * Checks bytes 510 and 511 for the boot-sector signature 0x55, 0xaa.
 */
static inline bool diskLayoutHasBootSignature(const uint8_t* sector) { return sector[510] == 0x55 && sector[511] == 0xaa; }

/**
 * Recognize the FAT boot-sector geometry before interpreting its overlapping
 * bytes as an MBR partition table. This does not validate the filesystem.
 */
static inline bool diskLayoutHasFatGeometry(const uint8_t* sector)
{
	if(sector[0] != 0xeb && sector[0] != 0xe9)
		return false;

	uint16_t bytesPerSector = (uint16_t) (sector[11]) | ((uint16_t) (sector[12]) << 8);
	uint8_t sectorsPerCluster = sector[13];
	uint16_t reservedSectors = (uint16_t) (sector[14]) | ((uint16_t) (sector[15]) << 8);
	uint8_t fatCount = sector[16];

	if(bytesPerSector != G_VOLUME_SECTOR_SIZE || !sectorsPerCluster || !reservedSectors)
		return false;
	if(sectorsPerCluster & (sectorsPerCluster - 1))
		return false;

	return fatCount == 1 || fatCount == 2;
}

/**
 * Identifies the MBR extended-partition types 0x05, 0x0f and 0x85.
 */
static inline bool diskLayoutIsExtendedMbrType(uint32_t type) { return type == 0x05 || type == 0x0f || type == 0x85; }

#endif
