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

#include "boot.hpp"

g_volume_status adminBootPrepare(g_volume_registry* registry, g_tid sender, g_disk_boot_info* boot)
{
	g_volume_disk* disk = registryDiskFind(registry, boot->device);
	if(!disk)
		return G_VOLUME_NOT_FOUND;

	if(!boot->claim || disk->owner != sender || disk->claim != boot->claim || disk->allowedVolume != boot->volume)
		return G_VOLUME_NOT_OWNER;

	if(registryDiskIsBusy(registry, boot->device, sender, 0))
		return G_VOLUME_BUSY;

	uint32_t required = G_BLOCK_WRITABLE | G_BLOCK_FLUSH_SUPPORTED;
	if((disk->snapshot.disk.flags & required) != required)
		return G_VOLUME_UNSUPPORTED;

	g_volume_record* volume = registryVolumeFind(registry, boot->volume);
	if(!volume || !volume->active || volume->info.device != boot->device)
		return G_VOLUME_NOT_FOUND;

	boot->partition = volume->info.partition;
	g_disk_boot_info actual = {};
	g_volume_status status = diskBootGetInfo(&disk->snapshot, boot, &actual);

	if(status != G_VOLUME_OK)
		return status;

	if(actual.firstSector != volume->info.firstSector || actual.sectorCount != volume->info.sectorCount)
		return G_VOLUME_CONFLICT;

	return G_VOLUME_OK;
}
