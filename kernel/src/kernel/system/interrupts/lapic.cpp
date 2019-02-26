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

#include "kernel/system/interrupts/lapic.hpp"
#include "kernel/system/timing/pit.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/kernel.hpp"

static bool globalPrepared = false;

// All APICs are at the same physical and virtual address
static g_physical_address physicalBase = 0;
static g_virtual_address virtualBase = 0;

void lapicGlobalPrepare(g_physical_address lapicAddress)
{
	physicalBase = lapicAddress;

	// Warn if APIC not at expected location
	if(physicalBase != G_EXPECTED_APIC_PHYSICAL_ADDRESS)
		logWarn("%! is at %h, not %h as expected", "lapic", physicalBase, G_EXPECTED_APIC_PHYSICAL_ADDRESS);

	logDebug("%! base is %h", "lapic", physicalBase);

	// Map it to virtual space
	lapicCreateMapping();
	globalPrepared = true;
}

bool lapicGlobalIsPrepared()
{
	return globalPrepared;
}

void lapicInitialize()
{
	// Read version
#if G_LOGGING_DEBUG
	uint32_t apicVersionRegVal = lapicRead(APIC_REGISTER_VERSION);

	uint32_t localId = lapicReadId();
	uint8_t apicVersion = apicVersionRegVal & 0xFF;
	uint16_t maxLvtIndex = (apicVersionRegVal >> 16) & 0xFF;
	logDebug("%! id %i, version %h (%s), maxlvtindex: %i", "lapic", localId,
			apicVersion,
			(apicVersion < 0x10 ? "82489DX discrete" : "integrated"),
			maxLvtIndex);
#endif

	// Initialize APIC to well-known state
	lapicWrite(APIC_REGISTER_DEST_FORMAT, 0xFFFFFFFF);
	lapicWrite(APIC_REGISTER_LOGICAL_DEST, (lapicRead(APIC_REGISTER_LOGICAL_DEST) & 0x00FFFFFF) | 1);
	lapicWrite(APIC_REGISTER_LVT_TIMER, APIC_LVT_INT_MASKED);
	lapicWrite(APIC_REGISTER_LVT_PERFMON, APIC_LVT_DELIVERY_MODE_NMI);
	lapicWrite(APIC_REGISTER_LVT_LINT0, APIC_LVT_INT_MASKED);
	lapicWrite(APIC_REGISTER_LVT_LINT1, APIC_LVT_INT_MASKED);
	lapicWrite(APIC_REGISTER_TASK_PRIO, 0);

	// Enable the APIC
	lapicWrite(APIC_REGISTER_SPURIOUS_IVT, 0xFF | APIC_SPURIOUS_IVT_SOFTWARE_ENABLE);

	// Set up timer
	lapicStartTimer();
}

void lapicCreateMapping()
{
	// Map it to virtual space
	virtualBase = addressRangePoolAllocate(memoryVirtualRangePool, 1);
	if(!virtualBase)
		kernelPanic("%! could not get a virtual range for mapping", "apic");

	// "APIC registers are memory-mapped to a 4-KByte region of the processor’s physical
	// address space with an initial starting address of FEE00000H." - x86 System Programming Manual, 10.4.1
	pagingMapPage(virtualBase, physicalBase, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS | G_PAGE_CACHE_DISABLED);
}

uint32_t lapicRead(uint32_t reg)
{
	return *((volatile uint32_t*) (virtualBase + reg));
}

void lapicWrite(uint32_t reg, uint32_t value)
{
	*((volatile uint32_t*) (virtualBase + reg)) = value;
}

void lapicStartTimer()
{
	logDebug("%! starting timer", "lapic");

	// Tell APIC timer to use divider 16
	lapicWrite(APIC_REGISTER_TIMER_DIV, 0x3);

	// Prepare the PIT to sleep for 10ms (10000µs)
	pitPrepareSleep(10000);

	// Set APIC init counter to -1
	lapicWrite(APIC_REGISTER_TIMER_INITCNT, 0xFFFFFFFF);

	// Perform PIT-supported sleep
	pitPerformSleep();

	// Stop the APIC timer
	lapicWrite(APIC_REGISTER_LVT_TIMER, APIC_LVT_INT_MASKED);

	// Now we know how often the APIC timer has ticked in 10ms
	uint32_t ticksPer10ms = 0xFFFFFFFF - lapicRead(APIC_REGISTER_TIMER_CURRCNT);

	// Start timer as periodic on IRQ 0
	lapicWrite(APIC_REGISTER_LVT_TIMER, 32 | APIC_LVT_TIMER_MODE_PERIODIC);
	lapicWrite(APIC_REGISTER_TIMER_DIV, 0x3);
	lapicWrite(APIC_REGISTER_TIMER_INITCNT, ticksPer10ms / 10);
}

uint32_t lapicReadId()
{
	if(!globalPrepared)
		return 0;
	return (lapicRead(APIC_REGISTER_ID) >> 24) & 0xFF;
}

void lapicSendEndOfInterrupt()
{
	lapicWrite(APIC_REGISTER_EOI, 0);
}

void lapicWaitForIcrSend()
{
	while(APIC_LVT_GET_DELIVERY_STATUS(lapicRead(APIC_REGISTER_INT_COMMAND_HIGH)) == APIC_ICR_DELIVS_SEND_PENDING)
	{
	}
}

