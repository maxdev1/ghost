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

#include <cstdio>
#include <libpci/pci.hpp>
#include <libpci/driver.hpp>
#include <ghost/io.h>
#include "pcidriver.hpp"

g_atom pciLock = g_atomic_initialize();

g_pci_device* deviceList = nullptr;
g_atom deviceListLock = g_atomic_initialize();

int main()
{
	pciDriverScanBus();

	if(!g_task_register_id(G_PCI_DRIVER_IDENTIFIER))
	{
		klog("PCI driver failed to register with identifier '%s'",G_PCI_DRIVER_IDENTIFIER);
		return -1;
	}
	pciDriverReceiveMessages();
}

void pciDriverReceiveMessages()
{
	size_t buflen = sizeof(g_message_header) + 1024;
	uint8_t buf[buflen];
	for(;;)
	{
		auto result = g_receive_message(buf, buflen);
		if(result != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			klog("error receiving message, retrying");
			continue;
		}

		auto header = (g_message_header*) buf;
		auto request = (g_pci_request_header*) G_MESSAGE_CONTENT(buf);

		if(request->command == G_PCI_IDENTIFY_AHCI_CONTROLLER)
		{
			pciDriverIdentifyAhciController(header->sender, header->transaction);
		}
	}
}

void pciDriverIdentifyAhciController(g_tid sender, g_message_transaction transaction)
{
	g_pci_identify_ahci_controller_response response{};
	response.count = 0;

	g_atomic_lock(deviceListLock);

	g_pci_device* dev = deviceList;
	while(dev)
	{
		if(dev->classCode == PCI_BASE_CLASS_MASS_STORAGE
		   && dev->subclassCode == PCI_01_SUBCLASS_SATA
		   && dev->progIf == PCI_01_06_PROGIF_AHCI)
		{
			uint32_t bar = pciReadConfigInt(dev->bus, dev->device, dev->function, PCI_CONFIG_OFF_BAR5);
			uint8_t interruptLine = pciReadConfigByte(dev->bus, dev->device, dev->function, PCI_CONFIG_OFF_INTR);

			response.entries[response.count].baseAddress = bar & ~0xF;
			response.entries[response.count].interruptLine = interruptLine;

			++response.count;
		}

		dev = dev->next;
	}
	g_atomic_unlock(deviceListLock);

	g_send_message_t(sender, &response, sizeof(g_pci_identify_ahci_controller_response), transaction);
}

void pciDriverScanBus()
{
	int total = 0;
	for(uint16_t bus = 0; bus < PCI_NUM_BUSES; bus++)
	{
		for(uint8_t dev = 0; dev < PCI_NUM_DEVICES; dev++)
		{
			for(uint8_t fun = 0; fun < PCI_NUM_FUNCTIONS; fun++)
			{
				uint32_t header = pciReadConfigInt(bus, dev, fun, PCI_CONFIG_OFF_CLASS);
				uint8_t classCode = (header >> 24) & 0xFF;
				uint8_t subClass = (header >> 16) & 0xFF;
				uint8_t progIf = (header >> 8) & 0xFF;

				if(classCode != 0xFF)
				{
					g_atomic_lock(deviceListLock);
					auto device = new g_pci_device();
					device->bus = bus;
					device->device = dev;
					device->function = fun;
					device->classCode = classCode;
					device->subclassCode = subClass;
					device->progIf = progIf;
					device->next = deviceList;
					deviceList = device;
					g_atomic_unlock(deviceListLock);

					++total;
				}
			}
		}
	}

	klog("PCI driver identified %i devices", total);
}

uint32_t pciReadConfigInt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	uint32_t address = (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset);

	g_atomic_lock(pciLock);
	ioOutportInt(PCI_CONFIG_PORT_ADDR, address);
	auto result = ioInportInt(PCI_CONFIG_PORT_DATA);
	g_atomic_unlock(pciLock);
	return result;
}

uint8_t pciReadConfigByte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	return pciReadConfigInt(bus, device, function, offset) & 0xFF;
}
