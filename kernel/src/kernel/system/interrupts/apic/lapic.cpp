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

#include "kernel/system/interrupts/apic/lapic.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/panic.hpp"
#include "kernel/system/acpi/acpi.hpp"
#include "kernel/system/configuration.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/timing/pit.hpp"
#include "kernel/tasking/clock.hpp"

static bool available = false;
static bool hypervisorEoi;

enum class g_lapic_timer_mode
{
	PERIODIC,
	STOPPED,
	ONE_SHOT
};

struct g_lapic_timer
{
	// Downcounter frequency after the divide-by-16 prescaler
	uint64_t ticksPerSecond = 0;
	uint32_t periodicCount = 0;
	g_lapic_timer_mode mode = g_lapic_timer_mode::STOPPED;
};

struct g_lapic_calibration_sample
{
	uint32_t count;
	uint64_t before;
	uint64_t after;
	bool timed;
};

static constexpr uint32_t G_LAPIC_TIMER_CAPACITY = 256;
static g_lapic_timer timers[G_LAPIC_TIMER_CAPACITY];

// All APICs are at the same physical and virtual address
static g_physical_address physicalBase = 0;
static g_virtual_address virtualBase = 0;


void lapicSetup(g_physical_address address)
{
	physicalBase = address;

	if(physicalBase != G_EXPECTED_APIC_PHYSICAL_ADDRESS)
		logWarn("%! is at %h, not %h as expected", "lapic", physicalBase, G_EXPECTED_APIC_PHYSICAL_ADDRESS);

	logDebug("%! base is %h", "lapic", physicalBase);

	lapicCreateMapping();
	available = true;

	// Hv#1 interface advertises accelerated APIC register MSRs
	hypervisorEoi = processorHasHypervFeature(1u << 4);
	if(hypervisorEoi)
		logInfo("%! using Hyper-V EOI MSR", "lapic");
}

static g_lapic_timer* lapicGetTimer()
{
	uint32_t processor = processorGetCurrentId();
	if(processor >= G_LAPIC_TIMER_CAPACITY)
		panic("%! processor %i exceeds local timer capacity", "lapic", processor);

	return &timers[processor];
}

bool lapicIsAvailable() { return available; }

void lapicInitialize()
{
	// Read version
#if G_LOGGING_DEBUG
	uint32_t apicVersionRegVal = lapicRead(APIC_REGISTER_VERSION);

	uint32_t localId = lapicReadId();
	uint8_t apicVersion = apicVersionRegVal & 0xFF;
	uint16_t maxLvtIndex = (apicVersionRegVal >> 16) & 0xFF;
	logDebug("%! id %i, version %h (%s), maxlvtindex: %i", "lapic", localId, apicVersion,
			 (apicVersion < 0x10 ? "82489DX discrete" : "integrated"), maxLvtIndex);
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
		panic("%! could not get a virtual range for mapping", "apic");

	// "APIC registers are memory-mapped to a 4-KByte region of the processor’s physical
	// address space with an initial starting address of FEE00000H." - x86 System Programming Manual, 10.4.1
	pagingMapPage(virtualBase, physicalBase, G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_TABLE_KERNEL_DEFAULT,
				  G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_UNCACHED);
}

uint32_t lapicReadId()
{
	if(!available)
		return 0;
	return (lapicRead(APIC_REGISTER_ID) >> 24) & 0xFF;
}

uint32_t lapicRead(uint32_t reg) { return *((volatile uint32_t*) (virtualBase + reg)); }

bool lapicVectorIsBusy(uint8_t vector)
{
	constexpr uint32_t inService = 0x100;
	constexpr uint32_t requested = 0x200;
	uint32_t offset = (vector / 32) * 0x10;
	uint32_t bit = 1u << (vector % 32);
	return ((lapicRead(requested + offset) | lapicRead(inService + offset)) & bit) != 0;
}

void lapicWrite(uint32_t reg, uint32_t value) { *((volatile uint32_t*) (virtualBase + reg)) = value; }

static g_lapic_calibration_sample lapicSampleTimer()
{
	uint64_t before, after;
	const bool timed = clockReadCalibrationNanos(&before);
	const uint32_t count = lapicRead(APIC_REGISTER_TIMER_CURRCNT);
	if(timed && clockReadCalibrationNanos(&after) && after >= before)
		return {count, before, after, true};
	return {count, 0, 0, false};
}

static bool lapicFrequencyUsable(uint64_t frequency)
{
	return frequency >= G_TIMER_FREQUENCY && frequency <= uint64_t(UINT32_MAX) * G_TIMER_FREQUENCY;
}

static uint64_t lapicMeasureFrequency()
{
	lapicWrite(APIC_REGISTER_LVT_TIMER, APIC_LVT_INT_MASKED);
	uint64_t nominal = 0;
	uint64_t bestFrequency = 0, bestWidth = 0, bestShortest = 0;
	for(unsigned attempt = 0; attempt < 3; ++attempt)
	{
		pitPrepareSleep(10000);
		lapicWrite(APIC_REGISTER_TIMER_INITCNT, UINT32_MAX);

		const auto start = lapicSampleTimer();
		pitPerformSleep();
		const auto end = lapicSampleTimer();

		// A one-shot parked at zero no longer records the full elapsed count
		if(end.count == 0 || end.count >= start.count)
			continue;

		const uint64_t ticks = start.count - end.count;

		// Preserve the legacy PIT estimate for machines without a usable
		// reference. Of the finite counts, the smallest has the least overrun:
		const uint64_t pitFrequency = ticks * 100;
		if(lapicFrequencyUsable(pitFrequency) && (!nominal || pitFrequency < nominal))
			nominal = pitFrequency;
		if(!start.timed || !end.timed || end.before <= start.after)
			continue;

		// The true interval lies between the inner and outer reference reads.
		// A width of 2% bounds the midpoint error to 1%. Retry wider brackets
		// and retain the best one whose midpoint error is bounded by 5%
		uint64_t shortest = end.before - start.after;
		uint64_t longest = end.after - start.before;
		uint64_t uncertainty = longest - shortest;
		if(uncertainty > shortest / 10)
			continue;

		uint64_t elapsed = shortest + uncertainty / 2;
		uint64_t frequency = ticks * 1000000000ULL / elapsed;
		if(!lapicFrequencyUsable(frequency))
			continue;

		if(uncertainty <= shortest / 50)
			return frequency;

		if(!bestFrequency || (unsigned __int128) uncertainty * bestShortest < (unsigned __int128) bestWidth * shortest)
		{
			bestFrequency = frequency;
			bestWidth = uncertainty;
			bestShortest = shortest;
		}
	}
	return bestFrequency ? bestFrequency : nominal;
}

void lapicStartTimer()
{
	auto timer = lapicGetTimer();
	lapicWrite(APIC_REGISTER_TIMER_DIV, 0x3); // Divide APIC bus clock by 16

	uint64_t frequency = processorGetApicFrequencyHz() / 16;
	if(!lapicFrequencyUsable(frequency))
		frequency = lapicMeasureFrequency();

	if(!lapicFrequencyUsable(frequency))
		panic("%! processor %i local timer calibration returned no usable ticks", "lapic", processorGetCurrentId());

	timer->ticksPerSecond = frequency;
	timer->periodicCount = (frequency + G_TIMER_FREQUENCY / 2) / G_TIMER_FREQUENCY;
	lapicWrite(APIC_REGISTER_LVT_TIMER, 0x20 | APIC_LVT_TIMER_MODE_PERIODIC);
	lapicWrite(APIC_REGISTER_TIMER_INITCNT, timer->periodicCount);
	timer->mode = g_lapic_timer_mode::PERIODIC;
}

void lapicConfigureTimer(bool idle, uint64_t delayMilliseconds)
{
	auto timer = lapicGetTimer();
	if(!timer->periodicCount)
		panic("%! processor %i local timer is not calibrated", "lapic", processorGetCurrentId());

	if(!idle)
	{
		if(timer->mode == g_lapic_timer_mode::PERIODIC)
			return;

		lapicWrite(APIC_REGISTER_TIMER_DIV, 0x3);
		lapicWrite(APIC_REGISTER_LVT_TIMER, 0x20 | APIC_LVT_TIMER_MODE_PERIODIC);
		lapicWrite(APIC_REGISTER_TIMER_INITCNT, timer->periodicCount);
		timer->mode = g_lapic_timer_mode::PERIODIC;
		return;
	}

	if(!delayMilliseconds)
	{
		if(timer->mode == g_lapic_timer_mode::STOPPED)
			return;

		lapicWrite(APIC_REGISTER_LVT_TIMER, 0x20 | APIC_LVT_INT_MASKED);
		lapicWrite(APIC_REGISTER_TIMER_INITCNT, 0);
		timer->mode = g_lapic_timer_mode::STOPPED;
		return;
	}

	uint64_t count = UINT32_MAX;
	if(delayMilliseconds <= UINT64_MAX / timer->ticksPerSecond)
		count = delayMilliseconds * timer->ticksPerSecond / 1000;
	if(count == 0)
		count = 1;
	else if(count > UINT32_MAX)
		count = UINT32_MAX;

	lapicWrite(APIC_REGISTER_TIMER_DIV, 0x3);
	lapicWrite(APIC_REGISTER_LVT_TIMER, 0x20 | APIC_LVT_TIMER_MODE_ONESHOT);
	lapicWrite(APIC_REGISTER_TIMER_INITCNT, (uint32_t) count);
	timer->mode = g_lapic_timer_mode::ONE_SHOT;
}

void lapicSendEndOfInterrupt()
{
	if(hypervisorEoi)
	{
		processorWriteMsr(0x40000070, 0, 0);
		return;
	}
	lapicWrite(APIC_REGISTER_EOI, 0);
}

void lapicWaitForIcrSend()
{
	while(lapicRead(APIC_REGISTER_INT_COMMAND_LOW) & APIC_ICR_DELIVS_SEND_PENDING)
	{
		asm volatile("pause");
	}
}

void lapicSendIpi(uint32_t apicId, uint8_t vector)
{
	g_interrupt_guard interrupts;

	lapicWaitForIcrSend();
	lapicWrite(APIC_REGISTER_INT_COMMAND_HIGH, apicId << 24);
	lapicWrite(APIC_REGISTER_INT_COMMAND_LOW, vector | APIC_ICR_LEVEL_ASSERT);
	lapicWaitForIcrSend();
}

void lapicSendIpiAllOthers(uint8_t vector)
{
	g_interrupt_guard interrupts;

	lapicWaitForIcrSend();
	lapicWrite(APIC_REGISTER_INT_COMMAND_LOW, vector | APIC_ICR_LEVEL_ASSERT | APIC_ICR_DEST_SHORTHAND_ALL_NOT_SELF);
	lapicWaitForIcrSend();
}
