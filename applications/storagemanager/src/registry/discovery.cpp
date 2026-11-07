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

#include "discovery.hpp"
#include <ghost/system.h>
#include <string.h>

g_volume_status registryDiscoveryScanDisk(g_volume_registry* registry, const g_volume_info* known, g_tid requester, g_volume_job_id job,
						 bool manual)
{
	g_volume_disk* disk = registryDiskFind(registry, known->device);
	if(disk && registryDiskIsBusy(registry, known->device, requester, job))
		return G_VOLUME_BUSY;

	g_block_device_info device = {};
	if(blockDeviceGetInfo(known->driver, known->device, &device) != G_BLOCK_SUCCESS)
		return G_VOLUME_IO_ERROR;
	if(!disk)
		disk = registryDiskRegister(registry, known->device);

	g_block_buffer buffer = {};
	if(blockDeviceOpenBuffer(known->driver, &buffer) != G_BLOCK_SUCCESS)
		return G_VOLUME_IO_ERROR;

	g_disk_info details = {};
	details.device = known->device;
	details.driver = known->driver;
	details.sectorSize = device.sectorSize;
	details.sectorCount = device.sectorCount;
	details.flags = device.flags;
	memcpy(details.model, device.model, sizeof(details.model));
	memcpy(details.serial, device.serial, sizeof(details.serial));
	details.generation = disk->snapshot.disk.generation + 1;
	g_volume_storage_io io = {known->device, &buffer, NULL, G_VOLUME_MAX_TRANSFER_SECTORS};
	g_volume_status status = diskLayoutInspect(&details, &io, &disk->snapshot);

	g_block_status closed = blockDeviceCloseBuffer(&buffer);
	if(closed != G_BLOCK_SUCCESS)
		closed = blockDeviceCloseBuffer(&buffer);
	if(closed != G_BLOCK_SUCCESS)
		return G_VOLUME_IO_ERROR;
	if(status == G_VOLUME_IO_ERROR && !manual && disk->info.sectorCount)
		return status;

	g_volume_info info = *known;
	info.sectorCount = device.sectorCount;
	info.sectorSize = device.sectorSize;
	g_volume_status published = registryVolumeReconcileDisk(registry, &info, manual);
	return published == G_VOLUME_OK ? status : published;
}

void registryDiscoveryHandleDevice(g_volume_registry* registry, g_tid manager, const g_message_header* message,
					   const g_device_event_device_registered* event)
{
	if(message->sender != manager || message->length != sizeof(*event))
		return;
	if(event->header.event != G_DEVICE_EVENT_DEVICE_REGISTERED || event->type != G_DEVICE_TYPE_BLOCK || !event->id ||
	   event->driver == G_TID_NONE || registryDiskFind(registry, event->id))
		return;

	g_volume_info info = {};
	info.device = event->id;
	info.driver = event->driver;
	registryDiscoveryScanDisk(registry, &info, G_TID_NONE, 0, false);
}
