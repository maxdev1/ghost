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

enum g_block_buffer_state : uint32_t
{
	G_BLOCK_BUFFER_EMPTY,
	G_BLOCK_BUFFER_OPENING,
	G_BLOCK_BUFFER_READY,
	G_BLOCK_BUFFER_BUSY,
	G_BLOCK_BUFFER_UNCERTAIN
};

static bool blockDeviceIsMappingValid(const g_block_buffer_mapping& mapping)
{
	return mapping.address && mapping.address <= UINTPTR_MAX && !(mapping.address & (G_BLOCK_DMA_BUFFER_BYTES - 1)) &&
		   mapping.size == G_BLOCK_DMA_BUFFER_BYTES && !mapping.reserved;
}

static bool blockDeviceLockBuffer(g_block_buffer* buffer, uint32_t* previous)
{
	if(!buffer)
		return false;

	*previous = memoryAtomicLoad(&buffer->state, G_MEMORY_ORDER_ACQUIRE);
	while(*previous != G_BLOCK_BUFFER_BUSY)
	{
		if(memoryAtomicCompareExchange(&buffer->state, previous, G_BLOCK_BUFFER_BUSY, false, G_MEMORY_ORDER_ACQUIRE,
									   G_MEMORY_ORDER_RELAXED))
			return true;
	}

	return false;
}

static void blockDeviceUnlockBuffer(g_block_buffer* buffer, uint32_t state)
{
	memoryAtomicStore(&buffer->state, state, G_MEMORY_ORDER_RELEASE);
}

/**
 * An unacknowledged transfer may still use the shared page. Further I/O remains
 * blocked until Close synchronizes with the service and releases the buffer.
 */
static g_block_status blockDeviceTransferBuffer(g_block_buffer* buffer, g_device_id device, uint64_t lba,
												uint32_t sectors, uint32_t offset, bool write)
{
	if(!buffer)
		return G_BLOCK_INVALID_REQUEST;

	uint32_t previous;
	if(!blockDeviceLockBuffer(buffer, &previous))
		return G_BLOCK_BUSY;

	if(previous != G_BLOCK_BUFFER_READY || !sectors || offset % G_BLOCK_SECTOR_SIZE || offset > buffer->size ||
	   sectors > (buffer->size - offset) / G_BLOCK_SECTOR_SIZE)
	{
		blockDeviceUnlockBuffer(buffer, previous);
		return G_BLOCK_INVALID_REQUEST;
	}

	g_block_transfer_request request{{G_BLOCK_MAGIC, G_BLOCK_VERSION, write ? G_BLOCK_WRITE : G_BLOCK_READ},
										 device, lba, sectors, buffer->handle, offset};
	g_block_transfer_response response;

	memoryAtomicFence(G_MEMORY_ORDER_SEQ_CST);
	auto status = blockDeviceExchange(buffer->handler, request, &response);

	bool uncertain = status == G_BLOCK_TIMEOUT || status == G_BLOCK_UNAVAILABLE || status == G_BLOCK_PROTOCOL_ERROR ||
					 status == G_BLOCK_IO_ERROR;
	blockDeviceUnlockBuffer(buffer, uncertain ? G_BLOCK_BUFFER_UNCERTAIN : G_BLOCK_BUFFER_READY);
	return status;
}

g_block_status blockDeviceOpenBuffer(g_tid handler, g_block_buffer* buffer)
{
	if(!buffer)
		return G_BLOCK_INVALID_REQUEST;

	uint32_t previous;
	if(!blockDeviceLockBuffer(buffer, &previous))
		return G_BLOCK_BUSY;

	if(previous != G_BLOCK_BUFFER_EMPTY && (previous != G_BLOCK_BUFFER_OPENING || buffer->handler != handler))
	{
		blockDeviceUnlockBuffer(buffer, previous);
		return G_BLOCK_INVALID_REQUEST;
	}

	if(previous == G_BLOCK_BUFFER_EMPTY)
	{
		buffer->handler = handler;
		buffer->aliasReleased = false;
		buffer->handle = uint64_t(static_cast<uint32_t>(g_get_message_tx_id())) + 1;
	}

	g_block_open_buffer_request request{{G_BLOCK_MAGIC, G_BLOCK_VERSION, G_BLOCK_OPEN_BUFFER}, buffer->handle};
	g_block_open_buffer_response response;
	auto status = blockDeviceExchange(handler, request, &response);
	const auto& mapping = response.mapping;
	if(status == G_BLOCK_SUCCESS)
	{
		if(!blockDeviceIsMappingValid(mapping))
			status = G_BLOCK_PROTOCOL_ERROR;
		else
		{
			buffer->data = reinterpret_cast<uint8_t*>(uintptr_t(mapping.address));
			buffer->size = mapping.size;
		}
	}

	blockDeviceUnlockBuffer(buffer, status == G_BLOCK_SUCCESS ? G_BLOCK_BUFFER_READY : G_BLOCK_BUFFER_OPENING);
	return status;
}

g_block_status blockDeviceCloseBuffer(g_block_buffer* buffer)
{
	if(!buffer)
		return G_BLOCK_INVALID_REQUEST;

	uint32_t previous;
	if(!blockDeviceLockBuffer(buffer, &previous))
		return G_BLOCK_BUSY;

	if(previous == G_BLOCK_BUFFER_EMPTY)
	{
		blockDeviceUnlockBuffer(buffer, G_BLOCK_BUFFER_EMPTY);
		return G_BLOCK_SUCCESS;
	}

	g_block_close_buffer_request request{{G_BLOCK_MAGIC, G_BLOCK_VERSION, G_BLOCK_CLOSE_BUFFER}, buffer->handle};
	g_block_close_buffer_response response;
	auto status = blockDeviceExchange(buffer->handler, request, &response);

	const auto& mapping = response.mapping;
	if(status == G_BLOCK_SUCCESS)
	{
		if(!blockDeviceIsMappingValid(mapping) || (buffer->data && uintptr_t(buffer->data) != mapping.address))
			status = G_BLOCK_PROTOCOL_ERROR;
		else
		{
			if(!buffer->aliasReleased)
				g_unmap(reinterpret_cast<void*>(uintptr_t(mapping.address)));
			buffer->aliasReleased = true;
			buffer->data = nullptr;
			buffer->size = 0;

			g_block_release_buffer_request release{{G_BLOCK_MAGIC, G_BLOCK_VERSION, G_BLOCK_RELEASE_BUFFER}, buffer->handle};
			g_block_release_buffer_response acknowledgement;
			status = blockDeviceExchange(buffer->handler, release, &acknowledgement);
		}
	}
	else if(status == G_BLOCK_DEVICE_NOT_FOUND)
	{
		if(buffer->data)
			g_unmap(buffer->data);
		buffer->data = nullptr;
		buffer->size = 0;
		status = G_BLOCK_SUCCESS;
	}

	if(status == G_BLOCK_SUCCESS)
	{
		buffer->handler = -1;
		buffer->handle = 0;
	}

	blockDeviceUnlockBuffer(buffer, status == G_BLOCK_SUCCESS ? G_BLOCK_BUFFER_EMPTY : G_BLOCK_BUFFER_UNCERTAIN);
	return status;
}

g_block_status blockDeviceReadBuffer(g_block_buffer* buffer, g_device_id device, uint64_t lba, uint32_t sectors,
									 uint32_t offset)
{
	return blockDeviceTransferBuffer(buffer, device, lba, sectors, offset, false);
}

g_block_status blockDeviceWriteBuffer(g_block_buffer* buffer, g_device_id device, uint64_t lba, uint32_t sectors,
									  uint32_t offset)
{
	return blockDeviceTransferBuffer(buffer, device, lba, sectors, offset, true);
}
