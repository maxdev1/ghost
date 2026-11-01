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

#ifndef LIBBLOCKDEVICE_DRIVER
#define LIBBLOCKDEVICE_DRIVER

#include "interface.hpp"

/**
 * Handles belong to the creating process. Calls on one buffer are serialized;
 * callers must also serialize their own direct accesses to data.
 * Do not copy an active buffer or access data while an operation is in flight.
 * state and aliasReleased are private bookkeeping for retryable open/close calls.
 */
struct g_block_buffer
{
	g_tid handler = -1;
	uint64_t handle = 0;
	uint8_t* data = nullptr;
	uint32_t size = 0;
	uint32_t state = 0;
	bool aliasReleased = false;
};

g_block_status blockDeviceOpenBuffer(g_tid handler, g_block_buffer* buffer);
g_block_status blockDeviceCloseBuffer(g_block_buffer* buffer);
g_block_status blockDeviceReadBuffer(g_block_buffer* buffer, g_device_id device, uint64_t lba, uint32_t sectors,
									 uint32_t offset = 0);
g_block_status blockDeviceWriteBuffer(g_block_buffer* buffer, g_device_id device, uint64_t lba, uint32_t sectors,
									  uint32_t offset = 0);

g_block_status blockDeviceGetInfo(g_tid handler, g_device_id device, g_block_device_info* outInfo);
g_block_status blockDeviceRead(g_tid handler, g_device_id device, uint64_t lba, uint32_t sectors, void* buffer,
							   uint32_t* outCompleted = nullptr);
g_block_status blockDeviceWrite(g_tid handler, g_device_id device, uint64_t lba, uint32_t sectors, const void* buffer,
								uint32_t* outCompleted = nullptr);
g_block_status blockDeviceFlush(g_tid handler, g_device_id device);

#endif
