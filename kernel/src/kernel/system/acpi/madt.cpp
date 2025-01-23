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

#include <ghost/memory/types.h>

#include "kernel/system/acpi/madt.hpp"
#include "kernel/system/interrupts/apic/ioapic.hpp"
#include "kernel/system/interrupts/apic/lapic.hpp"
#include "kernel/system/processor/processor.hpp"

#include "shared/logger/logger.hpp"

void madtParse(g_acpi_table_header* madtSdtHeader)
{
	g_madt_header* madtHeader = (g_madt_header*) madtSdtHeader;
	lapicSetup(madtHeader->localControllerAddress);

	uint32_t pos = 0;
	uint32_t entriesLength = (madtHeader->header.length - sizeof(g_madt_header));
	uint8_t* entriesData = (uint8_t*) (((g_address) madtHeader) + sizeof(g_madt_header));
	while(pos < entriesLength)
	{
		g_madt_entry_header* entryHeader = (g_madt_entry_header*) &entriesData[pos];

		if(entryHeader->deviceType == 0) // Local APIC
		{
			g_madt_lapic_entry* entry = (g_madt_lapic_entry*) entryHeader;

			// only useable if entry flag 1
			if(entry->flags == 1)
			{
				logDebug("%# lapic, id: %i, processorId: %i, flags: %i", entry->apicId, entry->processorId, entry->flags);
				processorAdd(entry->apicId, entry->processorId);
			}
		}
		else if(entryHeader->deviceType == 1) // IO APIC
		{
			g_madt_ioapic_entry* entry = (g_madt_ioapic_entry*) entryHeader;
			logDebug("%# ioapic, id: %i, address: %h", entry->ioapicId, entry->ioapicAddress);
			ioapicCreate(entry->ioapicId, entry->ioapicAddress, entry->globalSystemInterruptBase);
		}
		else if(entryHeader->deviceType == 2) // Interrupt Source Override
		{
			g_madt_interrupt_src_override_entry* entry = (g_madt_interrupt_src_override_entry*) entryHeader;
			logDebug("%# int src override, irqSource: %i, globInt: %i, busSource: %i", entry->irqSource, entry->globalSystemInterrupt, entry->busSource);
		}
		else
		{
			logDebug("%# device of unknown type %i", entryHeader->deviceType);
		}

		pos += entryHeader->recordLength;
	}

	processorApicIdCreateMappingTable();
}
