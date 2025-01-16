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

#include <ghost.h>
#define G_PCI_DRIVER_IDENTIFIER		"pcidriver"

typedef int g_pci_command;
#define G_PCI_IDENTIFY_AHCI_CONTROLLER	((g_pci_command) 0)

struct g_pci_request_header
{
    g_pci_command command;
}__attribute__((packed));

struct g_pci_identify_ahci_controller_request
{
    g_pci_request_header header;
}__attribute__((packed));

struct g_pci_identify_ahci_controller_entry
{
    g_physical_address baseAddress;
    uint8_t interruptLine;
}__attribute__((packed));

#define G_PCI_IDENTIFY_AHCI_CONTROLLER_ENTRIES 16

struct g_pci_identify_ahci_controller_response
{
    uint8_t count;
    g_pci_identify_ahci_controller_entry entries[G_PCI_IDENTIFY_AHCI_CONTROLLER_ENTRIES];
}__attribute__((packed));

/**
 * Requests the PCI driver to identify AHCI controllers and return information about them.
 *
 * @param out
 *      must point to 16 entries space
 * @return whether the request was successful
 */
bool pciDriverIdentifyAhciController(g_pci_identify_ahci_controller_entry** out);

#endif
