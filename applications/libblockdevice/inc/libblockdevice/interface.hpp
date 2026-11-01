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

#ifndef LIBBLOCKDEVICE_INTERFACE
#define LIBBLOCKDEVICE_INTERFACE

#include <cstdint>
#include <libdevice/interface.hpp>

constexpr uint32_t G_BLOCK_MAGIC = 0x47424c4b;
constexpr uint16_t G_BLOCK_VERSION = 2;
constexpr uint32_t G_BLOCK_DMA_BUFFER_BYTES = 4096;
constexpr uint32_t G_BLOCK_SECTOR_SIZE = 512;
constexpr uint64_t G_BLOCK_IPC_TIMEOUT_MS = 15000;

enum g_block_command : uint16_t
{
	G_BLOCK_GET_INFO = 1,
	G_BLOCK_READ = 2,
	G_BLOCK_WRITE = 3,
	G_BLOCK_FLUSH = 4,
	G_BLOCK_OPEN_BUFFER = 5,
	G_BLOCK_CLOSE_BUFFER = 6,
	G_BLOCK_RELEASE_BUFFER = 7
};

enum g_block_status : uint32_t
{
	G_BLOCK_SUCCESS = 0,
	G_BLOCK_INVALID_REQUEST = 1,
	G_BLOCK_DEVICE_NOT_FOUND = 2,
	G_BLOCK_UNSUPPORTED = 3,
	G_BLOCK_IO_ERROR = 4,
	G_BLOCK_TIMEOUT = 5,
	G_BLOCK_UNAVAILABLE = 6,
	G_BLOCK_PROTOCOL_ERROR = 7,
	G_BLOCK_BUSY = 8
};

constexpr uint32_t G_BLOCK_WRITABLE = 1;
constexpr uint32_t G_BLOCK_FLUSH_SUPPORTED = 2;

struct g_block_device_info
{
	uint64_t sectorCount;
	uint32_t sectorSize;
	uint32_t maxTransferSectors;
	uint32_t flags;
	char model[41];
	char serial[21];
} __attribute__((packed));

struct g_block_request
{
	uint32_t magic;
	uint16_t version;
	g_block_command command;
	g_device_id device;
	uint16_t reserved;
	uint64_t lba;
	uint32_t sectorCount;
	uint64_t buffer;
	uint32_t offset;
} __attribute__((packed));

struct g_block_response
{
	uint32_t magic;
	uint16_t version;
	g_block_command command;
	g_device_id device;
	uint16_t reserved;
	g_block_status status;
	uint32_t sectorsTransferred;
	uint64_t buffer;
} __attribute__((packed));

/**
 * DMA window alias in the requesting process, created through g_share_mem.
 */
struct g_block_buffer_mapping
{
	uint64_t address;
	uint32_t size;
	uint32_t reserved;
} __attribute__((packed));

#endif
