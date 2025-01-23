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
#include <libahci/ahci.hpp>
#include <libahci/driver.hpp>

int main()
{
	g_task_register_id(G_AHCI_DRIVER_IDENTIFIER);

	g_pci_identify_ahci_controller_entry* entries;
	if((entries = ahciDriverIdentifyController()) == nullptr)
		return -1;

	auto ahciController = entries[0];
	g_irq_create_redirect(ahciController.interruptLine, 2);

	auto ahciControllerVirt = g_map_mmio((void*) ahciController.baseAddress, 0x1000);
	klog("mapped AHCI controller at %x to virtual %x", ahciController.baseAddress, ahciControllerVirt);

	auto ahciGhc = (volatile g_ahci_hba_ghc*) ahciControllerVirt;
	ahciGhc->ghc.ahciEnable = 1;
	ahciGhc->ghc.interruptEnable = 1;
	klog("AHCI ports implemented: %i", ahciGhc->pi);

	auto ahciPorts = (volatile g_ahci_hba_port*) (ahciControllerVirt + G_AHCI_HBA_PORT_OFFSET);
	for(uint8_t portNumber = 0; portNumber < 32; portNumber++)
	{
		if(!(ahciGhc->pi & (1 << portNumber)))
			continue;
		ahciIdentifyDevice(portNumber, &ahciPorts[portNumber]);
	}

	g_sleep(999999);
}

g_pci_identify_ahci_controller_entry* ahciDriverIdentifyController()
{
	g_pci_identify_ahci_controller_entry* entries = nullptr;
	while(true)
	{
		int count;
		// TODO avoid this loop
		if(pciDriverIdentifyAhciController(&entries, &count))
		{
			if(count < 1)
			{
				klog("no AHCI controllers were reported by PCI driver, exiting");
				return nullptr;
			}

			break;
		}
		else
		{
			klog("failed to identify AHCI controller, retrying soon...");
		}
		g_sleep(5000);
	}
	return entries;
}

void ahciIdentifyDevice(uint8_t portNumber, volatile g_ahci_hba_port* port)
{
	if(!(port->ssts.det == G_AHCI_HBA_PORT_SSTS_DET_READY))
	{
		klog("device on port %i not present or ready: %i", portNumber, port->ssts.det);
		return;
	}

	if(port->sig != G_SATA_SIGNATURE_ATA)
	{
		klog("port %i does not have an ATA device: %x", portNumber, port->sig);
		return;
	}

	if(!ahciPortStopCommands(port))
	{
		klog("timed out when trying to stop commands on port %i", portNumber);
		return;
	}

	// ...
	klog("ready to initialize device on port %i", portNumber);
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
