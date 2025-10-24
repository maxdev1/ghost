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
#include <cstdio>
#include <cstring>
#include <libahci/ahci.hpp>
#include <libahci/ata.hpp>
#include <libahci/driver.hpp>
#include <libpci/driver.hpp>

static uint32_t controllerBar;
static uint8_t controllerIntrLine;

void debugDumpBytes(const uint8_t* data, size_t len)
{
	for(size_t i = 0; i < len; i += 64)
	{
		size_t chunk = (len - i >= 64) ? 64 : (len - i);
		char chars[64];

		for(size_t j = 0; j < chunk; j++)
		{
			uint8_t b = data[i + j];
			chars[j] = (b == 0) ? '0' : (char) b;
		}
		for(size_t j = chunk; j < 64; j++)
		{
			chars[j] = ' ';
		}

		klog(
				"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"
				"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				chars[0], chars[1], chars[2], chars[3], chars[4], chars[5], chars[6], chars[7],
				chars[8], chars[9], chars[10], chars[11], chars[12], chars[13], chars[14], chars[15],
				chars[16], chars[17], chars[18], chars[19], chars[20], chars[21], chars[22], chars[23],
				chars[24], chars[25], chars[26], chars[27], chars[28], chars[29], chars[30], chars[31],
				chars[32], chars[33], chars[34], chars[35], chars[36], chars[37], chars[38], chars[39],
				chars[40], chars[41], chars[42], chars[43], chars[44], chars[45], chars[46], chars[47],
				chars[48], chars[49], chars[50], chars[51], chars[52], chars[53], chars[54], chars[55],
				chars[56], chars[57], chars[58], chars[59], chars[60], chars[61], chars[62], chars[63]
				);
	}
}

int main()
{
	if(!g_task_register_name(G_AHCI_DRIVER_NAME))
	{
		klog("failed to register as %s", G_AHCI_DRIVER_NAME);
		return -1;
	}

	if(!ahciDriverIdentifyController())
	{
		klog("Failed to identify AHCI controller");
		return -1;
	}

	g_irq_create_redirect(controllerIntrLine, 2);

	auto ahciControllerVirt = g_map_mmio((void*) controllerBar, 0x1000);
	klog("mapped AHCI controller at %x to virtual %x", controllerBar, ahciControllerVirt);

	auto ahciGhc = (volatile g_ahci_hba_ghc*) ahciControllerVirt;
	ahciGhc->ghc.ahciEnable = 1;
	ahciGhc->ghc.interruptEnable = 1;
	klog("AHCI ports implemented: %i", ahciGhc->pi);

	auto ahciPorts = (volatile g_ahci_hba_port*) (ahciControllerVirt + G_AHCI_HBA_PORT_OFFSET);
	for(uint8_t portNumber = 0; portNumber < 32; portNumber++)
	{
		if(!(ahciGhc->pi & (1 << portNumber)))
			continue;

		g_ahci_device* ahciDevice = nullptr;
		if(!ahciInitializeDevice(portNumber, &ahciPorts[portNumber], &ahciDevice))
		{
			klog("AHCI device initialization on port %i failed", portNumber);
			continue;
		}

		if(!ahciIdentifyDevice(ahciDevice))
		{
			klog("AHCI device identification on port %i failed", portNumber);
			continue;
		}

		// Try to read something
		void* outVirt;
		if(!ahciReadDMA(ahciDevice, 0, 1, &outVirt))
		{
			klog("failed to read from AHCI device on port %i", portNumber);
			continue;
		}
		klog("SUCCESSFULLY READ DATA!");

		debugDumpBytes((uint8_t*) outVirt, 512);
	}

	g_sleep(999999);
}

bool ahciDriverIdentifyController()
{
	int count;
	g_pci_device_data* devices;
	if(!pciDriverListDevices(&count, &devices))
	{
		klog("failed to list PCI devices");
		return false;
	}

	bool found = false;
	for(int i = 0; i < count; i++)
	{
		if(devices[i].classCode == PCI_BASE_CLASS_MASS_STORAGE &&
		   devices[i].subclassCode == PCI_01_SUBCLASS_SATA &&
		   devices[i].progIf == PCI_01_06_PROGIF_AHCI)
		{
			g_address bar;
			if(!pciDriverReadBAR(devices[i].deviceAddress, 5, &bar))
			{
				klog("Failed to read BAR5 from PCI device %x", devices[i].deviceAddress);
				continue;
			}

			uint32_t interruptLine;
			if(!pciDriverReadConfig(devices[i].deviceAddress, PCI_CONFIG_OFF_INTR, 1, &interruptLine))
			{
				klog("Failed to read interrupt line from PCI device %x", devices[i].deviceAddress);
				continue;
			}

			controllerBar = bar;
			controllerIntrLine = interruptLine;
			klog("AHCI controller at bar %x, intr line %x", bar, interruptLine);
			found = true;
			break;
		}
	}
	pciDriverFreeDeviceList(devices);

	return found;
}


void ahciPrintDeviceData(identify_device_response_t* id_data)
{
	char model[41];
	memset(model, 0, sizeof(model));
	for(int i = 0; i < 20; i++)
	{
		model[i * 2] = (id_data->ModelNumber[i] >> 8) & 0xFF;
		model[i * 2 + 1] = id_data->ModelNumber[i] & 0xFF;
	}

	char serial[21];
	memset(serial, 0, sizeof(serial));
	for(int i = 0; i < 10; i++)
	{
		serial[i * 2] = (id_data->SerialNumber[i] >> 8) & 0xFF;
		serial[i * 2 + 1] = id_data->SerialNumber[i] & 0xFF;
	}

	char firmware[5];
	memset(firmware, 0, sizeof(firmware));
	for(int i = 0; i < 4; i++)
	{
		firmware[i] = (id_data->FirmwareRevision[i] >> 8) & 0xFF;
		firmware[i + 1] = id_data->FirmwareRevision[i] & 0xFF;
	}

	klog("model: \"%s\"", model);
	klog("serial number: \"%s\"", serial);
	klog("firmware revision: \"%s\"", firmware);
	klog("heads: %i", id_data->Heads);
	klog("cylinders: %i", id_data->CylindersLow);
	klog("sectors per track: %i", id_data->SectorsPerTrack);
}

bool ahciReadDMA(g_ahci_device* ahciDevice,
                 uint64_t lba,
                 uint16_t sectorCount,
                 void** outVirt)
{
	int slot = ahciFindFreeCommandSlot(ahciDevice->port);
	if(slot == -1)
		return false;

	size_t bytes = (size_t) sectorCount * 512;
	if(bytes > G_PAGE_SIZE)
	{
		// TODO
		klog("TODO: FAILED TO READ DMA - no contiugous physical alloc implemneted");
		return false;
	}

	size_t alloc_size = ((bytes + G_PAGE_SIZE - 1) / G_PAGE_SIZE) * G_PAGE_SIZE;
	void* data_phys;
	void* data_virt = g_alloc_mem_p(alloc_size, &data_phys);
	if(!data_phys)
		return false;
	memset(data_virt, 0, alloc_size);

	void* cmdTablePhys;
	void* cmdTable = g_alloc_mem_p(G_PAGE_SIZE, &cmdTablePhys);
	if(!cmdTablePhys)
	{
		klog("failed to allocate buffer for cmd table");
		return false;
	}
	memset(cmdTable, 0, G_PAGE_SIZE);

	auto cmd = &ahciDevice->cmdList.mapped[slot];
	memset((void*) cmd, 0, sizeof(*cmd));

	auto prdts = (g_hba_prdt_entry*) (cmdTable + G_HBA_COMMAND_TABLE_PRDT_OFFSET);
	prdts[0].dba = ((uint64_t) data_phys) & 0xFFFFFFFFUL;
	prdts[0].dbau = ((uint64_t) data_phys >> 32) & 0xFFFFFFFFUL;
	prdts[0].dbc = (uint32_t) (bytes - 1);
	prdts[0].i = 1;

	cmd->prdtl = 1;
	cmd->ctba = ((uint64_t) cmdTablePhys) & 0xFFFFFFFFUL;
	cmd->ctbau = ((uint64_t) cmdTablePhys >> 32) & 0xFFFFFFFFUL;
	cmd->cfl = sizeof(g_fis_reg_h2d) / sizeof(uint32_t);
	cmd->w = 0; // read

	auto cmdFis = (g_fis_reg_h2d*) &((g_hba_command_table*) cmdTable)->cfis;
	memset(cmdFis, 0, sizeof(*cmdFis));
	cmdFis->type = G_FIS_TYPE_REG_H2D;
	cmdFis->c = 1;
	cmdFis->command = 0x25; // read dma ext
	cmdFis->device = 0x40; // lba mode

	cmdFis->lba0 = (uint8_t) (lba & 0xFF);
	cmdFis->lba1 = (uint8_t) ((lba >> 8) & 0xFF);
	cmdFis->lba2 = (uint8_t) ((lba >> 16) & 0xFF);
	cmdFis->lba3 = (uint8_t) ((lba >> 24) & 0xFF);
	cmdFis->lba4 = (uint8_t) ((lba >> 32) & 0xFF);
	cmdFis->lba5 = (uint8_t) ((lba >> 40) & 0xFF);

	cmdFis->countL = (uint8_t) (sectorCount & 0xFF);
	cmdFis->countH = (uint8_t) ((sectorCount >> 8) & 0xFF);

	ahciDevice->port->is = -1;
	ahciDevice->port->ie = 1;

	while(ahciDevice->port->tfd & (0x80 | 0x08))
		g_sleep(1);

	ahciDevice->port->ci = 1 << slot;

	while(ahciDevice->port->ci & (1 << slot))
	{
		if(ahciDevice->port->is & (1 << 30))
		{
			klog("ahci: task file error on port");
			return false;
		}
		g_sleep(1);
	}

	if(outVirt)
		*outVirt = data_virt;
	return true;
}


bool ahciInitializeDevice(uint8_t portNumber, volatile g_ahci_hba_port* port, g_ahci_device** outAhciDevice)
{
	if(!(port->ssts.det == G_AHCI_HBA_PORT_SSTS_DET_READY))
	{
		klog("device on port %i not present or ready: %i", portNumber, port->ssts.det);
		return false;
	}

	if(port->sig != G_SATA_SIGNATURE_ATA)
	{
		klog("port %i does not have an ATA device: %x", portNumber, port->sig);
		return false;
	}

	if(!ahciPortStopCommands(port))
	{
		klog("timed out when trying to stop commands on port %i", portNumber);
		return false;
	}

	// Initialize command list
	void* cmdListPhys;
	void* cmdList = g_alloc_mem_p(G_PAGE_SIZE, &cmdListPhys);
	if(!cmdListPhys)
	{
		klog("failed to allocate physical memory for command-list of device");
		return false;
	}
	port->clb = ((uint64_t) cmdListPhys) & 0xFFFFFFFFUL;
	port->clbu = ((uint64_t) cmdListPhys >> 32) & 0xFFFFFFFFUL;
	memset(cmdList, 0, G_PAGE_SIZE);

	// Initialize FIS memory
	void* fisPhys;
	void* fis = g_alloc_mem_p(G_PAGE_SIZE, &fisPhys);
	if(!fisPhys)
	{
		klog("failed to allocate physical memory for FIS of device");
		return false;
	}
	memset(fis, 0, G_PAGE_SIZE);
	port->fb = ((uint64_t) fisPhys) & 0xFFFFFFFFUL;;
	port->fbu = ((uint64_t) fisPhys >> 32) & 0xFFFFFFFFUL;

	// Start commands
	if(!ahciPortStartCommands(port))
	{
		klog("failed to start AHCI commands on port %i", port);
		return false;
	}

	// Clear pending interrupt bits
	port->is = -1;
	port->ie = 1;

	// Create device
	auto ahciDevice = new g_ahci_device();
	ahciDevice->portNumber = portNumber;
	ahciDevice->port = port;
	ahciDevice->cmdList.mapped = (g_hba_command_header*) cmdList;
	ahciDevice->cmdList.physical = cmdListPhys;
	ahciDevice->fis.mapped = fis;
	ahciDevice->fis.physical = fisPhys;
	*outAhciDevice = ahciDevice;

	return true;
}


bool ahciIdentifyDevice(g_ahci_device* ahciDevice)
{
	int slot = ahciFindFreeCommandSlot(ahciDevice->port);
	if(slot == -1)
	{
		klog("failed to find free slot to issue identify-device command");
		return false;
	}

	// Buffer for command result
	void* outPhys;
	void* out = g_alloc_mem_p(G_PAGE_SIZE, &outPhys);
	if(!outPhys)
	{
		klog("failed to allocate buffer for command output");
		return false;
	}
	memset(out, 0, G_PAGE_SIZE);

	// Buffer for cmd table
	void* cmdTablePhys;
	void* cmdTable = g_alloc_mem_p(G_PAGE_SIZE, &cmdTablePhys);
	if(!cmdTablePhys)
	{
		klog("failed to allocate buffer for cmd table");
		return false;
	}
	memset(cmdTable, 0, G_PAGE_SIZE);

	// Create command
	auto cmd = &ahciDevice->cmdList.mapped[slot];
	cmd->prdtl = 1;
	cmd->cfl = sizeof(g_fis_reg_h2d) / sizeof(uint32_t);
	cmd->w = 0;

	cmd->ctba = ((uint64_t) cmdTablePhys) & 0xFFFFFFFFUL;
	cmd->ctbau = ((uint64_t) cmdTablePhys >> 32) & 0xFFFFFFFFUL;

	// CMD table
	auto prdts = (g_hba_prdt_entry*) (cmdTable + G_HBA_COMMAND_TABLE_PRDT_OFFSET);
	prdts[0].dba = ((uint64_t) outPhys) & 0xFFFFFFFFUL;
	prdts[0].dbau = ((uint64_t) outPhys >> 32) & 0xFFFFFFFFUL;
	prdts[0].dbc = (512 - 1);
	prdts[0].i = 1;

	// Fill the FIS
	auto cmdFis = (g_fis_reg_h2d*) &((g_hba_command_table*) cmdTable)->cfis;
	cmdFis->type = G_FIS_TYPE_REG_H2D;
	cmdFis->command = 0xEC; // ATA Identify Device
	cmdFis->device = 0; // Master Device
	cmdFis->c = 1;

	// Wait for device to be ready
	while((ahciDevice->port->tfd & (0x80 /* ATA_DEV_BUSY */ | 0x08 /* ATA_DEV_DRQ */)))
		g_sleep(1);

	// Issue command in slot
	ahciDevice->port->ci = 1 << slot;

	// Wait for completion
	while(ahciDevice->port->ci & (1 << slot))
	{
		if(ahciDevice->port->is & (1 << 30))
		{
			// TODO Handle error
			klog("task file error");
			return false;
		}
		g_sleep(1);
	}

	ahciPrintDeviceData((identify_device_response_t*) out);
	return true;
}

bool ahciPortStartCommands(volatile g_ahci_hba_port* port)
{
	uint32_t timeout = 1000000;
	while(port->cmd.cr && --timeout)
	{
	}

	if(timeout > 0)
	{
		port->cmd.fre = 1;
		port->cmd.st = 1;
		return true;
	}
	return false;
}

bool ahciPortStopCommands(volatile g_ahci_hba_port* port)
{
	port->cmd.st = 0;
	port->cmd.fre = 0;

	uint32_t timeout = 1000000;
	while((port->cmd.fr || port->cmd.cr) && --timeout)
	{
	}
	return timeout > 0;
}

int ahciFindFreeCommandSlot(volatile g_ahci_hba_port* port)
{
	// If not set in SACT and CI, the slot is free
	uint32_t slots = (port->sact | port->ci);
	for(int i = 0; i < 32; i++)
	{
		if((slots & 1) == 0)
			return i;
		slots >>= 1;
	}
	return -1;
}
