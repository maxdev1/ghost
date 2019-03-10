/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __KERNEL_MADT__
#define __KERNEL_MADT__

#include "kernel/system/acpi/acpi.hpp"
#include "ghost/stdint.h"

struct g_madt_header
{
	g_acpi_table_header header;

	uint32_t localControllerAddress;
	uint32_t flags;
}__attribute__((packed));

/**
 * Header that each entry has
 */
struct g_madt_entry_header
{
	uint8_t deviceType;
	uint8_t recordLength;
}__attribute__((packed));

/**
 * Entry describing a local APIC
 */
struct g_madt_lapic_entry
{
	g_madt_entry_header header;
	uint8_t processorId;
	uint8_t apicId;
	uint32_t flags;
}__attribute__((packed));

/**
 * Entry describing an IO APIC
 */
struct g_madt_ioapic_entry
{
	g_madt_entry_header header;

	uint8_t ioapicId;
	uint8_t reserved0;
	uint32_t ioapicAddress;
	uint32_t globalSystemInterruptBase;
}__attribute__((packed));

/**
 * Entry describing an interrupt source override
 */
struct g_madt_interrupt_src_override_entry
{
	g_madt_entry_header header;

	uint8_t busSource;
	uint8_t irqSource;
	uint32_t globalSystemInterrupt;
	uint16_t flags;
}__attribute__((packed));

/**
 * Reads the Multiple APIC Descriptor Table
 */
void madtParse(g_acpi_table_header* madtHeader);

#endif
