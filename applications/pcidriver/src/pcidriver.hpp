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

#include <stdint.h>
#include <ghost.h>

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
 * Reads a BAR value
 */
uint32_t pciConfigGetBAR(g_pci_device* dev, int bar);

/**
 * Reads the size of a BAR. The value that is returned after writing to the BAR contains the fixed bits, hence the flipping.
 */
uint32_t pciConfigGetBARSize(g_pci_device* dev, int bar);

/**
 * Reads from or writes to the PCI configuration space.
*/
uint8_t pciConfigReadByte(g_pci_device* dev, uint8_t offset);
uint16_t pciConfigReadWord(g_pci_device* dev, uint8_t offset);
uint32_t pciConfigReadDword(g_pci_device* dev, uint8_t offset);
uint32_t pciConfigReadDwordAt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

void pciConfigWriteByte(g_pci_device* dev, uint8_t offset, uint8_t value);
void pciConfigWriteWord(g_pci_device* dev, uint8_t offset, uint16_t value);
void pciConfigWriteDword(g_pci_device* dev, uint8_t offset, uint32_t value);
void pciConfigWriteDwordAt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

/**
 * Enables or disables memory space, IO space and bus mastering.
 */
void pciEnableResourceAccess(g_pci_device* dev, bool enabled);

/**
 * Receives incoming messages.
 */
void pciDriverReceiveMessages();

/**
 * Identifies AHCI controllers
 */
void pciDriverIdentifyAhciController(g_tid sender, g_message_transaction transaction);

/**
 * Identifies VBox SVGA controllers
 */
void pciDriverIdentifyVboxSvgaController(g_tid sender, g_message_transaction transaction);
void pciDriverIdentifyVmSvgaController(g_tid sender, g_message_transaction transaction);


#endif
