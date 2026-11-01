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

#ifndef FATDRIVER
#define FATDRIVER

#include <ghost/filesystem.h>
#include <ghost/messages.h>
#include <libblockdevice/driver.hpp>
#include "disk.hpp"
#include <libstorage/volume.hpp>

constexpr g_fs_phys_id G_FAT_ROOT = 0;
constexpr const char* G_FAT_DRIVER_NAME = "fatdriver";
constexpr size_t G_FAT_SECTOR_CACHE_SIZE = 64;
constexpr size_t G_FAT_METADATA_CACHE_SIZE = 128;
constexpr size_t G_FAT_CHAIN_CACHE_SIZE = 32;
constexpr size_t G_FAT_MAX_INODES = 16384;

struct g_fat_inode;

struct g_fat_device
{
	g_device_id id = 0;
	g_tid handler = G_TID_NONE;
	g_block_device_info info{};
	g_block_buffer buffer;
	g_block_status status = G_BLOCK_SUCCESS;
	uint64_t firstSector = 0;
	uint64_t sectorCount = 0;
};

struct g_fat_volume
{
	uint64_t firstSector = 0;
	uint32_t sectorCount = 0;
	uint32_t clusterCount = 0;
	uint32_t sectorsPerFat = 0;
	uint32_t rootCluster = 0;
	uint32_t rootSector = 0;
	uint32_t rootSectors = 0;
	uint32_t dataSector = 0;
	uint16_t reservedSectors = 0;
	uint16_t rootEntries = 0;
	uint8_t sectorsPerCluster = 0;
	uint8_t fatCount = 0;
	uint8_t fatBits = 0;
	uint8_t activeFat = 0;
	bool mirrored = true;
	uint16_t fsInfoSector = 0xffff;
	uint16_t backupBootSector = 0xffff;
};

struct g_fat_sector_cache
{
	uint64_t stamp = 0;
	uint32_t sector = 0;
	uint8_t bytes[G_BLOCK_SECTOR_SIZE]{};
};

struct g_fat_chain_cache
{
	uint64_t stamp = 0;
	uint32_t first = 0;
	uint32_t count = 0;
	uint32_t current = 0;
	uint32_t index = 0;
	uint32_t last = 0;
};

/**
 * One serving thread owns each mount, its DMA window, caches and inode list.
 * A failed write freezes further mutation because completion may be uncertain.
 * Dirty payload may wait for fsync; allocation and directory publication use
 * explicit flushes to preserve their ordering on disk.
 */
struct g_fat_mount
{
	g_volume_info source;
	uint64_t claim = 0;
	g_fat_device device;
	g_fat_volume volume;
	g_fs_virt_id id = G_FS_VIRT_ID_NONE;
	g_fs_delegate_transaction_storage* storage = nullptr;
	uint64_t cacheClock = 0;
	g_fat_sector_cache sectors[G_FAT_SECTOR_CACHE_SIZE];
	size_t tableCacheIndex = 0;
	uint32_t freeClusters = UINT32_MAX;
	g_fat_sector_cache metadata[G_FAT_METADATA_CACHE_SIZE];
	g_fat_chain_cache chains[G_FAT_CHAIN_CACHE_SIZE];
	g_fat_inode* inodes = nullptr;
	uint64_t nextInode = 1;
	uint32_t inodeCount = 0;
	bool failed = false;
	bool dirty = true;
	bool allocationHintsInvalid = false;
	bool bootRoot = false;
	bool probeOnly = false;
	uint32_t nextFreeCluster = 2;
};

enum g_fat_result
{
	G_FAT_SUCCESS,
	G_FAT_END,
	G_FAT_ERROR
};

struct g_fat_node
{
	g_fs_phys_id id = G_FAT_ROOT;
	uint32_t firstCluster = 0;
	uint32_t size = 0;
	bool directory = false;
};

struct g_fat_inode
{
	g_fat_inode* next = nullptr;
	g_fat_node node;
	g_fs_phys_id parent = G_FAT_ROOT;
	uint64_t offset = 0;
	uint64_t slots[G_FAT_LONG_NAME_MAX_ENTRIES + 1]{};
	uint32_t opens = 0;
	uint8_t slotCount = 0;
	uint8_t attributes = 0;
	bool unlinked = false;
	bool valid = false;
};

struct g_fat_long_name
{
	uint16_t units[G_FAT_LONG_NAME_MAX_ENTRIES * G_FAT_LONG_NAME_UNITS_PER_ENTRY]{};
	uint8_t expected = 0;
	uint8_t checksum = 0;
	uint16_t length = 0;
	bool active = false;
};

struct g_fat_directory
{
	g_fat_node node;
	g_fat_long_name longName;
	uint32_t cluster = 0;
	uint32_t clustersRemaining = 0;
	uint32_t sectorInCluster = 0;
	uint32_t fixedSector = 0;
	uint64_t sectorOffset = 0;
	uint16_t position = G_BLOCK_SECTOR_SIZE;
	bool ended = false;
	uint64_t longOffsets[G_FAT_LONG_NAME_MAX_ENTRIES]{};
	uint8_t bytes[G_BLOCK_SECTOR_SIZE]{};
};

struct g_fat_directory_entry
{
	g_fat_node node;
	char name[G_FILENAME_MAX]{};
	char alias[40]{};
	uint8_t shortRaw[G_FAT_SHORT_NAME_SIZE]{};
};

bool fatDeviceOpen(g_fat_device* device);
bool fatDeviceClose(g_fat_device* device);

bool fatVolumeParse(const uint8_t* sector, uint64_t first, uint64_t available, g_fat_volume* volume);
bool fatVolumeProbe(g_fat_device* device, g_fat_volume* volume);

/**
 * Reads a bounded byte range through the device's reusable shared DMA window.
 * Only the owning serving thread accesses that window or issues its block calls.
 */
bool fatVolumeRead(g_fat_device* device, const g_fat_volume& volume,
	uint64_t offset, void* target, size_t length);
bool fatVolumeWrite(g_fat_mount* mount, uint64_t offset, const void* source, size_t length);
bool fatVolumeFlush(g_fat_mount* mount);
bool fatMetadataRead(g_fat_mount* mount, uint64_t offset, void* target, size_t length);
bool fatVolumeWritable(const g_fat_mount* mount);
bool fatVolumeStat(g_fat_mount* mount, g_volume_filesystem_info* info,
	g_volume_progress_callback progress = nullptr, void* context = nullptr);
void fatCacheInvalidate(g_fat_mount* mount);

g_fat_inode* fatInodeFind(g_fat_mount* mount, g_fs_phys_id id);
bool fatInodeObserve(g_fat_mount* mount, g_fat_node* node, g_fs_phys_id parent,
	uint64_t offset, const uint64_t* longOffsets, uint8_t longCount, uint8_t attributes);
void fatMountRelease(g_fat_mount* mount);

g_fs_mutation_status fatFileResize(g_fat_mount* mount, g_fs_phys_id id, uint64_t length);
g_fs_write_status fatFileWrite(g_fat_mount* mount, g_fs_phys_id id, uint64_t offset,
	const void* source, size_t length, size_t* written);
g_fs_mutation_status fatDirectoryCreate(g_fat_mount* mount, g_fs_phys_id parent, const char* name,
	bool directory, g_fat_directory_entry* created);
g_fs_mutation_status fatNodeRemove(g_fat_mount* mount, g_fs_phys_id id);
g_fs_mutation_status fatInodeClose(g_fat_mount* mount, g_fs_phys_id id);

bool fatClusterValid(const g_fat_volume& volume, uint32_t cluster);
uint64_t fatClusterOffset(const g_fat_volume& volume, uint32_t cluster);
g_fat_result fatClusterNext(g_fat_mount* mount, uint32_t cluster, uint32_t* next);
g_fat_chain_cache* fatChainGet(g_fat_mount* mount, uint32_t first);
g_fat_chain_cache* fatChainRemember(g_fat_mount* mount, uint32_t first, uint32_t count, uint32_t last);
bool fatChainSeek(g_fat_mount* mount, g_fat_chain_cache* chain, uint32_t index, uint32_t* cluster);

/**
 * Resolves the first cluster and byte count for a contiguous disk run, bounded
 * by limit bytes. Both file reads and writes use the same validated chain cursor.
 */
bool fatChainGetRun(g_fat_mount* mount, g_fat_chain_cache* chain, uint64_t offset, uint64_t limit,
	uint32_t* cluster, uint64_t* count);

bool fatNodeDecode(const g_fat_volume& volume, uint64_t offset, const uint8_t* raw, g_fat_node* node);
bool fatNodeRead(g_fat_mount* mount, g_fs_phys_id id, g_fat_node* node);
bool fatFileRead(g_fat_mount* mount, const g_fat_node& node, uint64_t offset,
	void* target, size_t length, size_t* read);

bool fatShortNameDecode(const uint8_t* raw, char* name);
void fatLongNameAppend(g_fat_long_name* name, const uint8_t* raw);
bool fatLongNameDecode(const g_fat_long_name& name, const uint8_t* raw, char* text);
bool fatNameEquals(const char* left, const char* right);
bool fatNameEncode(const char* text, uint16_t* units, size_t* count);

bool fatDirectoryOpen(g_fat_mount* mount, const g_fat_node& node, g_fat_directory* directory);
g_fat_result fatDirectoryNext(g_fat_mount* mount, g_fat_directory* directory, g_fat_directory_entry* entry);

bool fatMountHandleRequest(g_fat_mount* mount, const g_message_header* message, const void* content);

using g_fat_format_progress = void(*)(void*, uint64_t, uint64_t);
g_volume_status fatFormatVolume(g_fat_device* device, uint64_t first, uint64_t count, uint32_t serial,
	const char* label, g_fat_format_progress progress = nullptr, void* context = nullptr);
g_volume_status fatMountStart(g_fat_mount* mount, const char* requestedName = nullptr);

#endif
