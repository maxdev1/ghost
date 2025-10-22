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

#define G_PCI_DRIVER_NAME		"pcidriver"

/**
 * Packed type to more easily pass device id
 */
typedef uint32_t g_pci_device_address;
#define G_PCI_DEVICE_ADDRESS_BUILD(bus, device, function)   (((uint32_t) bus) | ((uint32_t) device << 8) | ((uint32_t) function << 16))
#define G_PCI_DEVICE_ADDRESS_BUS(id)                        ((uint32_t) (id & 0xFF))
#define G_PCI_DEVICE_ADDRESS_DEVICE(id)                     ((uint32_t) ((id >> 8) & 0xFF))
#define G_PCI_DEVICE_ADDRESS_FUNCTION(id)                   ((uint32_t) ((id >> 16) & 0xFF))

/**
 * Commands that the driver understands
 */
typedef int g_pci_command;
#define G_PCI_LIST_DEVICES	                ((g_pci_command) 1)
#define G_PCI_READ_CONFIG                   ((g_pci_command) 2)
#define G_PCI_WRITE_CONFIG                  ((g_pci_command) 3)
#define G_PCI_READ_BAR                      ((g_pci_command) 4)
#define G_PCI_READ_BAR_SIZE                 ((g_pci_command) 5)
#define G_PCI_ENABLE_RESOURCE_ACCESS        ((g_pci_command) 6)

/**
 * Header structure for messages when communicating with the driver
 */
struct g_pci_request_header
{
    g_pci_command command;
}__attribute__((packed));

/**
 * Basic information about a PCI device
 */
struct g_pci_device_data
{
    g_pci_device_address deviceAddress;

    uint8_t classCode;
    uint8_t subclassCode;
    uint8_t progIf;
}__attribute__((packed));

/**
 * Requests the PCI driver to list all known devices.
 */
bool pciDriverListDevices(int* outCount, g_pci_device_data** outDevices);
void pciDriverFreeDeviceList(g_pci_device_data* deviceList);

struct g_pci_list_devices_request
{
    g_pci_request_header header;
}__attribute__((packed));

struct g_pci_list_devices_count_response
{
    int numDevices;
}__attribute__((packed));

/**
 * Requests the PCI driver to read configuration space.
 */
bool pciDriverReadConfig(g_pci_device_address address, uint8_t offset, int bytes, uint32_t* outValue);

struct g_pci_read_config_request
{
    g_pci_request_header header;
    g_pci_device_address deviceAddress;
    uint8_t offset;
    uint8_t bytes;
}__attribute__((packed));

struct g_pci_read_config_response
{
    bool successful;
    uint32_t value;
}__attribute__((packed));

/**
 * Requests the PCI driver to write configuration space.
 */
bool pciDriverWriteConfig(g_pci_device_address address, uint8_t offset, int bytes, uint32_t value);

struct g_pci_write_config_request
{
    g_pci_request_header header;
    g_pci_device_address deviceAddress;
    uint8_t offset;
    uint8_t bytes;
    uint32_t value;
}__attribute__((packed));

struct g_pci_write_config_response
{
    bool successful;
}__attribute__((packed));

/**
 * Enable resource access on a device.
 */
bool pciDriverEnableResourceAccess(g_pci_device_address address, bool enabled);

struct g_pci_enable_resource_access_request
{
    g_pci_request_header header;
    g_pci_device_address deviceAddress;
    bool enabled;
}__attribute__((packed));

struct g_pci_enable_resource_access_response
{
    bool successful;
}__attribute__((packed));

/**
 * Read a BAR from a device.
 */
bool pciDriverReadBAR(g_pci_device_address address, uint8_t bar, g_address* outValue);

struct g_pci_read_bar_request
{
    g_pci_request_header header;
    g_pci_device_address deviceAddress;
    uint8_t bar;
}__attribute__((packed));

struct g_pci_read_bar_response
{
    bool successful;
    g_address value;
}__attribute__((packed));

/**
 * Read a BAR size from a device.
 */
bool pciDriverReadBARSize(g_pci_device_address address, uint8_t bar, g_address* outValue);

struct g_pci_read_bar_size_request
{
    g_pci_request_header header;
    g_pci_device_address deviceAddress;
    uint8_t bar;
}__attribute__((packed));

struct g_pci_read_bar_size_response
{
    bool successful;
    g_address value;
}__attribute__((packed));

#endif
