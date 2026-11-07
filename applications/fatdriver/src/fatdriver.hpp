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

struct g_fat_inode;

struct g_fat_device
{
	g_device_id id = 0;
	g_tid handler = G_TID_NONE;
	g_block_device_info info{};
	g_block_buffer buffer;
	g_block_status status = G_BLOCK_SUCCESS;
	bool bufferUncertain = false;
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
	uint16_t fsInfoSector = G_FAT_NO_RESERVED_SECTOR;
	uint16_t backupBootSector = G_FAT_NO_RESERVED_SECTOR;
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
	uint32_t freeClusters = G_FAT_ALLOCATION_HINT_UNKNOWN;
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
	uint32_t nextFreeCluster = G_FAT_FIRST_DATA_CLUSTER;
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
	char alias[G_FAT_SHORT_NAME_BUFFER_SIZE]{};
	uint8_t shortRaw[G_FAT_SHORT_NAME_SIZE]{};
};

/**
 * Validates the block device and opens its shared DMA buffer.
 */
bool fatDeviceOpen(g_fat_device* device);

/**
 * Closes the device's shared DMA buffer, retrying once on failure.
 */
bool fatDeviceClose(g_fat_device* device);

/**
 * Reads into the shared buffer, blocking further transfers after uncertain I/O
 * until the buffer has been closed and opened again.
 */
bool fatDeviceReadBuffer(g_fat_device* device, uint64_t sector, uint32_t count);

/**
 * Validates a FAT boot sector and extracts the volume geometry.
 */
bool fatVolumeParse(const uint8_t* sector, uint64_t first, uint64_t available, g_fat_volume* volume);

/**
 * Reads and parses the FAT boot sector in the device's selected storage region.
 */
bool fatVolumeProbe(g_fat_device* device, g_fat_volume* volume);

/**
 * Reads a bounded volume byte range using the owning thread's shared DMA buffer.
 */
bool fatVolumeRead(g_fat_device* device, const g_fat_volume& volume,
	uint64_t offset, void* target, size_t length);

/**
 * Writes volume bytes while preserving partial sectors and updating caches.
 */
bool fatVolumeWrite(g_fat_mount* mount, uint64_t offset, const void* source, size_t length);

/**
 * Flushes pending device writes and marks the mount failed on a flush error.
 */
bool fatVolumeFlush(g_fat_mount* mount);

/**
 * Reads volume metadata through the mount's sector cache.
 */
bool fatMetadataRead(g_fat_mount* mount, uint64_t offset, void* target, size_t length);

/**
 * Checks whether a healthy mount supports writes and durable flushes.
 */
bool fatVolumeWritable(const g_fat_mount* mount);

/**
 * Reports filesystem type, space usage and writability, with optional scan progress.
 */
bool fatVolumeStat(g_fat_mount* mount, g_volume_filesystem_info* info,
	g_volume_progress_callback progress = nullptr, void* context = nullptr);

/**
 * Invalidates cached sectors, allocation chains, inode data and free-space counts.
 */
void fatCacheInvalidate(g_fat_mount* mount);

/**
 * Finds a mount-local inode by its filesystem identity.
 */
g_fat_inode* fatInodeFind(g_fat_mount* mount, g_fs_phys_id id);

/**
 * Records a directory entry and assigns or reuses its mount-local inode identity.
 */
bool fatInodeObserve(g_fat_mount* mount, g_fat_node* node, g_fs_phys_id parent,
	uint64_t offset, const uint64_t* longOffsets, uint8_t longCount, uint8_t attributes);

/**
 * Frees all mount-local inode records.
 */
void fatMountRelease(g_fat_mount* mount);

/**
 * Resizes a file and updates its allocation chain and metadata.
 */
g_fs_mutation_status fatFileResize(g_fat_mount* mount, g_fs_phys_id id, uint64_t length);

/**
 * Writes file bytes, extending the file as needed and reporting the byte count.
 */
g_fs_write_status fatFileWrite(g_fat_mount* mount, g_fs_phys_id id, uint64_t offset,
	const void* source, size_t length, size_t* written);

/**
 * Creates a file or directory entry under the specified parent.
 */
g_fs_mutation_status fatDirectoryCreate(g_fat_mount* mount, g_fs_phys_id parent, const char* name,
	bool directory, g_fat_directory_entry* created);

/**
 * Removes a file or empty directory while preserving data held by open files.
 */
g_fs_mutation_status fatNodeRemove(g_fat_mount* mount, g_fs_phys_id id);

/**
 * Moves an inode within its volume, optionally replacing a specified file inode.
 */
g_fs_mutation_status fatNodeRename(g_fat_mount* mount, g_fs_phys_id id, g_fs_phys_id parent,
	const char* name, g_fs_phys_id replaced);

/**
 * Drops an inode's open count and frees unlinked data after its final close.
 */
g_fs_mutation_status fatInodeClose(g_fat_mount* mount, g_fs_phys_id id);

/**
 * Checks whether a cluster number lies in the volume's usable data range.
 */
bool fatClusterValid(const g_fat_volume& volume, uint32_t cluster);

/**
 * Converts a data-cluster number to a byte offset within the volume.
 */
uint64_t fatClusterOffset(const g_fat_volume& volume, uint32_t cluster);

/**
 * Reads the next cluster in a chain, distinguishing its end from corruption.
 */
g_fat_result fatClusterNext(g_fat_mount* mount, uint32_t cluster, uint32_t* next);

/**
 * Returns a cached, validated allocation chain or validates and caches it.
 */
g_fat_chain_cache* fatChainGet(g_fat_mount* mount, uint32_t first);

/**
 * Caches the length and final cluster of an already validated allocation chain.
 */
g_fat_chain_cache* fatChainRemember(g_fat_mount* mount, uint32_t first, uint32_t count, uint32_t last);

/**
 * Locates a cluster by its zero-based index using a cached chain cursor.
 */
bool fatChainSeek(g_fat_mount* mount, g_fat_chain_cache* chain, uint32_t index, uint32_t* cluster);

/**
 * Finds a contiguous cluster run at a byte offset, bounded by the byte limit.
 */
bool fatChainGetRun(g_fat_mount* mount, g_fat_chain_cache* chain, uint64_t offset, uint64_t limit,
	uint32_t* cluster, uint64_t* count);

/**
 * Decodes and validates a file or directory node from a raw directory entry.
 */
bool fatNodeDecode(const g_fat_volume& volume, uint64_t offset, const uint8_t* raw, g_fat_node* node);

/**
 * Loads node information by inode identity, using cached metadata when valid.
 */
bool fatNodeRead(g_fat_mount* mount, g_fs_phys_id id, g_fat_node* node);

/**
 * Reads file bytes up to the file's end and reports the number read.
 */
bool fatFileRead(g_fat_mount* mount, const g_fat_node& node, uint64_t offset,
	void* target, size_t length, size_t* read);

/**
 * Decodes a FAT 8.3 directory entry into a filename.
 */
bool fatShortNameDecode(const uint8_t* raw, char* name);

/**
 * Collects a long-name entry while checking its sequence and checksum.
 */
void fatLongNameAppend(g_fat_long_name* name, const uint8_t* raw);

/**
 * Validates an assembled long name against its short entry and decodes it to UTF-8.
 */
bool fatLongNameDecode(const g_fat_long_name& name, const uint8_t* raw, char* text);

/**
 * Compares UTF-8 filenames without case sensitivity.
 */
bool fatNameEquals(const char* left, const char* right);

/**
 * Validates a UTF-8 filename and encodes it as FAT long-name UTF-16 units.
 */
bool fatNameEncode(const char* text, uint16_t* units, size_t* count);

/**
 * Initializes an iterator for a directory's entries.
 */
bool fatDirectoryOpen(g_fat_mount* mount, const g_fat_node& node, g_fat_directory* directory);

/**
 * Returns the next file or directory entry, skipping deleted and special records.
 */
g_fat_result fatDirectoryNext(g_fat_mount* mount, g_fat_directory* directory, g_fat_directory_entry* entry);

/**
 * Validates and serves a kernel filesystem delegation request for the mount.
 */
bool fatMountHandleRequest(g_fat_mount* mount, const g_message_header* message, const void* content);

using g_fat_format_progress = void(*)(void*, uint64_t, uint64_t);

/**
 * Formats a storage region as FAT32 and reports progress when requested.
 */
g_volume_status fatFormatVolume(g_fat_device* device, uint64_t first, uint64_t count, uint32_t serial,
	const char* label, g_fat_format_progress progress = nullptr, void* context = nullptr);

/**
 * Claims a volume, registers its VFS mount and commits it to the storage manager.
 */
g_volume_status fatMountStart(g_fat_mount* mount, const char* requestedName = nullptr);

#endif
