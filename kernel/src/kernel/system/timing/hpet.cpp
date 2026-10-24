/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "kernel/system/timing/hpet.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/system/acpi/acpi.hpp"
#include "kernel/logger/logger.hpp"

static volatile uint64_t* mmio = nullptr;
static bool available = false;
static uint64_t periodFemtoseconds = 0;

void _hpetFindAndMap();

void hpetInitialize()
{
	_hpetFindAndMap();

	if(!mmio)
		return;

	uint64_t capabilities = mmio[HPET_GEN_CAP_REG / 8];
	uint32_t clockPeriod = capabilities >> 32;
	if(!(capabilities & HPET_CAP_COUNTER_64BIT) || !(capabilities & 0xFF) ||
	   clockPeriod == 0 || clockPeriod > HPET_MAX_PERIOD_FEMTOSECONDS)
	{
		logInfo("%! unsupported counter or clock period, using timer ticks", "hpet");
		return;
	}

	periodFemtoseconds = clockPeriod;

	// Use only the free-running counter. Keep PIT/RTC routing intact and disable
	// comparator interrupts left enabled by firmware before starting the counter.
	mmio[HPET_GEN_CONFIG_REG / 8] = 0;
	uint32_t timerCount = ((capabilities >> 8) & 0x1F) + 1;
	for(uint32_t timer = 0; timer < timerCount; timer++)
	{
		uint32_t config = HPET_TIMER0_CONFIG_REG + timer * HPET_TIMER_REGISTER_STRIDE;
		mmio[config / 8] &= ~HPET_TIMER_INTERRUPT_ENABLE;
	}
	mmio[HPET_GEN_CONFIG_REG / 8] = HPET_CONFIG_ENABLE;
	available = (mmio[HPET_GEN_CONFIG_REG / 8] & HPET_CONFIG_ENABLE) != 0;
	if(available)
		logInfo("%! enabled %i-fs clock period", "hpet", clockPeriod);
}

void _hpetFindAndMap()
{
	auto entry = acpiGetEntryWithSignature("HPET");
	if(!entry)
	{
		logInfo("%! not present, timing will be inaccurate", "hpet");
		return;
	}

	if(entry->header->length < sizeof(g_acpi_hpet))
	{
		logInfo("%! invalid ACPI table", "hpet");
		return;
	}

	auto hpet = (g_acpi_hpet*) entry->header;
	if(hpet->baseAddress.addressSpace != 0)
	{
		logInfo("%! only supported with MMIO", "hpet");
		return;
	}

	if(!hpet->baseAddress.address || (hpet->baseAddress.address & G_PAGE_ALIGN_MASK))
	{
		logInfo("%! invalid MMIO base", "hpet");
		return;
	}

	auto virtBase = addressRangePoolAllocate(memoryVirtualRangePool, 1);
	if(!virtBase)
		return;
	if(!pagingMapPage(virtBase, hpet->baseAddress.address, G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_UNCACHED))
	{
		addressRangePoolFree(memoryVirtualRangePool, virtBase);
		logInfo("%! failed to map", "hpet");
		return;
	}

	mmio = (volatile uint64_t*) virtBase;
}

bool hpetIsAvailable()
{
	return available;
}

uint64_t hpetGetNanos()
{
	if(!available)
		return 0;

	uint64_t counterValue = mmio[HPET_MAIN_COUNTER_REG / 8];
	// Divide first so the counter * femtoseconds product cannot overflow after
	// only a few hours. Retain the remainder to preserve sub-tick precision.
	return (counterValue / HPET_FEMTOSECONDS_PER_NANOSECOND) * periodFemtoseconds +
	       ((counterValue % HPET_FEMTOSECONDS_PER_NANOSECOND) * periodFemtoseconds) /
	       HPET_FEMTOSECONDS_PER_NANOSECOND;
}
