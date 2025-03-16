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

#include <ghost.h>
#include <libpci/driver.hpp>
#include <cstdio>

// TODO:
// This is the device manager that is responsible to keep track of devices in
// the system, give each an ID and manage access to them. It also starts the
// right drivers once it knows which devices exist.

void deviceManagerCheckPciDevices();

int main()
{
	deviceManagerCheckPciDevices();
}

void deviceManagerCheckPciDevices()
{
	int num;
	g_pci_device_data* devices;
	if(!pciDriverListDevices(&num, &devices))
	{
		klog("Failed to list PCI devices");
	}

	bool foundVmsvga = false;

	for(int i = 0; i < num; i++)
	{
		if(devices[i].classCode == PCI_BASE_CLASS_DISPLAY &&
		   devices[i].subclassCode == PCI_03_SUBCLASS_VGA &&
		   devices[i].progIf == PCI_03_00_PROGIF_VGA_COMPATIBLE)
		{
			uint32_t vendorId;
			if(!pciDriverReadConfig(devices[i].deviceAddress, PCI_CONFIG_OFF_VENDOR_ID, 2, &vendorId))
			{
				klog("Failed to read vendor ID from PCI device %x", devices[i].deviceAddress);
				continue;
			}

			uint32_t deviceId;
			if(!pciDriverReadConfig(devices[i].deviceAddress, PCI_CONFIG_OFF_DEVICE_ID, 2, &deviceId))
			{
				klog("Failed to read device ID from PCI device %x", devices[i].deviceAddress);
				continue;
			}

			if(vendorId == 0x15AD /* VMWare */ && deviceId == 0x0405 /* SVGA2 */)
			{
				foundVmsvga = true;
			}
		}
	}
	pciDriverFreeDeviceList(devices);

	// TODO Implement something more sophisticated
	if(foundVmsvga)
	{
		klog("starting VMSVGA driver");
		g_spawn("/applications/vmsvgadriver.bin", "", "", G_SECURITY_LEVEL_DRIVER);
	}
	else
	{
		klog("starting VBE driver");
		g_spawn("/applications/vbedriver.bin", "", "", G_SECURITY_LEVEL_DRIVER);
	}
}
