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

#ifndef __LIBPCI_PCIDRIVER__
#define __LIBPCI_PCIDRIVER__

#include "pci.hpp"
#include <ghost.h>

/**
 * Requests the PCI driver to list all known devices. Release the returned list
 * with pciDriverFreeDeviceList.
 */
bool pciDriverListDevices(int* outCount, g_pci_device_data** outDevices);
void pciDriverFreeDeviceList(g_pci_device_data* deviceList);

/**
 * Requests the PCI driver to read or write configuration space.
 */
bool pciDriverReadConfig(g_pci_device_address address, uint8_t offset, int bytes, uint32_t* outValue);
bool pciDriverWriteConfig(g_pci_device_address address, uint8_t offset, int bytes, uint32_t value);

/**
 * Enables or disables a device's resource access.
 */
bool pciDriverEnableResourceAccess(g_pci_device_address address, bool enabled);

/**
 * Reads a device BAR.
 */
bool pciDriverReadBAR(g_pci_device_address address, uint8_t bar, g_address* outValue);

#endif
