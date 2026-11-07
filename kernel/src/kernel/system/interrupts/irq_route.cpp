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

#include "kernel/system/interrupts/irq_route.hpp"
#include "kernel/system/acpi/irq_routing.hpp"
#include "kernel/system/interrupts/apic/ioapic.hpp"
#include "kernel/system/interrupts/apic/lapic.hpp"
#include "kernel/system/interrupts/pic.hpp"
#include "kernel/system/processor/processor.hpp"

static uint32_t irqPicSlaveBindings;
static bool irqPicCascadeWasMasked;

g_irq_status irqRouteResolve(g_irq_source source, uint32_t line, g_irq_route* route)
{
	if(!line || line >= G_IRQ_VECTOR_COUNT)
		return G_IRQ_INVALID_ARGUMENT;

	route->gsi = line;
	route->irqOffset = line;

	switch(source)
	{
		case G_IRQ_SOURCE_ISA:
			if(line >= 16 || line == 2)
				return G_IRQ_INVALID_ARGUMENT;

			if(ioapicAreAvailable() && !acpiIrqResolveIsa(line, &route->gsi, &route->activeLow, &route->level))
				return G_IRQ_NOT_SUPPORTED;

			break;

		case G_IRQ_SOURCE_PCI_INTX:
			if(!ioapicAreAvailable())
				return G_IRQ_NOT_SUPPORTED;

			route->level = route->activeLow = true;
			break;

		case G_IRQ_SOURCE_ACPI:
			route->level = route->activeLow = true;
			if(ioapicAreAvailable() && !acpiIrqResolveSci(line, &route->gsi, &route->activeLow))
				return G_IRQ_NOT_SUPPORTED;

			break;
		default:
			return G_IRQ_INVALID_ARGUMENT;
	}

	if(!acpiIrqAllows(source, route->gsi) || (source == G_IRQ_SOURCE_ISA && !acpiIrqAllows(source, line)) ||
	   (!ioapicAreAvailable() && (route->gsi == 2 || route->gsi >= 16)))
		return G_IRQ_NOT_SUPPORTED;

	return G_IRQ_SUCCESSFUL;
}

g_irq_status irqRouteInstall(g_irq_route* route)
{
	if(!ioapicAreAvailable())
	{
		if(!picSupportsTrigger(route->gsi, route->level))
			return G_IRQ_NOT_SUPPORTED;

		route->previous = picIsMasked(route->gsi);
		picMaskIrq(route->gsi);

		if(route->gsi >= 8 && irqPicSlaveBindings++ == 0)
		{
			irqPicCascadeWasMasked = picIsMasked(2);
			picUnmaskIrq(2);
		}
		return G_IRQ_SUCCESSFUL;
	}

	auto io = ioapicGetResponsibleFor(route->gsi);
	if(!io)
		return G_IRQ_NOT_SUPPORTED;

	uint32_t index = route->gsi - io->globalSystemInterruptBase;
	uint8_t vector = 0x20 + route->irqOffset;
	uint64_t previous = ioapicGetRedirectionEntry(io, index);
	if(!(previous & IOAPIC_REDTBL_INTMASK_MASKED) ||
	   (previous & (IOAPIC_REDTBL_DELIVS_SEND_PENDING | IOAPIC_REDTBL_REMOTEIRR_ACCEPTING)) ||
	   ioapicVectorIsBusyElsewhere(vector, route->gsi))
		return G_IRQ_BUSY;

	auto bsp = processorGetBsp();
	uint64_t entry = vector | IOAPIC_REDTBL_INTMASK_MASKED | (uint64_t(bsp ? bsp->apicId : 0) << 56);
	if(route->level)
		entry |= IOAPIC_REDTBL_TRIGGERMOD_LEVEL;
	if(route->activeLow)
		entry |= IOAPIC_REDTBL_INTPOL_LOW_ACTIVE;

	route->previous = previous;
	ioapicSetRedirectionEntry(io, index, entry);

	return G_IRQ_SUCCESSFUL;
}

void irqRouteMask(const g_irq_route& route, bool masked)
{
	if(ioapicAreAvailable())
	{
		if(masked)
			ioapicMaskIrq(route.gsi);
		else
			ioapicUnmaskIrq(route.gsi);
	}
	else if(masked)
		picMaskIrq(route.gsi);
	else
		picUnmaskIrq(route.gsi);
}

/**
 * The IOAPIC read flushes the mask write. Delivery status and remote IRR
 * alone cannot distinguish an edge already queued in the destination LAPIC.
 * Inspect that LAPIC with local interrupts disabled before recycling its vector.
 */
bool irqRouteIsIdleOnBsp(const g_irq_route& route)
{
	if(processorGetCurrentId() != 0)
		return false;
	auto io = ioapicGetResponsibleFor(route.gsi);

	uint64_t entry = ioapicGetRedirectionEntry(io, route.gsi - io->globalSystemInterruptBase);
	return (entry & IOAPIC_REDTBL_INTMASK_MASKED) &&
		   !(entry & (IOAPIC_REDTBL_DELIVS_SEND_PENDING | IOAPIC_REDTBL_REMOTEIRR_ACCEPTING)) &&
		   !lapicVectorIsBusy(0x20 + route.irqOffset);
}

void irqRouteRestore(const g_irq_route& route, g_irq_end ending)
{
	irqRouteMask(route, true);
	if(ioapicAreAvailable())
	{
		auto io = ioapicGetResponsibleFor(route.gsi);
		ioapicSetRedirectionEntry(io, route.gsi - io->globalSystemInterruptBase, route.previous);
	}
	else
	{
		if(!route.previous && (ending == g_irq_end::RELEASE || !route.level))
			picUnmaskIrq(route.gsi);
		if(route.gsi >= 8 && --irqPicSlaveBindings == 0 && irqPicCascadeWasMasked)
			picMaskIrq(2);
	}
}
