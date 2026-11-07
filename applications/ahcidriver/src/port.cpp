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

#include "ahcidriver.hpp"
#include "identify.hpp"
#include <ahcidriver_platform.hpp>
#include <cstdio>

static void ahciConfigurePort(g_ahci_device* disk)
{
	auto port = disk->port;
	ahciWriteRegister(&port->ie, 0);
	ahciWriteRegister(&port->clb, uint64_t(disk->commandList.physical));
	ahciWriteRegister(&port->clbu, uint64_t(disk->commandList.physical) >> 32);
	ahciWriteRegister(&port->fb, uint64_t(disk->receivedFis.physical));
	ahciWriteRegister(&port->fbu, uint64_t(disk->receivedFis.physical) >> 32);
	ahciWriteRegister(&port->serr, 0xffffffffu);
	ahciWriteRegister(&port->is, 0xffffffffu);
	ahciDmaFence();
	ahciWriteRegister(&port->cmd, ahciReadRegister(&port->cmd) | G_AHCI_CMD_FIS_RECEIVE);
}

/**
 * Failed initialization can leave the HBA using its published addresses.
 * Retain the DMA pages unless both engines can be stopped.
 */
static void ahciDiscardDisk(g_ahci_device* disk)
{
	if(ahciStopPortCommands(disk->port))
		ahciReleaseDiskPages(disk);
	delete disk;
}

g_ahci_device* ahciInitializeDisk(uint8_t number, volatile g_ahci_hba_port* port, bool supports64BitDma)
{
	if((ahciReadRegister(&port->ssts) & 15) != G_AHCI_HBA_PORT_SSTS_DET_READY ||
	   ahciReadRegister(&port->sig) != G_SATA_SIGNATURE_ATA)
		return nullptr;
	if(!ahciStopPortCommands(port))
	{
		klog("ahci: port %i DMA engines did not stop", number);
		return nullptr;
	}

	auto disk = new g_ahci_device();
	disk->portNumber = number;
	disk->port = port;
	disk->supports64BitDma = supports64BitDma;
	if(!ahciAllocateDmaPage(disk->commandList, supports64BitDma) ||
	   !ahciAllocateDmaPage(disk->receivedFis, supports64BitDma) ||
	   !ahciAllocateDmaPage(disk->commandTable, supports64BitDma) ||
	   !ahciAllocateDmaPage(disk->identifyData, supports64BitDma))
	{
		ahciReleaseDiskPages(disk);
		delete disk;
		return nullptr;
	}

	ahciConfigurePort(disk);
	if(!ahciWaitRegisterClear(&port->tfd, G_AHCI_TASK_FILE_NOT_READY, G_AHCI_COMMAND_TIMEOUT_MS))
	{
		ahciFailCommand(disk, G_BLOCK_TIMEOUT);
		ahciDiscardDisk(disk);
		return nullptr;
	}

	ahciWriteRegister(&port->cmd, ahciReadRegister(&port->cmd) | G_AHCI_CMD_START);
	disk->online = true;
	auto status = ahciExecuteCommand(disk, G_ATA_COMMAND_IDENTIFY, 0, 0, G_BLOCK_SECTOR_SIZE, false);
	if(status != G_BLOCK_SUCCESS ||
	   !ahciParseIdentify(static_cast<const g_ata_identify_data*>(disk->identifyData.mapped)->words, &disk->info,
						  &disk->flushCommand))
	{
		klog("ahci: port %i IDENTIFY failed or unsupported disk geometry", number);
		ahciDiscardDisk(disk);
		return nullptr;
	}

	return disk;
}
