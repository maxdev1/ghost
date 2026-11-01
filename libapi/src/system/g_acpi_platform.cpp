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
#include "ghost/system/acpi.h"

g_acpi_status g_acpi_get_platform_info(uint64_t* rsdp, uint8_t* ioapic)
{
	g_syscall_acpi_platform data{};
	data.status = G_ACPI_NOT_SUPPORTED;
	data.operation = G_ACPI_PLATFORM_INFO;
	g_syscall(G_SYSCALL_ACPI_PLATFORM, (g_address) &data);

	if(rsdp)
		*rsdp = data.address;
	if(ioapic)
		*ioapic = data.ioapic;

	return data.status;
}

void* g_acpi_map(uint64_t address, uint32_t size)
{
	g_syscall_acpi_platform data{};
	data.status = G_ACPI_NOT_SUPPORTED;
	data.operation = G_ACPI_PLATFORM_MAP;
	data.address = address;
	data.size = size;
	g_syscall(G_SYSCALL_ACPI_PLATFORM, (g_address) &data);

	return data.status == G_ACPI_SUCCESSFUL ? (void*) data.address : nullptr;
}

g_acpi_status g_acpi_configure_irq(uint32_t irq, uint8_t operation)
{
	g_syscall_acpi_platform data{};
	data.status = G_ACPI_NOT_SUPPORTED;
	data.operation = operation;
	data.irq = irq;
	g_syscall(G_SYSCALL_ACPI_PLATFORM, (g_address) &data);

	return data.status;
}
