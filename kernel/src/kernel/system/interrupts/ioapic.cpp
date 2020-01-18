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

#include "kernel/system/interrupts/ioapic.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/kernel.hpp"

static g_ioapic* ioapicList = 0;

void ioapicInitializeAll()
{
	g_ioapic* io = ioapicList;
	while(io)
	{
		ioapicInitialize(io);
		io = io->next;
	}
}

void ioapicCreate(uint32_t id, g_physical_address physicalAddress, uint32_t globalSystemInterruptBase)
{
	g_ioapic* io = (g_ioapic*) heapAllocate(sizeof(g_ioapic));
	io->id = id;
	io->physicalAddress = physicalAddress;
	io->globalSystemInterruptBase = globalSystemInterruptBase;
	io->next = ioapicList;
	ioapicList = io;
}

void ioapicInitialize(g_ioapic* io)
{
	ioapicCreateMapping(io);

	// Get ID
	uint32_t idValue = ioapicRead(io, IOAPIC_ID);
	uint32_t reportedId = (idValue >> 24) & 0xF;

	// If not right ID, reprogram it
	if(reportedId != io->id)
	{
		logWarn("%! has different ID (%i) than what ACPI reported (%i), reprogramming", "ioapic", io->id, reportedId);

		// Remove the actual ID bits
		idValue &= ~(0xF << 24);
		// Set new ID bits
		idValue |= (io->id & 0xF) << 24;
		// Write value
		ioapicWrite(io, IOAPIC_ID, idValue);
	}

	// Get version
	uint32_t versionValue = ioapicRead(io, IOAPIC_VER);
	io->redirectEntryCount = (versionValue >> 16) & 0xFF;
	logDebug("%! id %i: version %i, redirect entries: %i", "ioapic", io->id, versionValue & 0xFF, io->redirectEntryCount);
}

void ioapicCreateMapping(g_ioapic* io)
{
	// Get a virtual range for mapping
	io->virtualAddress = addressRangePoolAllocate(memoryVirtualRangePool, 2);
	if(io->virtualAddress == 0)
		kernelPanic("%! could not get a virtual range for mapping", "ioapic");

	// Add the physical offset to the virtual address
	io->virtualAddress += io->physicalAddress - G_PAGE_ALIGN_DOWN(io->physicalAddress);

	// Map the two pages
	logDebug("%! mapped at %h (phys %h)", "ioapic", io->virtualAddress, io->physicalAddress);
	pagingMapPage(G_PAGE_ALIGN_DOWN(io->virtualAddress), G_PAGE_ALIGN_DOWN(io->physicalAddress), DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	pagingMapPage(G_PAGE_ALIGN_DOWN(io->virtualAddress) + G_PAGE_SIZE, G_PAGE_ALIGN_DOWN(io->physicalAddress) + G_PAGE_SIZE, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
}

uint32_t ioapicRead(g_ioapic* io, uint32_t reg)
{
	*((volatile uint32_t*) (io->virtualAddress + IOAPIC_REGSEL)) = reg;
	return *((volatile uint32_t*) (io->virtualAddress + IOAPIC_REGWIN));
}

void ioapicWrite(g_ioapic* io, uint32_t reg, uint32_t value)
{
	*((volatile uint32_t*) (io->virtualAddress + IOAPIC_REGSEL)) = reg;
	*((volatile uint32_t*) (io->virtualAddress + IOAPIC_REGWIN)) = value;
}

uint64_t ioapicGetRedirectionEntry(g_ioapic* io, uint32_t index)
{
	*((volatile uint32_t*) (io->virtualAddress + IOAPIC_REGSEL)) = IOAPIC_REDTBL_BASE + index * 2;
	uint64_t lo = *((volatile uint32_t*) (io->virtualAddress + IOAPIC_REGWIN));
	*((volatile uint32_t*) (io->virtualAddress + IOAPIC_REGSEL)) = IOAPIC_REDTBL_BASE + index * 2 + 1;
	uint64_t hi = *((volatile uint32_t*) (io->virtualAddress + IOAPIC_REGWIN));

	return (hi << 32) | lo;
}

void ioapicSetRedirectionEntry(g_ioapic* io, uint32_t index, uint64_t value)
{
	*((volatile uint32_t*) (io->virtualAddress + IOAPIC_REGSEL)) = IOAPIC_REDTBL_BASE + index * 2;
	*((volatile uint32_t*) (io->virtualAddress + IOAPIC_REGWIN)) = value & 0xFFFFFFFF;
	*((volatile uint32_t*) (io->virtualAddress + IOAPIC_REGSEL)) = IOAPIC_REDTBL_BASE + index * 2 + 1;
	*((volatile uint32_t*) (io->virtualAddress + IOAPIC_REGWIN)) = value >> 32;
}

void ioapicMask(g_ioapic* io, uint32_t source)
{
	uint32_t entryIndex = source - io->globalSystemInterruptBase;
	uint64_t entry = ioapicGetRedirectionEntry(io, entryIndex);

	entry &= ~(IOAPIC_REDTBL_MASK_INTMASK);
	entry |= IOAPIC_REDTBL_INTMASK_MASKED;

	ioapicSetRedirectionEntry(io, entryIndex, entry);
}

void ioapicUnmask(g_ioapic* io, uint32_t source)
{
	uint32_t entryIndex = source - io->globalSystemInterruptBase;
	uint64_t entry = ioapicGetRedirectionEntry(io, entryIndex);

	entry &= ~(IOAPIC_REDTBL_MASK_INTMASK);
	entry |= IOAPIC_REDTBL_INTMASK_UNMASKED;

	ioapicSetRedirectionEntry(io, entryIndex, entry);
}

bool ioapicAreAvailable()
{
	return ioapicList != 0;
}

g_ioapic* ioapicGetResponsibleFor(uint32_t source)
{
	g_ioapic* io = ioapicList;
	while(io)
	{
		if(source >= io->globalSystemInterruptBase && source < (io->globalSystemInterruptBase + io->redirectEntryCount))
		{
			break;
		}
		io = io->next;
	}
	return io;
}

bool ioapicCreateIsaRedirectionEntry(uint32_t source, uint32_t irq, uint32_t destinationApic)
{
	g_ioapic* io = ioapicGetResponsibleFor(source);
	if(!io)
	{
		logWarn("%! found no responsible I/O APIC for interrupt %i", "ioapicmgr", source);
		return false;
	}

	uint64_t redirectionTableEntry = 0;
	redirectionTableEntry |= IOAPIC_REDTBL_INTVEC_MAKE(0x20 + irq);
	redirectionTableEntry |= IOAPIC_REDTBL_DELMOD_FIXED;
	redirectionTableEntry |= IOAPIC_REDTBL_DESTMOD_PHYSICAL;
	redirectionTableEntry |= IOAPIC_REDTBL_INTPOL_HIGH_ACTIVE;
	redirectionTableEntry |= IOAPIC_REDTBL_TRIGGERMOD_EDGE;
	redirectionTableEntry |= IOAPIC_REDTBL_INTMASK_UNMASKED;
	redirectionTableEntry |= IOAPIC_REDTBL_DESTINATION_MAKE(destinationApic, IOAPIC_REDTBL_DESTINATION_FLAG_PHYSICAL);

	ioapicSetRedirectionEntry(io, source, redirectionTableEntry);

	logDebug("%! wrote ISA redirection entry %i -> %i", "ioapicmgr", source, irq);
	return true;
}

void ioapicMaskIrq(uint32_t irq)
{
	g_ioapic* io = ioapicGetResponsibleFor(irq);
	if(io)
	{
		ioapicMask(io, irq);
	}
}

void ioapicUnmaskIrq(uint32_t irq)
{
	g_ioapic* io = ioapicGetResponsibleFor(irq);
	if(io)
	{
		ioapicUnmask(io, irq);
	}
}

