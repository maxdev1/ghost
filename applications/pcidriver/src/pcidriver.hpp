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

#ifndef __PCIDRIVER__
#define __PCIDRIVER__

struct g_pci_device
{
    uint8_t bus;
    uint8_t device;
    uint8_t function;

    uint8_t classCode;
    uint8_t subclassCode;
    uint8_t progIf;

    g_pci_device* next;
};

/**
 * Enumerates all devices found on the PCI bus and stores them.
 */
void pciDriverScanBus();

/**
 * Reads a dword from the PCI configuration space.
 */
uint32_t pciReadConfigInt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

/**
 * Reads a byte from the PCI configuration space.
 */
uint8_t pciReadConfigByte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

/**
 * Receives incoming messages.
 */
void pciDriverReceiveMessages();

/**
 * Identifies AHCI controllers and responds accordingly.
 */
void pciDriverIdentifyAhciController(g_tid sender, g_message_transaction transaction);

#endif
