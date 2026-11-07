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

#include <cstddef>
#include <cstdint>
#include <libdevice/interface.hpp>

constexpr uint32_t G_BLOCK_MAGIC = 0x47424c4b;
constexpr uint16_t G_BLOCK_VERSION = 3;
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

struct g_block_header
{
	uint32_t magic = G_BLOCK_MAGIC;
	uint16_t version = G_BLOCK_VERSION;
	g_block_command command = G_BLOCK_GET_INFO;
} __attribute__((packed));

struct g_block_response_header
{
	uint32_t magic = G_BLOCK_MAGIC;
	uint16_t version = G_BLOCK_VERSION;
	g_block_command command = G_BLOCK_GET_INFO;
	g_block_status status = G_BLOCK_INVALID_REQUEST;
} __attribute__((packed));

struct g_block_device_request
{
	g_block_header header;
	g_device_id device = 0;
} __attribute__((packed));

using g_block_get_info_request = g_block_device_request;
using g_block_flush_request = g_block_device_request;

struct g_block_transfer_request
{
	g_block_header header;
	g_device_id device = 0;
	uint64_t lba = 0;
	uint32_t sectorCount = 0;
	uint64_t buffer = 0;
	uint32_t offset = 0;
} __attribute__((packed));

using g_block_read_request = g_block_transfer_request;
using g_block_write_request = g_block_transfer_request;

struct g_block_buffer_request
{
	g_block_header header;
	uint64_t buffer = 0;
} __attribute__((packed));

using g_block_open_buffer_request = g_block_buffer_request;
using g_block_close_buffer_request = g_block_buffer_request;
using g_block_release_buffer_request = g_block_buffer_request;

/**
 * DMA window alias in the requesting process, created through g_share_mem.
 */
struct g_block_buffer_mapping
{
	uint64_t address;
	uint32_t size;
	uint32_t reserved;
} __attribute__((packed));

struct g_block_get_info_response
{
	g_block_response_header header;
	g_device_id device = 0;
	g_block_device_info info{};
} __attribute__((packed));

struct g_block_flush_response
{
	g_block_response_header header;
	g_device_id device = 0;
} __attribute__((packed));

struct g_block_transfer_response
{
	g_block_response_header header;
	g_device_id device = 0;
	uint64_t buffer = 0;
	uint32_t sectorsTransferred = 0;
} __attribute__((packed));

using g_block_read_response = g_block_transfer_response;
using g_block_write_response = g_block_transfer_response;

struct g_block_buffer_mapping_response
{
	g_block_response_header header;
	uint64_t buffer = 0;
	g_block_buffer_mapping mapping{};
} __attribute__((packed));

using g_block_open_buffer_response = g_block_buffer_mapping_response;
using g_block_close_buffer_response = g_block_buffer_mapping_response;

struct g_block_release_buffer_response
{
	g_block_response_header header;
	uint64_t buffer = 0;
} __attribute__((packed));

constexpr size_t blockDeviceRequestSize(g_block_command command)
{
	switch(command)
	{
		case G_BLOCK_GET_INFO:
		case G_BLOCK_FLUSH: return sizeof(g_block_device_request);
		case G_BLOCK_READ:
		case G_BLOCK_WRITE: return sizeof(g_block_transfer_request);
		case G_BLOCK_OPEN_BUFFER:
		case G_BLOCK_CLOSE_BUFFER:
		case G_BLOCK_RELEASE_BUFFER: return sizeof(g_block_buffer_request);
		default: return 0;
	}
}

constexpr size_t blockDeviceResponseSize(g_block_command command)
{
	switch(command)
	{
		case G_BLOCK_GET_INFO: return sizeof(g_block_get_info_response);
		case G_BLOCK_FLUSH: return sizeof(g_block_flush_response);
		case G_BLOCK_READ:
		case G_BLOCK_WRITE: return sizeof(g_block_transfer_response);
		case G_BLOCK_OPEN_BUFFER:
		case G_BLOCK_CLOSE_BUFFER: return sizeof(g_block_buffer_mapping_response);
		case G_BLOCK_RELEASE_BUFFER: return sizeof(g_block_release_buffer_response);
		default: return 0;
	}
}

#endif
