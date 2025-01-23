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

#ifndef __AHCIDRIVER__
#define __AHCIDRIVER__

#include <stdint.h>
#include <ghost.h>
#include <libahci/ahci.hpp>
#include <libpci/driver.hpp>

g_pci_identify_ahci_controller_entry* ahciDriverIdentifyController();
bool ahciPortStartCommands(volatile g_ahci_hba_port* port);
bool ahciPortStopCommands(volatile g_ahci_hba_port* port);
int ahciFindFreeCommandSlot(volatile g_ahci_hba_port* port);
void ahciIdentifyDevice(uint8_t portNumber, volatile g_ahci_hba_port* port);

#endif
