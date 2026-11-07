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
#include "administration/jobs.hpp"
#include <string.h>

bool diskIoRead(const g_volume_storage_io* io, uint64_t lba, uint8_t* bytes, uint32_t count)
{
	if(!io->buffer || blockDeviceReadBuffer(io->buffer, io->device, lba, count) != G_BLOCK_SUCCESS)
		return false;

	memcpy(bytes, io->buffer->data, count * G_VOLUME_SECTOR_SIZE);
	return true;
}

bool diskIoWrite(const g_volume_storage_io* io, uint64_t lba, const uint8_t* bytes, uint32_t count)
{
	if(!io->buffer)
		return false;

	if(io->job)
		io->job->changed = true;

	memcpy(io->buffer->data, bytes, count * G_VOLUME_SECTOR_SIZE);
	return blockDeviceWriteBuffer(io->buffer, io->device, lba, count) == G_BLOCK_SUCCESS;
}

bool diskIoFlush(const g_volume_storage_io* io)
{
	return io->buffer && blockDeviceFlush(io->buffer->handler, io->device) == G_BLOCK_SUCCESS;
}

void diskIoReportProgress(const g_volume_storage_io* io, g_volume_job_phase phase, uint64_t done, uint64_t total)
{
	if(io->job)
		adminJobReportProgress(io->job, phase, done, total);
}
