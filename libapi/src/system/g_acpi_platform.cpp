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

#include "ghost/syscall.h"
#include "ghost/system.h"

g_acpi_status g_acpi_get_platform_info(uint64_t* rsdp, uint8_t* ioapic)
{
	g_syscall_acpi_platform_info data{};
	data.status = G_ACPI_NOT_SUPPORTED;

	g_syscall(G_SYSCALL_ACPI_GET_PLATFORM_INFO, (g_address) &data);

	if(rsdp)
		*rsdp = data.rsdp;
	if(ioapic)
		*ioapic = data.ioapic;

	return data.status;
}

void* g_acpi_map(uint64_t address, uint32_t size)
{
	g_syscall_acpi_map data{};
	data.physicalAddress = address;
	data.size = size;
	data.status = G_ACPI_NOT_SUPPORTED;

	g_syscall(G_SYSCALL_ACPI_MAP, (g_address) &data);

	return data.status == G_ACPI_SUCCESSFUL ? (void*) data.virtualAddress : nullptr;
}
