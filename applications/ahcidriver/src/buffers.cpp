/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include <vector>
#include "ahcidriver.hpp"

struct g_ahci_shared_buffer
{
	uint64_t handle;
	g_pid owner;
	g_ahci_dma_page page;
	uint64_t clientAddress;
	bool closing = false;
	bool poisoned = false;
};

static constexpr size_t G_AHCI_MAX_SHARED_BUFFERS = 64;
static constexpr size_t G_AHCI_MAX_PROCESS_BUFFERS = 8;
static std::vector<g_ahci_shared_buffer> ahciSharedBuffers;
static std::vector<g_ahci_dma_page> ahciQuarantinedBuffers;

/**
 * A poisoned page may still be used by a stuck DMA engine. Keep its driver
 * mapping permanently, even after the client closes it or exits.
 */
static void ahciReleaseSharedBuffer(size_t index)
{
	auto& buffer = ahciSharedBuffers[index];
	if(buffer.poisoned)
		ahciQuarantinedBuffers.push_back(buffer.page);
	else
		g_unmap(buffer.page.mapped);
	ahciSharedBuffers.erase(ahciSharedBuffers.begin() + index);
}

/**
 * Reaps dead owners between commands in the serial service loop. Kernel page
 * references keep a surviving client alias backed after the driver unmaps it.
 */
static void ahciReapSharedBuffers()
{
	for(size_t i = 0; i < ahciSharedBuffers.size();)
	{
		if(g_get_pid_for_tid(ahciSharedBuffers[i].owner) != ahciSharedBuffers[i].owner)
			ahciReleaseSharedBuffer(i);
		else
			++i;
	}
}

static g_ahci_shared_buffer* ahciFindSharedBuffer(g_pid owner, uint64_t handle)
{
	for(auto& buffer: ahciSharedBuffers)
		if(buffer.owner == owner && buffer.handle == handle)
			return &buffer;

	return nullptr;
}

static g_block_status ahciCreateSharedBuffer(g_tid sender, g_pid owner, uint64_t handle,
											 g_ahci_shared_buffer** outBuffer)
{
	size_t count = 0;
	for(auto& buffer: ahciSharedBuffers)
		if(buffer.owner == owner)
			++count;

	if(count >= G_AHCI_MAX_PROCESS_BUFFERS || ahciSharedBuffers.size() >= G_AHCI_MAX_SHARED_BUFFERS)
		return G_BLOCK_BUSY;

	g_ahci_dma_page page;
	if(!ahciAllocateDmaPage(page, true))
		return G_BLOCK_UNAVAILABLE;

	auto alias = g_share_mem(page.mapped, G_PAGE_SIZE, sender);
	if(!alias)
	{
		g_unmap(page.mapped);
		return G_BLOCK_UNAVAILABLE;
	}

	ahciSharedBuffers.push_back({handle, owner, page, uint64_t(alias)});
	*outBuffer = &ahciSharedBuffers.back();
	return G_BLOCK_SUCCESS;
}

static bool ahciValidateTransfer(const g_ahci_device* disk, const g_block_transfer_request& request)
{
	uint32_t count = request.sectorCount;
	return count && count <= disk->info.maxTransferSectors && request.lba < disk->info.sectorCount &&
		   count <= disk->info.sectorCount - request.lba && !(request.offset % disk->info.sectorSize) &&
		   request.offset <= G_BLOCK_DMA_BUFFER_BYTES &&
		   count <= (G_BLOCK_DMA_BUFFER_BYTES - request.offset) / disk->info.sectorSize;
}

g_block_status ahciHandleBufferRequest(const g_block_buffer_request& request, g_tid sender, g_block_buffer_mapping* mapping)
{
	if(!request.buffer)
		return G_BLOCK_INVALID_REQUEST;

	auto owner = g_get_pid_for_tid(sender);
	if(owner == G_PID_NONE)
		return G_BLOCK_INVALID_REQUEST;

	if(request.header.command == G_BLOCK_OPEN_BUFFER || request.header.command == G_BLOCK_RELEASE_BUFFER)
		ahciReapSharedBuffers();

	auto buffer = ahciFindSharedBuffer(owner, request.buffer);
	if(request.header.command == G_BLOCK_OPEN_BUFFER && !buffer)
	{
		auto status = ahciCreateSharedBuffer(sender, owner, request.buffer, &buffer);
		if(status != G_BLOCK_SUCCESS)
			return status;
	}

	if(!buffer)
		return request.header.command == G_BLOCK_RELEASE_BUFFER ? G_BLOCK_SUCCESS : G_BLOCK_DEVICE_NOT_FOUND;

	switch(request.header.command)
	{
		case G_BLOCK_OPEN_BUFFER:
			if(buffer->closing)
				return G_BLOCK_INVALID_REQUEST;
			break;
		case G_BLOCK_CLOSE_BUFFER:
			buffer->closing = true;
			break;
		case G_BLOCK_RELEASE_BUFFER:
			if(!buffer->closing)
				return G_BLOCK_INVALID_REQUEST;

			ahciReleaseSharedBuffer(buffer - ahciSharedBuffers.data());
			return G_BLOCK_SUCCESS;
		default:
			return G_BLOCK_UNSUPPORTED;
	}

	*mapping = {buffer->clientAddress, G_BLOCK_DMA_BUFFER_BYTES, 0};
	return G_BLOCK_SUCCESS;
}

g_block_status ahciTransfer(g_ahci_device* disk, const g_block_transfer_request& request, g_tid sender)
{
	if(!ahciValidateTransfer(disk, request))
		return G_BLOCK_INVALID_REQUEST;

	auto buffer = ahciFindSharedBuffer(g_get_pid_for_tid(sender), request.buffer);
	if(!buffer)
		return G_BLOCK_DEVICE_NOT_FOUND;
	if(buffer->closing)
		return G_BLOCK_INVALID_REQUEST;
	if(buffer->poisoned)
		return G_BLOCK_UNAVAILABLE;

	bool write = request.header.command == G_BLOCK_WRITE;
	if(write && !(disk->info.flags & G_BLOCK_WRITABLE))
		return G_BLOCK_UNSUPPORTED;

	auto status = ahciExecuteCommand(disk, write ? G_ATA_COMMAND_WRITE_DMA_EXT : G_ATA_COMMAND_READ_DMA_EXT,
									 request.lba, request.sectorCount, request.sectorCount * disk->info.sectorSize,
									 write, &buffer->page, request.offset);
	if(!disk->dmaSafe)
		buffer->poisoned = true;
	return status;
}
