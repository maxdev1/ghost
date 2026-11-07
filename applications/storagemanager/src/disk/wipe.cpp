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
#include <string.h>

g_volume_status diskWipeRange(const g_disk_info* disk, uint64_t first, uint64_t count, const g_volume_storage_io* io)
{
	if(disk->sectorSize != G_VOLUME_SECTOR_SIZE)
		return G_VOLUME_UNSUPPORTED;
	if(!count || first >= disk->sectorCount || count > disk->sectorCount - first || !io->maxSectors ||
	   io->maxSectors > G_VOLUME_MAX_TRANSFER_SECTORS)
		return G_VOLUME_INVALID;

	uint8_t zero[G_VOLUME_SECTOR_SIZE * G_VOLUME_MAX_TRANSFER_SECTORS] = {};
	uint8_t check[G_VOLUME_SECTOR_SIZE * G_VOLUME_MAX_TRANSFER_SECTORS];
	for(uint64_t done = 0; done < count;)
	{
		uint32_t sectors = io->maxSectors;
		if(sectors > count - done)
			sectors = (uint32_t) (count - done);
		if(!diskIoWrite(io, first + done, zero, sectors) || !diskIoRead(io, first + done, check, sectors) ||
		   memcmp(zero, check, sectors * G_VOLUME_SECTOR_SIZE))
			return G_VOLUME_IO_ERROR;
		done += sectors;
		diskIoReportProgress(io, G_VOLUME_JOB_ZEROING, done, count);
	}
	return diskIoFlush(io) ? G_VOLUME_OK : G_VOLUME_IO_ERROR;
}
