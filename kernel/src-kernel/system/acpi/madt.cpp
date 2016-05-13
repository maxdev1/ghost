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

#include "ghost/types.h"
#include <system/acpi/madt.hpp>
#include <system/system.hpp>
#include <logger/logger.hpp>
#include <system/interrupts/lapic.hpp>
#include <system/interrupts/ioapic_manager.hpp>

/**
 *
 */
void g_madt::parse(g_acpi_table_header* madtSdtHeader) {

	g_madt_header* madtHeader = (g_madt_header*) madtSdtHeader;

	uint32_t entriesLength = (madtHeader->header.length - sizeof(g_madt_header));
	uint8_t* entriesData = (uint8_t*) (((g_address) madtHeader) + sizeof(g_madt_header));

	/**
	 * TODO: tuxie mentioned that the value in the ACPI tables might not be trustworthy
	 * due to GRUB changing the APIC location and not updating the MADT. Check this.
	 */
	g_log_debug("%! reported LAPIC address %h", "madt", madtHeader->localControllerAddress);
	g_lapic::prepare(madtHeader->localControllerAddress);

	g_log_debug("%! listing entries in MADT:", "madt");
	uint32_t pos = 0;
	while (pos < entriesLength) {
		g_madt_entry_header* entryHeader = (g_madt_entry_header*) &entriesData[pos];

		if (entryHeader->deviceType == 0) {	// Local APIC
			g_madt_lapic_entry* entry = (g_madt_lapic_entry*) entryHeader;

			// only useable if entry flag 1
			if (entry->flags == 1) {
				g_log_debug("%# lapic, id: %i, processorId: %i, flags: %i", entry->apicId, entry->processorId, entry->flags);
				g_system::addProcessor(entry->apicId);
			}

		} else if (entryHeader->deviceType == 1) { // IO APIC
			g_madt_ioapic_entry* entry = (g_madt_ioapic_entry*) entryHeader;
			g_log_debug("%# ioapic, id: %i, address: %h", entry->ioapicId, entry->ioapicAddress);
			g_ioapic_manager::create(entry->ioapicId, entry->ioapicAddress, entry->globalSystemInterruptBase);

		} else if (entryHeader->deviceType == 2) { // Interrupt Source Override

			#if	G_LOGGING_DEBUG
			g_madt_interrupt_src_override_entry* entry = (g_madt_interrupt_src_override_entry*) entryHeader;
			g_log_debug("%# int src override, irqSource: %i, globInt: %i, busSource: %i", entry->irqSource, entry->globalSystemInterrupt, entry->busSource);
			#endif

		} else {
			g_log_debug("%# device of unknown type %i", entryHeader->deviceType);
		}

		pos += entryHeader->recordLength;

	}
}
