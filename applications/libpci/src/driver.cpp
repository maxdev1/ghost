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

#include "libpci/driver.hpp"
#include <cstring>

bool pciDriverIdentifyAhciController(g_pci_identify_ahci_controller_entry** out)
{
	g_tid driverTid = g_task_get_id(G_PCI_DRIVER_IDENTIFIER);
	if(driverTid == -1)
	{
		return false;
	}

	g_message_transaction transaction = g_get_message_tx_id();

	g_pci_identify_ahci_controller_request request{};
	request.header.command = G_PCI_IDENTIFY_AHCI_CONTROLLER;
	g_send_message_t(driverTid, &request, sizeof(g_pci_identify_ahci_controller_request), transaction);

	size_t buflen = sizeof(g_message_header) + sizeof(g_pci_identify_ahci_controller_response);
	uint8_t buf[buflen];
	auto status = g_receive_message_t(buf, buflen, transaction);
	auto response = (g_pci_identify_ahci_controller_response*) G_MESSAGE_CONTENT(buf);

	if(status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		memcpy(out, response->entries,
		       sizeof(g_pci_identify_ahci_controller_entry) * G_PCI_IDENTIFY_AHCI_CONTROLLER_ENTRIES);
		return true;
	}

	return false;
}
