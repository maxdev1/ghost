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

#include "kernel/system/acpi/platform.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/system/acpi/acpi.hpp"
#include "kernel/system/acpi/irq_routing.hpp"
#include "kernel/tasking/tasking.hpp"

static uint64_t acpiPlatformRsdp;

void acpiPlatformInitialize(uint64_t rsdp)
{
	acpiPlatformRsdp = rsdp;

	auto fadt = acpiGetEntryWithSignature("FACP");
	auto madt = acpiGetEntryWithSignature("APIC");
	acpiIrqConfigure(fadt ? fadt->header : nullptr, madt ? madt->header : nullptr);
}

uint64_t acpiPlatformGetRsdp() { return acpiPlatformRsdp; }

uint64_t acpiPlatformMap(g_task* task, uint64_t address, uint32_t length)
{
	if(!length || length > UINT32_MAX - G_PAGE_SIZE || !memoryPhysicalIsRangeAddressable(address, length))
		return 0;

	auto offset = address & (G_PAGE_SIZE - 1);
	uint32_t pages = G_PAGE_ALIGN_UP(uint64_t(length) + offset) / G_PAGE_SIZE;
	auto pool = task->process->virtualRangePool;
	g_mutex_guard guard(&task->process->lock);

	auto base = addressRangePoolAllocate(pool, pages, G_PROC_VIRTUAL_RANGE_FLAG_WEAK);
	if(!base)
		return 0;

	uint32_t mapped = 0;
	for(; mapped < pages; ++mapped)
	{
		if(!pagingMapPage(base + mapped * G_PAGE_SIZE, G_PAGE_ALIGN_DOWN(address) + mapped * G_PAGE_SIZE,
						  G_PAGE_TABLE_USER_DEFAULT,
						  G_PAGE_USER_DEFAULT | G_PAGE_CACHE_DISABLE | G_PAGE_UNMANAGED_FLAG))
			break;
	}

	if(mapped != pages)
	{
		while(mapped)
			pagingUnmapPage(base + --mapped * G_PAGE_SIZE);
		addressRangePoolFree(pool, base);
		return 0;
	}

	return base + offset;
}
