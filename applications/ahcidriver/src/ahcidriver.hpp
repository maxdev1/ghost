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

#ifndef AHCIDRIVER
#define AHCIDRIVER

#include <ghost.h>
#include <libahci/ahci.hpp>
#include <libahci/ata.hpp>
#include <libblockdevice/interface.hpp>

constexpr uint32_t G_AHCI_CMD_START = 1;
constexpr uint32_t G_AHCI_CMD_FIS_RECEIVE = 1 << 4;
constexpr uint32_t G_AHCI_CMD_FIS_RUNNING = 1 << 14;
constexpr uint32_t G_AHCI_CMD_RUNNING = 1 << 15;
constexpr uint32_t G_AHCI_TASK_FILE_NOT_READY = 0x80 | 0x08;
constexpr uint64_t G_AHCI_COMMAND_TIMEOUT_MS = 5000;
constexpr uint64_t G_AHCI_ENGINE_TIMEOUT_MS = 1000;

struct g_ahci_dma_page
{
	void* mapped = nullptr;
	void* physical = nullptr;
};

struct g_ahci_device
{
	g_device_id id = 0;
	uint8_t portNumber = 0;
	volatile g_ahci_hba_port* port = nullptr;
	g_ahci_dma_page commandList;
	g_ahci_dma_page receivedFis;
	g_ahci_dma_page commandTable;
	g_ahci_dma_page identifyData;
	g_block_device_info info{};
	uint8_t flushCommand = 0;
	bool online = false;
	bool dmaSafe = true;
	bool supports64BitDma = false;
};

bool ahciWaitRegisterClear(volatile uint32_t* reg, uint32_t bits, uint64_t timeout);

bool ahciAllocateDmaPage(g_ahci_dma_page& page, bool supports64BitDma);

/**
 * Releases private DMA pages only before their addresses are published to the
 * HBA, or after both port engines have stopped.
 */
void ahciReleaseDiskPages(g_ahci_device* disk);

/**
 * Stops command processing before FIS reception, as required by AHCI. Callers
 * must retain DMA memory if either engine fails to stop.
 */
bool ahciStopPortCommands(volatile g_ahci_hba_port* port);
g_block_status ahciFailCommand(g_ahci_device* disk, g_block_status status);

/**
 * Executes one non-NCQ command in the serial service loop. Data points at a
 * borrowed shared DMA page, or the private IDENTIFY page when omitted.
 * On failure, dmaSafe determines whether the borrowed page can be released.
 */
g_block_status ahciExecuteCommand(g_ahci_device* disk, uint8_t command, uint64_t lba, uint16_t sectors, uint32_t bytes,
								  bool write, g_ahci_dma_page* data = nullptr, uint32_t offset = 0);

bool ahciPowerFlush();

g_ahci_device* ahciInitializeDisk(uint8_t number, volatile g_ahci_hba_port* port, bool supports64BitDma);

bool ahciInitializeControllers();

void ahciRegisterDevices();

g_ahci_device* ahciFindDisk(g_device_id id);

/**
 * Implements process-owned buffer handles. Open and Close return the same
 * mapping on retries. Close forbids further I/O but retains the driver page
 * until Release acknowledges that the client has unmapped its alias.
 */
g_block_status ahciHandleBufferRequest(const g_block_buffer_request& request, g_tid sender, g_block_buffer_mapping* mapping);

g_block_status ahciTransfer(g_ahci_device* disk, const g_block_transfer_request& request, g_tid sender);

/**
 * Dispatches control-only IPC in the serial service loop. Replies are sent
 * without blocking so a stalled client cannot stop service for other disks.
 */
void ahciHandleRequest(const g_message_header* message, const uint8_t* content);

[[noreturn]] void ahciReceiveMessages();

#endif
