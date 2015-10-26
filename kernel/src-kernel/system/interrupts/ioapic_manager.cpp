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

#include <system/interrupts/ioapic_manager.hpp>
#include <system/interrupts/ioapic.hpp>

#include <system/io_ports.hpp>
#include <logger/logger.hpp>
#include <kernel.hpp>
#include <system/processor.hpp>

static g_ioapic* first = 0;

/**
 *
 */
g_ioapic* g_ioapic_manager::getEntries() {
	return first;
}

/**
 *
 */
void g_ioapic_manager::create(uint32_t id, g_physical_address ioapicAddress, uint32_t globalSystemInterruptBase) {
	first = new g_ioapic(id, ioapicAddress, globalSystemInterruptBase, first);
}

/**
 *
 */
bool g_ioapic_manager::areAvailable() {
	return first != 0;
}

/**
 *
 */
g_ioapic* g_ioapic_manager::getResponsibleFor(uint32_t source) {
	g_ioapic* n = first;
	while (n) {
		if (source >= n->getGlobalSystemInterruptBase() && source < (n->getGlobalSystemInterruptBase() + n->getRedirectEntryCount())) {
			break;
		}
		n = n->getNext();
	}
	return n;
}

/**
 *
 */
bool g_ioapic_manager::createIsaRedirectionEntry(uint32_t source, uint32_t irq, uint32_t destinationApic) {

	g_ioapic* n = getResponsibleFor(source);

	if (n) {
		uint64_t redirectionTableEntry = 0;
		redirectionTableEntry |= IOAPIC_REDTBL_INTVEC_MAKE(0x20 + irq);
		redirectionTableEntry |= IOAPIC_REDTBL_DELMOD_FIXED;
		redirectionTableEntry |= IOAPIC_REDTBL_DESTMOD_PHYSICAL;
		redirectionTableEntry |= IOAPIC_REDTBL_INTPOL_HIGH_ACTIVE;
		redirectionTableEntry |= IOAPIC_REDTBL_TRIGGERMOD_EDGE;
		redirectionTableEntry |= IOAPIC_REDTBL_INTMASK_UNMASKED;
		redirectionTableEntry |= IOAPIC_REDTBL_DESTINATION_MAKE(destinationApic, IOAPIC_REDTBL_DESTINATION_FLAG_PHYSICAL);

		n->setRedirectionEntry(source, redirectionTableEntry);

		g_log_debug("%! wrote ISA redirection entry %i -> %i", "ioapicmgr", source, irq);
		return true;
	} else {
		g_log_warn("%! found no response I/O APIC for interrupt %i", "ioapicmgr", source);
		return false;
	}
}

/**
 *
 */
void g_ioapic_manager::maskIrq(uint32_t irq) {
	g_ioapic* n = getResponsibleFor(irq);

	if (n) {
		g_log_debug("masking irq %i", irq);
		n->mask(irq);
	} else {
		g_log_warn("%! tried to mask IRQ %i that no IOAPIC is responsible for", "ioapicmgr", irq);
	}
}

/**
 *
 */
void g_ioapic_manager::unmaskIrq(uint32_t irq) {
	g_ioapic* n = getResponsibleFor(irq);

	if (n) {
		g_log_debug("unmasking irq %i", irq);
		n->unmask(irq);
	} else {
		g_log_warn("%! tried to unmask IRQ %i that no IOAPIC is responsible for", "ioapicmgr", irq);
	}
}

