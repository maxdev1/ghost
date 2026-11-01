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

#include "storage.hpp"
#include <algorithm>
#include <cstring>
#include "partitions.hpp"

static bool diskReadSector(void* context, uint64_t lba, uint8_t* bytes)
{
	auto io = static_cast<const g_volume_storage_io*>(context);
	return io->read(io->context, lba, bytes, 1);
}

g_volume_status diskInspect(const g_disk_info& disk, const g_volume_storage_io& io, g_volume_disk_snapshot* snapshot,
							std::vector<g_volume_info>* volumes)
{
	*snapshot = {};
	snapshot->disk = disk;
	snapshot->disk.partitions = 0;
	snapshot->disk.firstUsable = 1;
	snapshot->disk.lastUsable = disk.sectorCount ? disk.sectorCount - 1 : 0;
	if(disk.sectorSize != G_VOLUME_SECTOR_SIZE)
	{
		snapshot->disk.layout = G_DISK_UNSUPPORTED;
		return G_VOLUME_UNSUPPORTED;
	}
	if(!io.read(io.context, 0, snapshot->mbr, 1))
	{
		snapshot->disk.layout = G_DISK_INVALID;
		return G_VOLUME_IO_ERROR;
	}

	g_volume_info raw;
	raw.device = disk.device;
	raw.driver = disk.driver;
	raw.sectorCount = disk.sectorCount;
	raw.sectorSize = disk.sectorSize;
	auto status = diskDiscoverPartitions(raw, diskReadSector, const_cast<g_volume_storage_io*>(&io), volumes);
	if(status != G_VOLUME_OK)
	{
		snapshot->disk.layout = status == G_VOLUME_UNSUPPORTED ? G_DISK_UNSUPPORTED : G_DISK_INVALID;
		return status;
	}

	auto mbr = snapshot->mbr;
	bool signature = diskHasSignature(mbr);
	bool filesystem = diskHasFilesystem(mbr);
	snapshot->disk.wholeFilesystem = filesystem;
	snapshot->disk.layout = signature && !filesystem ? G_DISK_MBR : G_DISK_NONE;
	for(unsigned i = 0; i < 4; ++i)
		if(signature && !filesystem && mbr[450 + i * 16] == 0xee)
			snapshot->disk.layout = G_DISK_GPT;

	uint64_t gptTable = 0;
	if(snapshot->disk.layout == G_DISK_GPT)
	{
		uint8_t header[G_VOLUME_SECTOR_SIZE];
		if(!io.read(io.context, 1, header, 1))
		{
			snapshot->disk.layout = G_DISK_INVALID;
			return G_VOLUME_IO_ERROR;
		}
		snapshot->disk.firstUsable = diskRead64(header + 40);
		snapshot->disk.lastUsable = diskRead64(header + 48);
		gptTable = diskRead64(header + 72);
	}

	if(snapshot->disk.layout == G_DISK_MBR)
	{
		for(unsigned i = 0; i < 4; ++i)
		{
			auto entry = mbr + 446 + i * 16;
			if(!diskIsExtended(entry[4]))
				continue;
			snapshot->disk.extended = 1;
			g_disk_partition_info part;
			part.device = disk.device;
			part.number = i + 1;
			part.type = entry[4];
			part.firstSector = diskRead32(entry + 8);
			part.sectorCount = diskRead32(entry + 12);
			part.container = 1;
			snapshot->partitions.push_back(part);
		}
	}

	if(snapshot->disk.layout != G_DISK_NONE && volumes->size() == 1 && volumes->front().scheme == G_VOLUME_WHOLE_DISK)
		volumes->clear();

	for(const auto& volume: *volumes)
	{
		if(!volume.partition)
			continue;
		g_disk_partition_info part;
		part.device = disk.device;
		part.number = volume.partition;
		part.firstSector = volume.firstSector;
		part.sectorCount = volume.sectorCount;
		part.scheme = volume.scheme;
		if(volume.scheme == G_VOLUME_MBR && volume.partition <= 4)
		{
			auto entry = mbr + 446 + (volume.partition - 1) * 16;
			part.type = entry[4];
			part.bootable = entry[0] == 0x80;
		}
		if(volume.scheme == G_VOLUME_GPT)
		{
			uint8_t entries[G_VOLUME_SECTOR_SIZE];
			if(!io.read(io.context, gptTable + (volume.partition - 1) / 4, entries, 1))
			{
				snapshot->disk.layout = G_DISK_INVALID;
				return G_VOLUME_IO_ERROR;
			}
			memcpy(part.typeGuid, entries + (volume.partition - 1) % 4 * 128, 16);
		}
		snapshot->partitions.push_back(part);
	}

	std::sort(snapshot->partitions.begin(), snapshot->partitions.end(),
			  [](const g_disk_partition_info& a, const g_disk_partition_info& b) { return a.number < b.number; });
	snapshot->disk.partitions = snapshot->partitions.size();
	return G_VOLUME_OK;
}

static void diskProgress(const g_volume_storage_io& io, g_volume_job_phase phase, uint64_t done, uint64_t total)
{
	if(io.progress)
		io.progress(io.context, phase, done, total);
}

g_volume_status diskTableWrite(const g_volume_disk_snapshot& snapshot, const uint8_t* replacement,
							   const g_volume_storage_io& io)
{
	uint8_t current[G_VOLUME_SECTOR_SIZE];
	if(!io.read(io.context, 0, current, 1))
		return G_VOLUME_IO_ERROR;

	if(memcmp(current, snapshot.mbr, G_VOLUME_SECTOR_SIZE))
		return G_VOLUME_CONFLICT;

	diskProgress(io, G_VOLUME_JOB_EDITING, 0, 1);
	if(!io.write(io.context, 0, replacement, 1) || !io.flush(io.context) || !io.read(io.context, 0, current, 1) ||
	   memcmp(current, replacement, G_VOLUME_SECTOR_SIZE))
		return G_VOLUME_IO_ERROR;

	diskProgress(io, G_VOLUME_JOB_EDITING, 1, 1);
	return G_VOLUME_OK;
}

g_volume_status diskTableCreate(const g_volume_disk_snapshot& snapshot, const g_volume_storage_io& io, bool replace)
{
	if(snapshot.disk.sectorSize != G_VOLUME_SECTOR_SIZE || !snapshot.disk.sectorCount)
		return G_VOLUME_UNSUPPORTED;

	if(!io.maxSectors || io.maxSectors > G_VOLUME_MAX_TRANSFER_SECTORS)
		return G_VOLUME_INVALID;

	auto leading = std::min(uint64_t(2048), snapshot.disk.sectorCount);
	auto trailing = std::max(leading, snapshot.disk.sectorCount - leading);
	uint8_t mbr[G_VOLUME_SECTOR_SIZE]{};
	mbr[510] = 0x55;
	mbr[511] = 0xaa;
	if(replace)
	{
		auto fresh = snapshot;
		uint8_t empty[G_VOLUME_SECTOR_SIZE]{};
		auto status = diskTableWrite(snapshot, empty, io);
		if(status != G_VOLUME_OK)
			return status;
		status = diskZero(snapshot.disk, 0, leading, io);
		if(status != G_VOLUME_OK)
			return status;
		if(trailing < snapshot.disk.sectorCount)
		{
			status = diskZero(snapshot.disk, trailing, snapshot.disk.sectorCount - trailing, io);
			if(status != G_VOLUME_OK)
				return status;
		}
		memset(fresh.mbr, 0, sizeof(fresh.mbr));
		return diskTableWrite(fresh, mbr, io);
	}

	if(snapshot.disk.layout == G_DISK_MBR)
		return snapshot.partitions.empty() ? G_VOLUME_OK : G_VOLUME_CONFLICT;
	if(snapshot.disk.layout != G_DISK_NONE || snapshot.disk.wholeFilesystem)
		return G_VOLUME_CONFLICT;

	uint8_t bytes[G_VOLUME_SECTOR_SIZE * G_VOLUME_MAX_TRANSFER_SECTORS];


	auto total = leading + snapshot.disk.sectorCount - trailing;
	const uint64_t ranges[][2] = {{0, leading}, {trailing, snapshot.disk.sectorCount - trailing}};
	uint64_t done = 0;
	for(const auto& range: ranges)
	{
		for(uint64_t offset = 0; offset < range[1];)
		{
			auto count = uint32_t(std::min(uint64_t(io.maxSectors), range[1] - offset));
			if(!io.read(io.context, range[0] + offset, bytes, count))
				return G_VOLUME_IO_ERROR;
			for(uint32_t i = 0; i < count * G_VOLUME_SECTOR_SIZE; ++i)
				if(bytes[i])
					return G_VOLUME_CONFLICT;

			offset += count;
			done += count;
			diskProgress(io, G_VOLUME_JOB_CHECKING, done, total);
		}
	}
	return diskTableWrite(snapshot, mbr, io);
}

static bool diskAlign(uint64_t value, uint64_t alignment, uint64_t* result)
{
	if(!alignment)
		return false;

	auto remainder = value % alignment;
	auto extra = remainder ? alignment - remainder : 0;
	if(value > UINT64_MAX - extra)
		return false;
	*result = value + extra;
	return true;
}

g_volume_status diskPartitionCreate(const g_volume_disk_snapshot& snapshot, const g_volume_admin_request& request,
									const g_volume_storage_io& io, uint32_t* created)
{
	if(snapshot.disk.layout != G_DISK_MBR || snapshot.disk.extended)
		return G_VOLUME_UNSUPPORTED;

	if(request.slot > 4 || !request.type || request.type > 255 || diskIsExtended(request.type) ||
	   request.type == 0xee || !request.alignment ||
	   (request.flags & ~(G_DISK_PARTITION_HAS_START | G_DISK_PARTITION_REMAINING)) ||
	   (!(request.flags & G_DISK_PARTITION_REMAINING) && !request.sectorCount))
		return G_VOLUME_INVALID;

	unsigned slot = request.slot;
	if(!slot)
	{
		for(unsigned i = 1; i <= 4; ++i)
		{
			if(snapshot.mbr[446 + (i - 1) * 16 + 4])
				continue;
			slot = i;
			break;
		}
	}

	if(!slot || snapshot.mbr[446 + (slot - 1) * 16 + 4])
		return G_VOLUME_CONFLICT;

	auto parts = snapshot.partitions;
	std::sort(parts.begin(), parts.end(), [](const g_disk_partition_info& a, const g_disk_partition_info& b)
			  { return a.firstSector < b.firstSector; });
	uint64_t limit = std::min(snapshot.disk.sectorCount, uint64_t(UINT32_MAX) + 1);
	uint64_t first;
	bool explicitStart = request.flags & G_DISK_PARTITION_HAS_START;
	if(!diskAlign(explicitStart ? request.firstSector : 1, request.alignment, &first) || !first ||
	   (explicitStart && first != request.firstSector))
		return G_VOLUME_INVALID;

	uint64_t available = 0;
	for(size_t i = 0; i <= parts.size(); ++i)
	{
		if(i < parts.size() && parts[i].firstSector + parts[i].sectorCount <= first)
			continue;
		auto next = i < parts.size() ? std::min(limit, parts[i].firstSector) : limit;
		if(first < next)
		{
			available = next - first;
			if((request.flags & G_DISK_PARTITION_REMAINING) || request.sectorCount <= available)
				break;
		}

		available = 0;
		if(i == parts.size() || explicitStart)
			break;

		if(!diskAlign(parts[i].firstSector + parts[i].sectorCount, request.alignment, &first))
			return G_VOLUME_INVALID;
	}
	if(!available)
		return G_VOLUME_CONFLICT;

	uint64_t count = request.flags & G_DISK_PARTITION_REMAINING ? available : request.sectorCount;
	if(!count || first > UINT32_MAX || count > UINT32_MAX || count > available)
		return G_VOLUME_INVALID;

	uint8_t mbr[G_VOLUME_SECTOR_SIZE];
	memcpy(mbr, snapshot.mbr, sizeof(mbr));
	auto entry = mbr + 446 + (slot - 1) * 16;
	memset(entry, 0, 16);
	entry[1] = entry[5] = 0xfe;
	entry[2] = entry[3] = entry[6] = entry[7] = 0xff;
	entry[4] = request.type;
	diskWrite32(entry + 8, first);
	diskWrite32(entry + 12, count);
	auto status = diskTableWrite(snapshot, mbr, io);
	if(status == G_VOLUME_OK)
		*created = slot;
	return status;
}

g_volume_status diskPartitionDelete(const g_volume_disk_snapshot& snapshot, uint32_t number,
									const g_volume_storage_io& io)
{
	if(snapshot.disk.layout != G_DISK_MBR || snapshot.disk.extended)
		return G_VOLUME_UNSUPPORTED;

	if(number < 1 || number > 4)
		return G_VOLUME_INVALID;

	if(!snapshot.mbr[446 + (number - 1) * 16 + 4])
		return G_VOLUME_NOT_FOUND;

	uint8_t mbr[G_VOLUME_SECTOR_SIZE];
	memcpy(mbr, snapshot.mbr, sizeof(mbr));
	memset(mbr + 446 + (number - 1) * 16, 0, 16);
	return diskTableWrite(snapshot, mbr, io);
}

g_volume_status diskZero(const g_disk_info& disk, uint64_t first, uint64_t count, const g_volume_storage_io& io)
{
	if(disk.sectorSize != G_VOLUME_SECTOR_SIZE)
		return G_VOLUME_UNSUPPORTED;

	if(!count || first >= disk.sectorCount || count > disk.sectorCount - first || !io.maxSectors ||
	   io.maxSectors > G_VOLUME_MAX_TRANSFER_SECTORS)
		return G_VOLUME_INVALID;

	uint8_t zero[G_VOLUME_SECTOR_SIZE * G_VOLUME_MAX_TRANSFER_SECTORS]{};
	uint8_t check[G_VOLUME_SECTOR_SIZE * G_VOLUME_MAX_TRANSFER_SECTORS];
	for(uint64_t done = 0; done < count;)
	{
		auto sectors = uint32_t(std::min(uint64_t(io.maxSectors), count - done));
		if(!io.write(io.context, first + done, zero, sectors) || !io.read(io.context, first + done, check, sectors) ||
		   memcmp(zero, check, sectors * G_VOLUME_SECTOR_SIZE))
			return G_VOLUME_IO_ERROR;

		done += sectors;
		diskProgress(io, G_VOLUME_JOB_ZEROING, done, count);
	}
	return io.flush(io.context) ? G_VOLUME_OK : G_VOLUME_IO_ERROR;
}
