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

#ifndef STORAGEMANAGER_STORAGE
#define STORAGEMANAGER_STORAGE

#include <libstorage/admin.hpp>
#include <vector>
#include "disk.hpp"

struct g_volume_storage_io
{
	void* context;
	bool (*read)(void*, uint64_t, uint8_t*, uint32_t);
	bool (*write)(void*, uint64_t, const uint8_t*, uint32_t);
	bool (*flush)(void*);
	void (*progress)(void*, g_volume_job_phase, uint64_t, uint64_t);
	uint32_t maxSectors = G_VOLUME_MAX_TRANSFER_SECTORS;
};

struct g_volume_disk_snapshot
{
	g_disk_info disk;
	std::vector<g_disk_partition_info> partitions;
	uint8_t mbr[G_VOLUME_SECTOR_SIZE]{};
};

g_volume_status diskInspect(const g_disk_info& disk, const g_volume_storage_io& io, g_volume_disk_snapshot* snapshot,
							std::vector<g_volume_info>* volumes);

g_volume_status diskTableCreate(const g_volume_disk_snapshot& snapshot, const g_volume_storage_io& io,
								bool replace = false);

g_volume_status diskPartitionCreate(const g_volume_disk_snapshot& snapshot, const g_volume_admin_request& request,
									const g_volume_storage_io& io, uint32_t* created);

g_volume_status diskPartitionDelete(const g_volume_disk_snapshot& snapshot, uint32_t number,
									const g_volume_storage_io& io);

g_volume_status diskZero(const g_disk_info& disk, uint64_t first, uint64_t count, const g_volume_storage_io& io);

g_volume_status diskTableWrite(const g_volume_disk_snapshot& snapshot, const uint8_t* replacement,
							   const g_volume_storage_io& io);

g_volume_status diskBootInfo(const g_volume_disk_snapshot& snapshot, const g_volume_admin_request& request,
							 g_disk_boot_info* info);

g_volume_status diskBootWrite(const g_volume_disk_snapshot& snapshot, const g_volume_admin_request& request,
							  const g_volume_storage_io& io);

g_volume_status diskBootCommit(const g_volume_disk_snapshot& snapshot, const g_volume_admin_request& request,
							   const g_volume_storage_io& io);
#endif
