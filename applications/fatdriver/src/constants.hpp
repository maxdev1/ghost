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

#ifndef FATDRIVER_CONSTANTS
#define FATDRIVER_CONSTANTS

#include <cstddef>
#include <cstdint>

/**
 * FAT directory records, short aliases and long-name continuation records.
 * Offsets are bytes from the beginning of one directory entry. The minimum
 * date encodes 1980-01-01 in the FAT calendar format.
 */
constexpr unsigned G_FAT_DIRECTORY_ENTRY_SIZE = 32;
constexpr unsigned G_FAT_SHORT_NAME_SIZE = 11;
constexpr unsigned G_FAT_LONG_NAME_UNITS_PER_ENTRY = 13;
constexpr unsigned G_FAT_LONG_NAME_MAX_ENTRIES = 20;
constexpr uint8_t G_FAT_ATTRIBUTE_READ_ONLY = 0x01;
constexpr uint8_t G_FAT_ATTRIBUTE_VOLUME = 0x08;
constexpr uint8_t G_FAT_ATTRIBUTE_DIRECTORY = 0x10;
constexpr uint8_t G_FAT_ATTRIBUTE_ARCHIVE = 0x20;
constexpr uint8_t G_FAT_ATTRIBUTE_LONG_NAME = 0x0f;
constexpr uint8_t G_FAT_ENTRY_DELETED = 0xe5;
constexpr unsigned G_FAT_LONG_NAME_OFFSETS[] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};

constexpr unsigned G_FAT_SHORT_BASE_SIZE = 8;
constexpr unsigned G_FAT_SHORT_EXTENSION_SIZE = 3;
constexpr unsigned G_FAT_SHORT_NAME_BUFFER_SIZE = 40;
constexpr unsigned G_FAT_VOLUME_LABEL_SIZE = 11;
constexpr unsigned G_FAT_LONG_NAME_MAX_UNITS = 255;
constexpr unsigned G_FAT_ALIAS_MAX_SERIAL = 999999;
constexpr unsigned G_FAT_DIRECTORY_GROW_ATTEMPTS = 2;
constexpr unsigned G_FAT_DOT_ENTRY_COUNT = 2;
constexpr uint8_t G_FAT_ENTRY_ESCAPED_DELETED = 0x05;
constexpr uint8_t G_FAT_ENTRY_END = 0;
constexpr unsigned G_FAT_ENTRY_ATTRIBUTES_OFFSET = 11;
constexpr unsigned G_FAT_ENTRY_CASE_FLAGS_OFFSET = 12;
constexpr unsigned G_FAT_ENTRY_CREATED_DATE_OFFSET = 16;
constexpr unsigned G_FAT_ENTRY_ACCESSED_DATE_OFFSET = 18;
constexpr unsigned G_FAT_ENTRY_CLUSTER_HIGH_OFFSET = 20;
constexpr unsigned G_FAT_ENTRY_MODIFIED_DATE_OFFSET = 24;
constexpr unsigned G_FAT_ENTRY_CLUSTER_LOW_OFFSET = 26;
constexpr unsigned G_FAT_ENTRY_FILE_SIZE_OFFSET = 28;
constexpr unsigned G_FAT_ENTRY_CLUSTER_HIGH_SHIFT = 16;
constexpr uint16_t G_FAT_ENTRY_CLUSTER_HIGH_MASK = 0x0fff;
constexpr uint16_t G_FAT_DATE_MINIMUM = 0x21;
constexpr uint8_t G_FAT_SHORT_BASE_LOWER_CASE = 0x08;
constexpr uint8_t G_FAT_SHORT_EXTENSION_LOWER_CASE = 0x10;
constexpr uint8_t G_FAT_LONG_NAME_ATTRIBUTE_MASK = 0x3f;
constexpr uint8_t G_FAT_LONG_NAME_ORDINAL_MASK = 0x1f;
constexpr uint8_t G_FAT_LONG_NAME_INVALID_ORDINAL_MASK = 0xa0;
constexpr uint8_t G_FAT_LONG_NAME_LAST_FLAG = 0x40;
constexpr unsigned G_FAT_LONG_NAME_TYPE_OFFSET = 12;
constexpr unsigned G_FAT_LONG_NAME_CHECKSUM_OFFSET = 13;
constexpr unsigned G_FAT_LONG_NAME_CLUSTER_OFFSET = 26;
constexpr uint16_t G_FAT_LONG_NAME_PADDING = 0xffff;
constexpr uint8_t G_FAT_LONG_NAME_PADDING_BYTE = 0xff;

/**
 * Boot-sector and BIOS parameter block fields. Offsets are bytes from the
 * beginning of the volume boot sector, including the FAT32 extended BPB.
 */
constexpr unsigned G_FAT_BOOT_JUMP_OFFSET = 0;
constexpr unsigned G_FAT_BOOT_JUMP_DISPLACEMENT_OFFSET = 1;
constexpr unsigned G_FAT_BOOT_NOP_OFFSET = 2;
constexpr uint8_t G_FAT_BOOT_SHORT_JUMP = 0xeb;
constexpr uint8_t G_FAT_BOOT_NEAR_JUMP = 0xe9;
constexpr uint8_t G_FAT_BOOT_NOP = 0x90;
constexpr uint8_t G_FAT32_BOOT_JUMP_DISPLACEMENT = 0x58;
constexpr unsigned G_FAT_BOOT_OEM_NAME_OFFSET = 3;
constexpr unsigned G_FAT_BOOT_OEM_NAME_SIZE = 8;
constexpr unsigned G_FAT_BPB_BYTES_PER_SECTOR_OFFSET = 11;
constexpr unsigned G_FAT_BPB_SECTORS_PER_CLUSTER_OFFSET = 13;
constexpr unsigned G_FAT_BPB_RESERVED_SECTORS_OFFSET = 14;
constexpr unsigned G_FAT_BPB_TABLE_COUNT_OFFSET = 16;
constexpr unsigned G_FAT_BPB_ROOT_ENTRIES_OFFSET = 17;
constexpr unsigned G_FAT_BPB_SECTOR_COUNT16_OFFSET = 19;
constexpr unsigned G_FAT_BPB_MEDIA_OFFSET = 21;
constexpr unsigned G_FAT_BPB_SECTORS_PER_FAT16_OFFSET = 22;
constexpr unsigned G_FAT_BPB_SECTORS_PER_TRACK_OFFSET = 24;
constexpr unsigned G_FAT_BPB_HEADS_OFFSET = 26;
constexpr unsigned G_FAT_BPB_HIDDEN_SECTORS_OFFSET = 28;
constexpr unsigned G_FAT_BPB_SECTOR_COUNT32_OFFSET = 32;
constexpr unsigned G_FAT_BPB_SECTORS_PER_FAT32_OFFSET = 36;
constexpr unsigned G_FAT_BPB_FLAGS_OFFSET = 40;
constexpr unsigned G_FAT_BPB_VERSION_OFFSET = 42;
constexpr unsigned G_FAT_BPB_ROOT_CLUSTER_OFFSET = 44;
constexpr unsigned G_FAT_BPB_FS_INFO_OFFSET = 48;
constexpr unsigned G_FAT_BPB_BACKUP_BOOT_OFFSET = 50;
constexpr unsigned G_FAT32_BOOT_DRIVE_OFFSET = 64;
constexpr unsigned G_FAT32_BOOT_EXTENDED_SIGNATURE_OFFSET = 66;
constexpr unsigned G_FAT32_BOOT_SERIAL_OFFSET = 67;
constexpr unsigned G_FAT32_BOOT_LABEL_OFFSET = 71;
constexpr unsigned G_FAT32_BOOT_TYPE_OFFSET = 82;
constexpr unsigned G_FAT_BOOT_TYPE_SIZE = 8;
constexpr unsigned G_FAT_BOOT_SIGNATURE_OFFSET = 510;
constexpr uint16_t G_FAT_BOOT_SIGNATURE = 0xaa55;
constexpr uint8_t G_FAT_BPB_NO_MIRRORING = 0x80;
constexpr uint8_t G_FAT_BPB_ACTIVE_TABLE_MASK = 0x0f;
constexpr uint16_t G_FAT_NO_RESERVED_SECTOR = 0xffff;
constexpr unsigned G_FAT_MBR_DISK_SIGNATURE_OFFSET = 440;

/**
 * FAT32 FSInfo signatures and advisory allocation fields. Offsets are bytes
 * from the beginning of the FSInfo sector; unknown hints are all one bits.
 */
constexpr unsigned G_FAT_FS_INFO_LEAD_SIGNATURE_OFFSET = 0;
constexpr unsigned G_FAT_FS_INFO_STRUCTURE_SIGNATURE_OFFSET = 484;
constexpr unsigned G_FAT_FS_INFO_FREE_COUNT_OFFSET = 488;
constexpr unsigned G_FAT_FS_INFO_NEXT_FREE_OFFSET = 492;
constexpr unsigned G_FAT_FS_INFO_TRAIL_SIGNATURE_OFFSET = 508;
constexpr uint32_t G_FAT_FS_INFO_LEAD_SIGNATURE = 0x41615252;
constexpr uint32_t G_FAT_FS_INFO_STRUCTURE_SIGNATURE = 0x61417272;
constexpr uint32_t G_FAT_FS_INFO_TRAIL_SIGNATURE = 0xaa550000;
constexpr uint32_t G_FAT_ALLOCATION_HINT_UNKNOWN = UINT32_MAX;

/**
 * Allocation-table widths, valid data-cluster ranges and reserved markers.
 * The canonical end marker is masked to the selected FAT entry width on write.
 */
constexpr unsigned G_FAT_BITS_PER_BYTE = 8;
constexpr unsigned G_FAT_NIBBLE_BITS = 4;
constexpr unsigned G_FAT12_ENTRY_BITS = 12;
constexpr unsigned G_FAT12_ENTRIES_PER_PAIR = 2;
constexpr unsigned G_FAT16_ENTRY_BITS = 16;
constexpr unsigned G_FAT32_ENTRY_BITS = 32;
constexpr unsigned G_FAT16_ENTRY_SIZE = sizeof(uint16_t);
constexpr unsigned G_FAT32_ENTRY_SIZE = sizeof(uint32_t);
constexpr uint32_t G_FAT_FIRST_DATA_CLUSTER = 2;
constexpr uint32_t G_FAT_RESERVED_ENTRY_COUNT = 2;
constexpr uint32_t G_FAT16_MIN_DATA_CLUSTERS = 4085;
constexpr uint32_t G_FAT32_MIN_DATA_CLUSTERS = 65525;
constexpr uint32_t G_FAT32_MAX_DATA_CLUSTERS = 0x0ffffff3;
constexpr uint32_t G_FAT12_ENTRY_MASK = 0x0fff;
constexpr uint32_t G_FAT16_ENTRY_MASK = 0xffff;
constexpr uint32_t G_FAT32_ENTRY_MASK = 0x0fffffff;
constexpr uint32_t G_FAT32_RESERVED_BITS_MASK = 0xf0000000;
constexpr uint16_t G_FAT12_LOW_NIBBLE_MASK = 0x000f;
constexpr uint16_t G_FAT12_HIGH_NIBBLE_MASK = 0xf000;
constexpr uint32_t G_FAT12_CLUSTER_RESERVED_MIN = 0x0ff0;
constexpr uint32_t G_FAT16_CLUSTER_RESERVED_MIN = 0xfff0;
constexpr uint32_t G_FAT32_CLUSTER_RESERVED_MIN = 0x0ffffff0;
constexpr uint32_t G_FAT12_CLUSTER_END_MIN = 0x0ff8;
constexpr uint32_t G_FAT16_CLUSTER_END_MIN = 0xfff8;
constexpr uint32_t G_FAT32_CLUSTER_END_MIN = 0x0ffffff8;
constexpr uint32_t G_FAT_CLUSTER_END = 0x0fffffff;
constexpr uint32_t G_FAT_FILE_SIZE_MAX = UINT32_MAX;
constexpr unsigned G_FAT_MAX_SECTORS_PER_CLUSTER = 128;
constexpr unsigned G_FAT_MAX_TABLE_COUNT = 2;

/**
 * Formatter defaults and legacy BIOS geometry fields.
 */
constexpr uint32_t G_FAT_FORMAT_RESERVED_SECTORS = 32;
constexpr uint32_t G_FAT_FORMAT_TABLE_COUNT = 2;
constexpr uint32_t G_FAT_FORMAT_INFO_SECTOR = 1;
constexpr uint32_t G_FAT_FORMAT_BACKUP_SECTOR = 6;
constexpr uint32_t G_FAT_FORMAT_BACKUP_SECTOR_COUNT = 2;
constexpr uint32_t G_FAT_FORMAT_TARGET_CLUSTERS = 262144;
constexpr uint16_t G_FAT_FORMAT_SECTORS_PER_TRACK = 63;
constexpr uint16_t G_FAT_FORMAT_HEADS = 255;
constexpr uint8_t G_FAT_MEDIA_FIXED_DISK = 0xf8;
constexpr uint32_t G_FAT32_MEDIA_ENTRY = 0x0fffff00 | G_FAT_MEDIA_FIXED_DISK;
constexpr uint8_t G_FAT_BIOS_FIXED_DISK_DRIVE = 0x80;
constexpr uint8_t G_FAT_EXTENDED_BOOT_SIGNATURE = 0x29;
constexpr char G_FAT_DEFAULT_VOLUME_LABEL[] = "NO NAME";

/**
 * Filename encoding boundaries for UTF-8, UTF-16 and the OEM upper half.
 */
constexpr unsigned G_FAT_ASCII_PRINTABLE_MIN = 0x20;
constexpr unsigned G_FAT_ASCII_PRINTABLE_MAX = 0x7e;
constexpr unsigned G_FAT_OEM_EXTENDED_FIRST = 0x80;
constexpr unsigned G_FAT_OEM_EXTENDED_COUNT = 128;
constexpr unsigned G_FAT_UTF8_MAX_BYTES = 4;
constexpr unsigned G_FAT_UTF8_LEAD_PREFIX_MASK = 0xff;
constexpr unsigned G_FAT_UTF8_TWO_BYTE_LEAD_MIN = 0xc2;
constexpr unsigned G_FAT_UTF8_TWO_BYTE_LEAD_MAX = 0xdf;
constexpr unsigned G_FAT_UTF8_THREE_BYTE_LEAD_MIN = 0xe0;
constexpr unsigned G_FAT_UTF8_THREE_BYTE_LEAD_MAX = 0xef;
constexpr unsigned G_FAT_UTF8_FOUR_BYTE_LEAD_MIN = 0xf0;
constexpr unsigned G_FAT_UTF8_FOUR_BYTE_LEAD_MAX = 0xf4;
constexpr unsigned G_FAT_UTF8_TWO_BYTE_MIN = 0x80;
constexpr unsigned G_FAT_UTF8_THREE_BYTE_MIN = 0x800;
constexpr unsigned G_FAT_UTF8_FOUR_BYTE_MIN = 0x10000;
constexpr unsigned G_FAT_UTF8_CONTINUATION_MASK = 0xc0;
constexpr unsigned G_FAT_UTF8_CONTINUATION_TAG = 0x80;
constexpr unsigned G_FAT_UTF8_CONTINUATION_PAYLOAD_MASK = 0x3f;
constexpr unsigned G_FAT_UTF8_CONTINUATION_BITS = 6;
constexpr unsigned G_FAT_UNICODE_MAX = 0x10ffff;
constexpr unsigned G_FAT_UTF16_SURROGATE_MIN = 0xd800;
constexpr unsigned G_FAT_UTF16_SURROGATE_MAX = 0xdfff;
constexpr unsigned G_FAT_UTF16_HIGH_SURROGATE_MAX = 0xdbff;
constexpr unsigned G_FAT_UTF16_LOW_SURROGATE_MIN = 0xdc00;
constexpr unsigned G_FAT_UTF16_SURROGATE_BITS = 10;
constexpr unsigned G_FAT_UTF16_SURROGATE_MASK = 0x3ff;
constexpr unsigned G_FAT_UTF16_SUPPLEMENTARY_MIN = 0x10000;

/**
 * Driver resource limits, progress cadence and boot-root selector encoding.
 */
constexpr size_t G_FAT_SECTOR_CACHE_SIZE = 64;
constexpr size_t G_FAT_METADATA_CACHE_SIZE = 128;
constexpr size_t G_FAT_CHAIN_CACHE_SIZE = 32;
constexpr size_t G_FAT_MAX_INODES = 16384;
constexpr uint32_t G_FAT_STAT_CHECK_CLUSTERS = 128;
constexpr uint32_t G_FAT_SCAN_YIELD_CLUSTERS = 4096;
constexpr uint64_t G_FAT_PROGRESS_INTERVAL_MS = 100;
constexpr uint64_t G_FAT_DISCOVERY_RETRY_MS = 100;
constexpr uint64_t G_FAT_IPC_RETRY_MS = 1;
constexpr uint64_t G_FAT_SCAN_YIELD_MS = 1;
constexpr uint64_t G_FAT_FORMAT_PROGRESS_SECTORS = 64;
constexpr unsigned G_FAT_BOOT_DISK_HEX_DIGITS = 8;
constexpr unsigned G_FAT_BOOT_ROOT_SPEC_SIZE = 17;

#endif
