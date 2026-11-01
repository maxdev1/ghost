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

#ifndef LIBBLOCKDEVICE_TRANSACTION
#define LIBBLOCKDEVICE_TRANSACTION

#include <cstddef>
#include <libblockdevice/driver.hpp>

/**
 * Sends a control request once accepted and waits for an authenticated reply.
 * A failed exchange does not establish that a shared DMA page is idle.
 */
g_block_status blockDeviceExchange(g_tid handler, const g_block_request& request, void* result, size_t resultBytes,
								   uint32_t expectedSectors);

inline g_block_request blockDeviceCreateRequest(g_device_id device, g_block_command command, uint64_t lba = 0,
												uint32_t sectors = 0, uint64_t buffer = 0, uint32_t offset = 0)
{
	return {G_BLOCK_MAGIC, G_BLOCK_VERSION, command, device, 0, lba, sectors, buffer, offset};
}

#endif
