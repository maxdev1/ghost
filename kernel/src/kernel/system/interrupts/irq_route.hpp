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

#ifndef G_KERNEL_INTERRUPTS_IRQ_ROUTE
#define G_KERNEL_INTERRUPTS_IRQ_ROUTE

#include <ghost/system/irq.h>

constexpr uint32_t G_IRQ_VECTOR_COUNT = 96;

struct g_irq_route
{
	uint32_t gsi = 0;
	uint8_t irqOffset = 0;
	bool activeLow = false;
	bool level = false;
	uint64_t previous = 0;
};

enum class g_irq_end
{
	RELEASE,
	OWNER_EXIT
};

/**
 * Resolves an ISA line, PCI INTx GSI or ACPI SCI line into its GSI, vector
 * offset, polarity and trigger mode. Rejects invalid, reserved or unsupported
 * sources without changing the interrupt controller configuration.
 */
g_irq_status irqRouteResolve(g_irq_source source, uint32_t line, g_irq_route* route);

/**
 * Saves the previous controller configuration and installs a masked route.
 * IOAPIC routes target the BSP. Returns busy if the entry or vector is in use,
 * or not supported if the controller cannot provide the resolved route.
 */
g_irq_status irqRouteInstall(g_irq_route* route);

/**
 * Masks the installed route when masked is true, or unmasks it otherwise,
 * using the IOAPIC or legacy PIC.
 */
void irqRouteMask(const g_irq_route& route, bool masked);

/**
 * Returns true only on the BSP when the IOAPIC route is masked and neither
 * it nor the destination LAPIC vector has an interrupt delivery outstanding.
 * Requires an installed IOAPIC route and local interrupts disabled.
 */
bool irqRouteIsIdleOnBsp(const g_irq_route& route);

/**
 * Restores the saved controller configuration after releasing a route.
 * On PIC owner exit, keeps level-triggered lines masked; restores the cascade
 * mask when the last slave binding ends. IOAPIC delivery must have drained first.
 */
void irqRouteRestore(const g_irq_route& route, g_irq_end ending);

#endif
