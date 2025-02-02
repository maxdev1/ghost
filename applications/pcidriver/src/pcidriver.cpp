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

#include "pcidriver.hpp"

#include <cstdio>
#include <libpci/pci.hpp>
#include <libpci/driver.hpp>
#include <ghost.h>

g_user_mutex pciLock = g_mutex_initialize();

g_pci_device* deviceList = nullptr;
g_user_mutex deviceListLock = g_mutex_initialize();

int main()
{
	pciDriverScanBus();

	if(!g_task_register_id(G_PCI_DRIVER_IDENTIFIER))
	{
		klog("PCI driver failed to register with identifier '%s'", G_PCI_DRIVER_IDENTIFIER);
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
		else if(request->command == G_PCI_IDENTIFY_VMSVGA_CONTROLLER)
		{
			pciDriverIdentifyVmSvgaController(header->sender, header->transaction);
		}
	}
}

void pciDriverIdentifyAhciController(g_tid sender, g_message_transaction transaction)
{
	g_pci_identify_ahci_controller_response response{};
	response.count = 0;

	g_mutex_acquire(deviceListLock);
	g_pci_device* dev = deviceList;
	while(dev)
	{
		if(dev->classCode == PCI_BASE_CLASS_MASS_STORAGE
		   && dev->subclassCode == PCI_01_SUBCLASS_SATA
		   && dev->progIf == PCI_01_06_PROGIF_AHCI)
		{
			uint32_t bar = pciConfigReadDword(dev, PCI_CONFIG_OFF_BAR5);
			uint8_t interruptLine = pciConfigReadByte(dev, PCI_CONFIG_OFF_INTR);

			response.entries[response.count].baseAddress = bar & ~0xF;
			response.entries[response.count].interruptLine = interruptLine;

			++response.count;
		}

		dev = dev->next;
	}
	g_mutex_release(deviceListLock);

	g_send_message_t(sender, &response, sizeof(g_pci_identify_ahci_controller_response), transaction);
}

void pciDriverIdentifyVboxSvgaController(g_tid sender, g_message_transaction transaction)
{
	g_mutex_acquire(deviceListLock);
	g_pci_device* dev = deviceList;
	while(dev)
	{
		if(dev->classCode == PCI_BASE_CLASS_DISPLAY
		   && dev->subclassCode == PCI_03_SUBCLASS_VGA
		   && dev->progIf == PCI_03_00_PROGIF_VGA_COMPATIBLE)
		{
			uint16_t vendor = pciConfigReadWord(dev, PCI_CONFIG_OFF_VENDOR_ID);
			uint16_t device = pciConfigReadWord(dev, PCI_CONFIG_OFF_DEVICE_ID);

			if(vendor == 0x80EE /* VBox */ && device == 0xBEEF /* SVGA */)
			{
				uint32_t bar0 = pciConfigReadDword(dev, PCI_CONFIG_OFF_BAR0) & ~0xF;
				pciConfigWriteDword(dev, PCI_CONFIG_OFF_BAR0, 0xFFFFFFFF);
				uint32_t bar0size = ~(pciConfigReadDword(dev, PCI_CONFIG_OFF_BAR0) & ~
				                      0xF) + 1;
				pciConfigWriteDword(dev, PCI_CONFIG_OFF_BAR0, bar0);
				klog("Found BAR0 %x with size %x", bar0, bar0size);
				// I can't find documentation how to properly implement this controller...
			}
		}

		dev = dev->next;
	}
	g_mutex_release(deviceListLock);
}

void pciDriverIdentifyVmSvgaController(g_tid sender, g_message_transaction transaction)
{
	bool found = false;
	g_mutex_acquire(deviceListLock);
	g_pci_device* dev = deviceList;
	while(dev)
	{
		if(dev->classCode == PCI_BASE_CLASS_DISPLAY
		   && dev->subclassCode == PCI_03_SUBCLASS_VGA
		   && dev->progIf == PCI_03_00_PROGIF_VGA_COMPATIBLE)
		{
			uint16_t vendor = pciConfigReadWord(dev, PCI_CONFIG_OFF_VENDOR_ID);
			uint16_t device = pciConfigReadWord(dev, PCI_CONFIG_OFF_DEVICE_ID);

			if(vendor == 0x15AD /* VMWare */ && device == 0x0405 /* SVGA2 */)
			{
				pciEnableResourceAccess(dev, true);

				g_pci_identify_vmsvga_controller_response response;
				response.found = true;
				response.ioBase = pciConfigGetBAR(dev, 0);
				response.fbBase = pciConfigGetBAR(dev, 1);
				response.fifoBase = pciConfigGetBAR(dev, 2);
				g_send_message_t(sender, &response, sizeof(g_pci_identify_vmsvga_controller_response), transaction);

				found = true;
				break;
			}
		}

		dev = dev->next;
	}
	g_mutex_release(deviceListLock);

	if(!found)
	{
		g_pci_identify_vmsvga_controller_response response;
		response.found = false;
		g_send_message_t(sender, &response, sizeof(g_pci_identify_vmsvga_controller_response), transaction);
	}
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
				uint32_t header = pciConfigReadDwordAt(bus, dev, fun, PCI_CONFIG_OFF_CLASS);
				uint8_t classCode = (header >> 24) & 0xFF;
				uint8_t subClass = (header >> 16) & 0xFF;
				uint8_t progIf = (header >> 8) & 0xFF;

				if(classCode != 0xFF)
				{
					g_mutex_acquire(deviceListLock);
					auto device = new g_pci_device();
					device->bus = bus;
					device->device = dev;
					device->function = fun;
					device->classCode = classCode;
					device->subclassCode = subClass;
					device->progIf = progIf;
					device->next = deviceList;
					deviceList = device;
					g_mutex_release(deviceListLock);

					++total;
				}
			}
		}
	}

	klog("PCI driver identified %i devices", total);
}

void pciEnableResourceAccess(g_pci_device* dev, bool enabled)
{
	uint16_t command = pciConfigReadWord(dev, PCI_CONFIG_OFF_COMMAND);
	const uint8_t flags = 0x0007;

	if(enabled)
		command |= flags;
	else
		command &= ~flags;

	pciConfigWriteWord(dev, PCI_CONFIG_OFF_COMMAND, command);
}

uint32_t pciConfigGetBAR(g_pci_device* dev, int bar)
{
	uint8_t offset = PCI_CONFIG_OFF_BAR0 + (0x4 * bar);

	uint32_t barVal = pciConfigReadDword(dev, offset);
	uint32_t mask = (barVal & PCI_CONFIG_BAR_IO) ? 0x3 : 0xF;
	return barVal & ~mask;
}

uint32_t pciConfigGetBARSize(g_pci_device* dev, int bar)
{
	uint8_t offset = PCI_CONFIG_OFF_BAR0 + (0x4 * bar);
	uint32_t barVal = pciConfigReadDword(dev, offset);

	pciConfigWriteDword(dev, offset, 0xFFFFFFFF);
	uint32_t barSize = ~(pciConfigReadDword(dev, offset) & 0xFFFFFFF0) + 1;;

	pciConfigWriteDword(dev, offset, barVal);
	return barSize;
}

uint8_t pciConfigReadByte(g_pci_device* dev, uint8_t offset)
{
	uint32_t result = pciConfigReadDword(dev, offset);
	return (result >> ((offset & 3) * 8)) & 0xFF;
}

uint16_t pciConfigReadWord(g_pci_device* dev, uint8_t offset)
{
	uint32_t result = pciConfigReadDword(dev, offset);
	return (result >> ((offset & 2) * 8)) & 0xFFFF;
}

uint32_t pciConfigReadDword(g_pci_device* dev, uint8_t offset)
{
	return pciConfigReadDwordAt(dev->bus, dev->device, dev->function, offset);
}

uint32_t pciConfigReadDwordAt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	g_mutex_acquire(pciLock);
	g_io_port_write_dword(PCI_CONFIG_PORT_ADDR, PCI_CONFIG_OFF(bus, device, function, offset));
	auto result = g_io_port_read_dword(PCI_CONFIG_PORT_DATA);
	g_mutex_release(pciLock);
	return result;
}

void pciConfigWriteByte(g_pci_device* dev, uint8_t offset, uint8_t value)
{
	uint32_t current = pciConfigReadDword(dev, offset & ~3);
	uint8_t shift = (offset & 3) * 8;
	current &= ~(0xFF << shift);
	current |= (value << shift);
	pciConfigWriteDword(dev, offset & ~3, current);
}

void pciConfigWriteWord(g_pci_device* dev, uint8_t offset, uint16_t value)
{
	uint32_t current = pciConfigReadDword(dev, offset & ~3);
	uint8_t shift = (offset & 2) * 8;
	current &= ~(0xFFFF << shift);
	current |= (value << shift);
	pciConfigWriteDword(dev, offset & ~3, current);
}

void pciConfigWriteDword(g_pci_device* dev, uint8_t offset, uint32_t value)
{
	pciConfigWriteDwordAt(dev->bus, dev->device, dev->function, offset, value);
}

void pciConfigWriteDwordAt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value)
{
	g_mutex_acquire(pciLock);
	g_io_port_write_dword(PCI_CONFIG_PORT_ADDR, PCI_CONFIG_OFF(bus, device, function, offset));
	g_io_port_write_dword(PCI_CONFIG_PORT_DATA, value);
	g_mutex_release(pciLock);
}
