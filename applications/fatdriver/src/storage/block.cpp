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
#include "cache.hpp"

bool fatDeviceOpen(g_fat_device* device)
{
	device->status = blockDeviceGetInfo(device->handler, device->id, &device->info);
	if(device->status != G_BLOCK_SUCCESS)
		return false;
	if(device->info.sectorSize != G_BLOCK_SECTOR_SIZE || !device->info.sectorCount || !device->info.maxTransferSectors)
	{
		device->status = G_BLOCK_UNSUPPORTED;
		return false;
	}
	device->status = blockDeviceOpenBuffer(device->handler, &device->buffer);
	return device->status == G_BLOCK_SUCCESS;
}

bool fatDeviceClose(g_fat_device* device)
{
	auto status = blockDeviceCloseBuffer(&device->buffer);
	if(status != G_BLOCK_SUCCESS)
		status = blockDeviceCloseBuffer(&device->buffer);
	return status == G_BLOCK_SUCCESS;
}

bool fatVolumeRead(g_fat_device* device, const g_fat_volume& volume, uint64_t offset, void* target, size_t length)
{
	auto volumeBytes = uint64_t(volume.sectorCount) * G_BLOCK_SECTOR_SIZE;
	if(offset > volumeBytes || length > volumeBytes - offset || volume.firstSector > device->info.sectorCount ||
	   volume.sectorCount > device->info.sectorCount - volume.firstSector)
		return false;

	auto maximum = device->buffer.size / G_BLOCK_SECTOR_SIZE;
	if(maximum > device->info.maxTransferSectors)
		maximum = device->info.maxTransferSectors;
	if(!maximum || !device->buffer.data)
		return false;

	auto bytes = static_cast<uint8_t*>(target);
	while(length)
	{
		auto skip = offset % G_BLOCK_SECTOR_SIZE;
		auto count = uint64_t(maximum) * G_BLOCK_SECTOR_SIZE - skip;
		if(count > length)
			count = length;
		auto sectors = (skip + count + G_BLOCK_SECTOR_SIZE - 1) / G_BLOCK_SECTOR_SIZE;
		device->status = blockDeviceReadBuffer(&device->buffer, device->id,
											   volume.firstSector + offset / G_BLOCK_SECTOR_SIZE, sectors);
		if(device->status != G_BLOCK_SUCCESS)
			return false;
		memcpy(bytes, device->buffer.data + skip, count);
		bytes += count;
		offset += count;
		length -= count;
	}
	return true;
}

bool fatVolumeWritable(const g_fat_mount* mount)
{
	auto required = G_BLOCK_WRITABLE | G_BLOCK_FLUSH_SUPPORTED;
	return !mount->failed && (mount->device.info.flags & required) == required;
}

bool fatVolumeFlush(g_fat_mount* mount)
{
	if(mount->failed || !(mount->device.info.flags & G_BLOCK_FLUSH_SUPPORTED))
		return false;
	if(!mount->dirty)
		return true;

	mount->device.status = blockDeviceFlush(mount->device.handler, mount->device.id);
	if(mount->device.status != G_BLOCK_SUCCESS)
		mount->failed = true;
	else
		mount->dirty = false;
	return !mount->failed;
}

/**
 * Writes whole DMA windows where possible and preserves partial sector edges.
 * Failed writes freeze mutation on this mount: a timed-out DMA request must not
 * be retried or have its window reused while its outcome is unknown.
 */
bool fatVolumeWrite(g_fat_mount* mount, uint64_t offset, const void* source, size_t length)
{
	auto& device = mount->device;
	auto capacity = uint64_t(mount->volume.sectorCount) * G_BLOCK_SECTOR_SIZE;
	if(!fatVolumeWritable(mount) || offset > capacity || length > capacity - offset)
		return false;

	auto maximum = std::min(device.info.maxTransferSectors, device.buffer.size / G_BLOCK_SECTOR_SIZE);
	if(!maximum || !device.buffer.data)
		return false;

	auto bytes = static_cast<const uint8_t*>(source);
	while(length)
	{
		auto skip = offset % G_BLOCK_SECTOR_SIZE;
		auto count = std::min(uint64_t(length), uint64_t(maximum) * G_BLOCK_SECTOR_SIZE - skip);
		auto sectors = uint32_t((skip + count + G_BLOCK_SECTOR_SIZE - 1) / G_BLOCK_SECTOR_SIZE);
		auto lba = mount->volume.firstSector + offset / G_BLOCK_SECTOR_SIZE;
		if(skip || count % G_BLOCK_SECTOR_SIZE)
		{
			bool cached = sectors == 1 && fatCacheReadSector(mount, offset / G_BLOCK_SECTOR_SIZE, device.buffer.data);
			device.status = cached ? G_BLOCK_SUCCESS : blockDeviceReadBuffer(&device.buffer, device.id, lba, sectors);
			if(device.status != G_BLOCK_SUCCESS)
			{
				mount->failed = true;
				return false;
			}
		}
		memcpy(device.buffer.data + skip, bytes, count);
		mount->dirty = true;
		device.status = blockDeviceWriteBuffer(&device.buffer, device.id, lba, sectors);
		if(device.status != G_BLOCK_SUCCESS)
		{
			mount->failed = true;
			return false;
		}
		fatCacheUpdate(mount, offset, bytes, count);
		bytes += count;
		offset += count;
		length -= count;
	}
	return true;
}
