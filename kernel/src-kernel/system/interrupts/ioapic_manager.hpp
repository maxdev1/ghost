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

#ifndef GHOST_INTERRUPTS_IOAPICMANAGER
#define GHOST_INTERRUPTS_IOAPICMANAGER

#include "ghost/kernel.h"
#include "ghost/stdint.h"
#include <system/interrupts/ioapic.hpp>

/**
 * I/O APIC manager
 */
class g_ioapic_manager {
private:
	static g_ioapic* getResponsibleFor(uint32_t source);

public:
	static g_ioapic* getEntries();

	static void create(uint32_t id, g_physical_address ioapicAddress, uint32_t globalSystemInterruptBase);
	static bool areAvailable();

	static bool createIsaRedirectionEntry(uint32_t source, uint32_t irq, uint32_t destinationApic);
	static void maskIrq(uint32_t irq);
	static void unmaskIrq(uint32_t irq);

};

#endif
