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

#include <ahcidriver_platform.hpp>
#include <cstdio>
#include <cstring>
#include "ahcidriver.hpp"

/**
 * Takes a failed disk offline and records whether borrowed DMA memory is safe
 * to release. Private port pages remain allocated even if shutdown fails.
 */
g_block_status ahciFailCommand(g_ahci_device* disk, g_block_status status)
{
	disk->online = false;
	disk->dmaSafe = ahciStopPortCommands(disk->port);
	klog("ahci: port %i offline, error %i, DMA stopped %i", disk->portNumber, status, disk->dmaSafe);
	return status;
}

static constexpr uint32_t G_AHCI_PORT_ERRORS =
		(1u << 30) | (1u << 29) | (1u << 28) | (1u << 27) | (1u << 26) | (1u << 24);

static constexpr uint32_t G_AHCI_TASK_FILE_ERRORS = 1 | (1 << 5);

static void ahciPrepareCommandHeader(g_ahci_device* disk, uint64_t physical, uint32_t bytes, bool write)
{
	auto header = static_cast<g_hba_command_header*>(disk->commandList.mapped);
	memset(header, 0, sizeof(*header));
	memset(disk->commandTable.mapped, 0, G_PAGE_SIZE);
	header->cfl = sizeof(g_fis_reg_h2d) / 4;
	header->w = write;
	header->ctba = uint64_t(disk->commandTable.physical);
	header->ctbau = uint64_t(disk->commandTable.physical) >> 32;

	if(bytes)
	{
		auto table = static_cast<uint8_t*>(disk->commandTable.mapped);
		auto prdt = reinterpret_cast<g_hba_prdt_entry*>(table + G_HBA_COMMAND_TABLE_PRDT_OFFSET);
		prdt->dba = physical;
		prdt->dbau = physical >> 32;
		prdt->dbc = bytes - 1;
		header->prdtl = 1;
	}
}

static void ahciPrepareCommandFis(g_ahci_device* disk, uint8_t command, uint64_t lba, uint16_t sectors)
{
	auto fis = static_cast<g_fis_reg_h2d*>(disk->commandTable.mapped);
	fis->type = G_FIS_TYPE_REG_H2D;
	fis->c = 1;
	fis->command = command;
	fis->device = 0x40;
	fis->lba0 = lba;
	fis->lba1 = lba >> 8;
	fis->lba2 = lba >> 16;
	fis->lba3 = lba >> 24;
	fis->lba4 = lba >> 32;
	fis->lba5 = lba >> 40;
	fis->countL = sectors;
	fis->countH = sectors >> 8;
}

static g_block_status ahciWaitCommandCompletion(g_ahci_device* disk, uint32_t bytes)
{
	auto port = disk->port;
	auto started = g_millis();
	unsigned quickPolls = 0;

	for(;;)
	{
		uint32_t active = ahciReadRegister(&port->ci);
		if((ahciReadRegister(&port->is) & G_AHCI_PORT_ERRORS) ||
		   (ahciReadRegister(&port->tfd) & G_AHCI_TASK_FILE_ERRORS))
			return ahciFailCommand(disk, G_BLOCK_IO_ERROR);

		if(!(active & 1))
			break;

		if(g_millis() - started >= G_AHCI_COMMAND_TIMEOUT_MS)
			return ahciFailCommand(disk, G_BLOCK_TIMEOUT);

		if(quickPolls < 256)
		{
			++quickPolls;
			ahciPollPause();
		}
		else
			g_sleep(1);
	}

	ahciDmaFence();
	auto header = static_cast<g_hba_command_header*>(disk->commandList.mapped);
	if(bytes && header->prdbc != bytes)
		return ahciFailCommand(disk, G_BLOCK_IO_ERROR);

	disk->dmaSafe = true;
	return G_BLOCK_SUCCESS;
}

bool ahciWaitRegisterClear(volatile uint32_t* reg, uint32_t bits, uint64_t timeout)
{
	auto started = g_millis();
	while(ahciReadRegister(reg) & bits)
	{
		if(g_millis() - started >= timeout)
			return false;

		g_sleep(1);
	}

	return true;
}

bool ahciAllocateDmaPage(g_ahci_dma_page& page, bool supports64BitDma)
{
	page.mapped = g_alloc_mem_p(G_PAGE_SIZE, &page.physical);
	if(!page.mapped || !page.physical || (!supports64BitDma && (uint64_t(page.physical) >> 32)))
	{
		if(page.mapped)
			g_unmap(page.mapped);
		page = {};
		return false;
	}

	memset(page.mapped, 0, G_PAGE_SIZE);
	return true;
}

void ahciReleaseDiskPages(g_ahci_device* disk)
{
	g_ahci_dma_page* pages[] = {&disk->identifyData, &disk->commandTable, &disk->receivedFis, &disk->commandList};
	for(auto page: pages)
	{
		if(page->mapped)
			g_unmap(page->mapped);
		*page = {};
	}
}

bool ahciStopPortCommands(volatile g_ahci_hba_port* port)
{
	ahciWriteRegister(&port->cmd, ahciReadRegister(&port->cmd) & ~G_AHCI_CMD_START);
	if(!ahciWaitRegisterClear(&port->cmd, G_AHCI_CMD_RUNNING, G_AHCI_ENGINE_TIMEOUT_MS))
		return false;

	ahciWriteRegister(&port->cmd, ahciReadRegister(&port->cmd) & ~G_AHCI_CMD_FIS_RECEIVE);
	return ahciWaitRegisterClear(&port->cmd, G_AHCI_CMD_FIS_RUNNING, G_AHCI_ENGINE_TIMEOUT_MS);
}

g_block_status ahciExecuteCommand(g_ahci_device* disk, uint8_t command, uint64_t lba, uint16_t sectors, uint32_t bytes,
								  bool write, g_ahci_dma_page* data, uint32_t offset)
{
	if(!disk->online)
		return G_BLOCK_UNAVAILABLE;
	if(offset > G_PAGE_SIZE || bytes > G_PAGE_SIZE - offset || lba >= G_ATA_LBA48_LIMIT)
		return G_BLOCK_INVALID_REQUEST;
	if(!data)
		data = &disk->identifyData;
	uint64_t physical = uint64_t(data->physical) + offset;
	if(bytes && (!data->physical || (!disk->supports64BitDma && physical + bytes - 1 > UINT32_MAX)))
		return G_BLOCK_UNSUPPORTED;

	auto port = disk->port;
	if(ahciReadRegister(&port->ci) || ahciReadRegister(&port->sact))
		return ahciFailCommand(disk, G_BLOCK_IO_ERROR);
	if(!ahciWaitRegisterClear(&port->tfd, G_AHCI_TASK_FILE_NOT_READY, G_AHCI_COMMAND_TIMEOUT_MS))
		return ahciFailCommand(disk, G_BLOCK_TIMEOUT);

	ahciPrepareCommandHeader(disk, physical, bytes, write);
	ahciPrepareCommandFis(disk, command, lba, sectors);
	ahciWriteRegister(&port->is, 0xffffffffu);
	ahciWriteRegister(&port->serr, 0xffffffffu);
	ahciDmaFence();
	disk->dmaSafe = false;
	ahciWriteRegister(&port->ci, 1);
	return ahciWaitCommandCompletion(disk, bytes);
}
