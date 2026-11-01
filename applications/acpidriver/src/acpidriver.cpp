/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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
#include <ghost.h>
#include "platform.hpp"
#include "power.hpp"
#include "service.hpp"

int main()
{
	if(!g_task_register_name(G_ACPI_DRIVER_NAME))
		return 1;

	uint64_t rsdp = 0;
	uint8_t ioapic = 0;
	g_acpi_service service;
	if(g_acpi_get_platform_info(&rsdp, &ioapic) == G_ACPI_SUCCESSFUL && acpiPlatformStart(rsdp))
		service.info = acpiPowerInitialize(ioapic);

	klog("acpi: driver ready; tables %u namespace %u", service.info.tablesReady, service.info.namespaceReady);
	uint8_t bytes[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	for(;;)
	{
		if(g_receive_message(bytes, sizeof(bytes)) != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			continue;
		auto header = reinterpret_cast<g_message_header*>(bytes);
		acpiServiceReceive(&service, header, G_MESSAGE_CONTENT(bytes));
	}
}
