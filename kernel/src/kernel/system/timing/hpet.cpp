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
#include "shared/logger/logger.hpp"

static volatile uint64_t* mmio = nullptr;
static bool available = false;
static uint64_t frequency = HPET_DEFAULT_FREQUENCY;
static double periodsPerSecond = 1.0 / frequency;

void _hpetFindAndMap();

void hpetInitialize()
{
	_hpetFindAndMap();

	// TODO this crashes in QEMU
	if(mmio && false)
	{
		// Read correct frequency
		uint64_t capabilities = mmio[HPET_GEN_CAP_REG / 8];
		uint32_t clockPeriod = (capabilities >> 32) & 0xFFFFFFFF;
		frequency = (1000000000000000.0 / clockPeriod); // TODO now on x86_64 do it properly
		periodsPerSecond = 1.0 / frequency;

		// Make sure it is enabled
		mmio[HPET_GEN_CONFIG_REG / 8] |= 1;

		available = true;
	}
}

void _hpetFindAndMap()
{
	auto entry = acpiGetEntryWithSignature("HPET");
	if(!entry)
	{
		logInfo("%! not present, timing will be inaccurate", "hpet");
		return;
	}

	auto hpet = (g_acpi_hpet*) entry->header;
	if(hpet->baseAddress.addressSpace != 0)
	{
		logInfo("%! only supported with MMIO", "hpet");
		return;
	}

	auto virtBase = addressRangePoolAllocate(memoryVirtualRangePool, 1);
	if(!pagingMapPage(virtBase, hpet->baseAddress.address, G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_UNCACHED))
	{
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
	return (uint64_t) ((double) counterValue * periodsPerSecond * 1000000000);
}
