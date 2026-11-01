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

#include <cstring>
#include <ghost.h>
#include "transaction.hpp"

static g_block_status blockDeviceTransfer(g_tid handler, g_device_id device, uint64_t lba, uint32_t sectors, void* data,
										  uint32_t* outCompleted, bool write)
{
	if(outCompleted)
		*outCompleted = 0;
	if(!data || !sectors)
		return G_BLOCK_INVALID_REQUEST;

	g_block_device_info info;
	auto status = blockDeviceGetInfo(handler, device, &info);
	if(status != G_BLOCK_SUCCESS)
		return status;

	if(lba >= info.sectorCount || sectors > info.sectorCount - lba || sectors > SIZE_MAX / info.sectorSize)
		return G_BLOCK_INVALID_REQUEST;
	if(write && !(info.flags & G_BLOCK_WRITABLE))
		return G_BLOCK_UNSUPPORTED;

	g_block_buffer buffer;
	status = blockDeviceOpenBuffer(handler, &buffer);
	uint32_t completed = 0;
	while(status == G_BLOCK_SUCCESS && completed < sectors)
	{
		uint32_t count = sectors - completed;
		if(count > info.maxTransferSectors)
			count = info.maxTransferSectors;

		size_t bytes = count * info.sectorSize;
		auto part = static_cast<uint8_t*>(data) + static_cast<size_t>(completed) * info.sectorSize;
		if(write)
			memcpy(buffer.data, part, bytes);

		status = write ? blockDeviceWriteBuffer(&buffer, device, lba + completed, count)
					   : blockDeviceReadBuffer(&buffer, device, lba + completed, count);
		if(status != G_BLOCK_SUCCESS)
			break;

		if(!write)
			memcpy(part, buffer.data, bytes);
		completed += count;
		if(outCompleted)
			*outCompleted = completed;
	}

	auto closed = blockDeviceCloseBuffer(&buffer);
	if(closed != G_BLOCK_SUCCESS)
		closed = blockDeviceCloseBuffer(&buffer);

	return status == G_BLOCK_SUCCESS ? closed : status;
}

g_block_status blockDeviceGetInfo(g_tid handler, g_device_id device, g_block_device_info* outInfo)
{
	if(!outInfo)
		return G_BLOCK_INVALID_REQUEST;

	g_block_device_info info{};
	auto request = blockDeviceCreateRequest(device, G_BLOCK_GET_INFO);
	auto status = blockDeviceExchange(handler, request, &info, sizeof(info), 0);
	if(status != G_BLOCK_SUCCESS)
		return status;

	if(!info.sectorCount || info.sectorSize != G_BLOCK_SECTOR_SIZE || !info.maxTransferSectors ||
	   info.maxTransferSectors > G_BLOCK_DMA_BUFFER_BYTES / info.sectorSize || info.model[sizeof(info.model) - 1] ||
	   info.serial[sizeof(info.serial) - 1])
		return G_BLOCK_PROTOCOL_ERROR;

	*outInfo = info;
	return G_BLOCK_SUCCESS;
}

g_block_status blockDeviceRead(g_tid handler, g_device_id device, uint64_t lba, uint32_t sectors, void* buffer,
							   uint32_t* outCompleted)
{
	return blockDeviceTransfer(handler, device, lba, sectors, buffer, outCompleted, false);
}

g_block_status blockDeviceWrite(g_tid handler, g_device_id device, uint64_t lba, uint32_t sectors, const void* buffer,
								uint32_t* outCompleted)
{
	return blockDeviceTransfer(handler, device, lba, sectors, const_cast<void*>(buffer), outCompleted, true);
}

g_block_status blockDeviceFlush(g_tid handler, g_device_id device)
{
	auto request = blockDeviceCreateRequest(device, G_BLOCK_FLUSH);
	return blockDeviceExchange(handler, request, nullptr, 0, 0);
}
