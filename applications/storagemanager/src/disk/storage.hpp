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

#ifndef STORAGEMANAGER_DISK_STORAGE
#define STORAGEMANAGER_DISK_STORAGE

#include <libblockdevice/driver.hpp>
#include <libstorage/admin.hpp>
#include "layout.hpp"
#include <vector>

#define G_DISK_MAX_PARTITIONS (G_DISK_GPT_MAX_PARTITIONS + G_DISK_MBR_PARTITION_COUNT)

struct g_volume_admin_job;

struct g_volume_storage_io
{
	g_device_id device;
	g_block_buffer* buffer;
	g_volume_admin_job* job;
	uint32_t maxSectors;
};

struct g_volume_disk_snapshot
{
	g_disk_info disk;
	std::vector<g_disk_partition_info> partitions;
	uint8_t mbr[G_VOLUME_SECTOR_SIZE];
};

struct g_disk_partition_options
{
	uint64_t firstSector;
	uint64_t sectorCount;
	uint64_t alignment;
	uint32_t flags;
	uint32_t type;
	uint32_t slot;
};

/**
 * Reads count sectors at a disk-relative LBA and copies them into bytes.
 */
bool diskIoRead(const g_volume_storage_io* io, uint64_t lba, uint8_t* bytes, uint32_t count);

/**
 * Writes count sectors from bytes at a disk-relative LBA. Marks an associated
 * job as changed before submitting the write, including failed attempts.
 */
bool diskIoWrite(const g_volume_storage_io* io, uint64_t lba, const uint8_t* bytes, uint32_t count);

/**
 * Requests the block driver to flush pending device writes to storage.
 */
bool diskIoFlush(const g_volume_storage_io* io);

/**
 * Publishes phase and progress to the associated job, if one is present.
 */
void diskIoReportProgress(const g_volume_storage_io* io, g_volume_job_phase phase, uint64_t done, uint64_t total);

/**
 * Classifies the disk layout from sector zero and MBR/GPT metadata, storing
 * validated partition descriptions in snapshot.
 */
g_volume_status diskLayoutInspect(const g_disk_info* disk, const g_volume_storage_io* io, g_volume_disk_snapshot* snapshot);

/**
 * Initializes an empty MBR, optionally replacing existing metadata by clearing
 * the disk's leading and trailing metadata areas.
 */
g_volume_status diskPartitionCreateTable(const g_volume_disk_snapshot* snapshot, const g_volume_storage_io* io, bool replace);

/**
 * Replaces sector zero only if it still matches expected, then flushes and
 * reads back the replacement to verify it.
 */
g_volume_status diskPartitionWriteTable(const uint8_t* expected, const uint8_t* replacement, const g_volume_storage_io* io);

/**
 * Allocates an aligned primary MBR partition in free space, writes the table
 * and returns its one-based slot in created on success.
 */
g_volume_status diskPartitionCreate(const g_volume_disk_snapshot* snapshot, const g_disk_partition_options* options,
								   const g_volume_storage_io* io, uint32_t* created);

/**
 * Removes a primary MBR entry without erasing the partition's sector contents.
 */
g_volume_status diskPartitionDelete(const g_volume_disk_snapshot* snapshot, uint32_t number, const g_volume_storage_io* io);

/**
 * Validates a primary MBR target and its pre-partition boot area, returning
 * the resolved partition geometry, disk identity and scan generation in info.
 */
g_volume_status diskBootGetInfo(const g_volume_disk_snapshot* snapshot, const g_disk_boot_info* request, g_disk_boot_info* info);

/**
 * Writes a sector-aligned chunk of size bytes into the reserved boot area,
 * excluding sector zero, then flushes and verifies the written data.
 */
g_volume_status diskBootWrite(const g_volume_disk_snapshot* snapshot, const g_disk_boot_info* boot, uint64_t firstSector,
							 const uint8_t* bytes, uint32_t size, const g_volume_storage_io* io);

/**
 * Installs MBR boot code and disk identity, optionally activating the target
 * primary partition while preserving partition geometry.
 */
g_volume_status diskBootCommit(const g_volume_disk_snapshot* snapshot, const g_disk_boot_info* boot,
							  const uint8_t* code, uint32_t flags, const g_volume_storage_io* io);

/**
 * Zero-fills and reads back the disk-relative sector range in bounded transfers,
 * then flushes the device.
 */
g_volume_status diskWipeRange(const g_disk_info* disk, uint64_t first, uint64_t count, const g_volume_storage_io* io);

#endif
