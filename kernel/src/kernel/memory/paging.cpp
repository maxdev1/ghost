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

#include "kernel/memory/paging.hpp"
#include "kernel/memory/paging_shootdown.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/memory/constants.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/panic.hpp"

g_physical_address pagingVirtualToPageEntry(g_virtual_address addr)
{
    auto table = (uint64_t*) G_MEM_PHYS_TO_VIRT(pagingGetCurrentSpace());
    const unsigned shifts[] = {39, 30, 21, 12};
    for(unsigned level = 0; level < 4; ++level)
    {
        const auto entry = __atomic_load_n(&table[(addr >> shifts[level]) & 511], __ATOMIC_ACQUIRE);
        if(!(entry & G_PAGE_PRESENT))
            return 0;
        if(level == 3 || ((level == 1 || level == 2) && (entry & G_PAGE_LARGE_PAGE_FLAG)))
            return entry;
        table = (uint64_t*) G_MEM_PHYS_TO_VIRT(entry & G_PAGE_PHYSICAL_MASK);
    }
    return 0;
}

g_physical_address pagingVirtualToPhysical(g_virtual_address addr)
{
	constexpr uint64_t addressMask4KiB = 0x000FFFFFFFFFF000ULL;
	constexpr uint64_t addressMask2MiB = 0x000FFFFFFFE00000ULL;
	constexpr uint64_t addressMask1GiB = 0x000FFFFFC0000000ULL;
	constexpr uint64_t offsetMask2MiB = (1ULL << 21) - 1;
	constexpr uint64_t offsetMask1GiB = (1ULL << 30) - 1;

	auto pml4 = (g_address*) G_MEM_PHYS_TO_VIRT(pagingGetCurrentSpace());
	uint64_t pml4Index = G_PML4_INDEX(addr);
	uint64_t pml4Value = __atomic_load_n(&pml4[pml4Index], __ATOMIC_ACQUIRE);
	if(!(pml4Value & G_PAGE_PRESENT))
		return 0;
	uint64_t pdptAddr = pml4Value & addressMask4KiB;
	if(!pdptAddr)
		return 0;

	auto pdpt = (g_address*) G_MEM_PHYS_TO_VIRT(pdptAddr);
	uint64_t pdptIndex = G_PDPT_INDEX(addr);
	uint64_t pdptValue = __atomic_load_n(&pdpt[pdptIndex], __ATOMIC_ACQUIRE);
	if(!(pdptValue & G_PAGE_PRESENT))
		return 0;
	if(pdptValue & G_PAGE_LARGE_PAGE_FLAG)
		return (pdptValue & addressMask1GiB) + (addr & offsetMask1GiB);

	uint64_t pdAddr = pdptValue & addressMask4KiB;
	if(!pdAddr)
		return 0;

	auto pd = (g_address*) G_MEM_PHYS_TO_VIRT(pdAddr);
	uint64_t pdIndex = G_PD_INDEX(addr);
	uint64_t pdValue = __atomic_load_n(&pd[pdIndex], __ATOMIC_ACQUIRE);
	if(!(pdValue & G_PAGE_PRESENT))
		return 0;
	if(pdValue & G_PAGE_LARGE_PAGE_FLAG)
		return (pdValue & addressMask2MiB) + (addr & offsetMask2MiB);

	uint64_t ptAddr = pdValue & addressMask4KiB;
	if(!ptAddr)
		return 0;

	auto pt = (g_address*) G_MEM_PHYS_TO_VIRT(ptAddr);
	uint64_t ptIndex = G_PT_INDEX(addr);
	uint64_t ptValue = __atomic_load_n(&pt[ptIndex], __ATOMIC_ACQUIRE);
	if(!(ptValue & G_PAGE_PRESENT))
		return 0;

	return (ptValue & addressMask4KiB) + (addr & G_PAGE_ALIGN_MASK);
}

void pagingSwitchToSpace(g_physical_address dir)
{
	asm volatile("mov %0, %%cr3" : : "b"(dir));
}

static volatile uint64_t* _pagingGetOrCreateTable(volatile uint64_t* parent, uint64_t index, uint64_t flags)
{
	uint64_t entry = __atomic_load_n(&parent[index], __ATOMIC_ACQUIRE);
	if(!entry)
	{
		auto physical = bitmapPageAllocatorTryAllocate(&memoryPhysicalAllocator, 0);
		if(!physical)
			return nullptr;
		auto table = (uint64_t*) G_MEM_PHYS_TO_VIRT(physical);
		for(int i = 0; i < 512; ++i)
			table[i] = 0;

		uint64_t expected = 0;
		entry = physical | flags;
		if(!__atomic_compare_exchange_n(&parent[index], &expected, entry, false,
		                                __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
		{
			bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, physical);
			entry = expected;
		}
	}
    if(!(entry & G_PAGE_PRESENT) || (entry & G_PAGE_LARGE_PAGE_FLAG))
        return nullptr;
    return (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(entry & G_PAGE_PHYSICAL_MASK);
}

bool pagingMapPage(g_virtual_address virt, g_physical_address phys,
                   uint64_t tableFlags, uint64_t ptFlags,
                   bool allowOverride)
{
	return pagingMapPage(virt, phys, tableFlags, tableFlags, tableFlags, ptFlags, allowOverride);
}

bool pagingMapPage(g_virtual_address virt, g_physical_address phys,
                   uint64_t pdptFlags, uint64_t pdFlags,
                   uint64_t ptFlags, uint64_t pageFlags,
                   bool allowOverride)
{
	if((virt & G_PAGE_ALIGN_MASK) || (phys & G_PAGE_ALIGN_MASK))
		panic("%! tried to map unaligned addresses: %h -> %h", "paging", virt, phys);

	auto pml4 = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(pagingGetCurrentSpace());


	// Kernel tables are shared by every address space and can grow on any core.
	// Initialize child tables before publishing them; a losing creator frees its
	// unused page instead of overwriting another core's mappings.
	auto pdpt = _pagingGetOrCreateTable(pml4, G_PML4_INDEX(virt), pdptFlags);
	if(!pdpt)
		return false;
	auto pd = _pagingGetOrCreateTable(pdpt, G_PDPT_INDEX(virt), pdFlags);
	if(!pd)
		return false;
	auto pt = _pagingGetOrCreateTable(pd, G_PD_INDEX(virt), ptFlags);
	if(!pt)
		return false;

	// Write page into page table
	uint64_t ptIndex = G_PT_INDEX(virt);
	uint64_t expected = 0;
	uint64_t entry = phys | pageFlags;
	if(allowOverride)
		__atomic_store_n(&pt[ptIndex], entry, __ATOMIC_RELEASE);
	else if(!__atomic_compare_exchange_n(&pt[ptIndex], &expected, entry, false,
	                                      __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
	{
		logInfo("%! failed to write paging entry for %x since it is already set to: %x", "paging", virt, expected);
		return false;
	}

	if(allowOverride)
		pagingSynchronizePage(virt);
	else
		pagingInvalidatePage(virt);
	return true;
}

void pagingUnmapPage(g_virtual_address virt)
{
    auto table = (uint64_t*) G_MEM_PHYS_TO_VIRT(pagingGetCurrentSpace());
    const unsigned shifts[] = {39, 30, 21};
    for(unsigned level = 0; level < 3; ++level)
    {
        const auto entry = __atomic_load_n(&table[(virt >> shifts[level]) & 511], __ATOMIC_ACQUIRE);
        if(!(entry & G_PAGE_PRESENT))
            return;
        if(level && (entry & G_PAGE_LARGE_PAGE_FLAG))
            panic("%! attempted 4 KiB unmap inside a huge mapping", "paging");
        table = (uint64_t*) G_MEM_PHYS_TO_VIRT(entry & G_PAGE_PHYSICAL_MASK);
    }
    if(__atomic_exchange_n(&table[G_PT_INDEX(virt)], 0, __ATOMIC_ACQ_REL))
        pagingSynchronizePage(virt);
}

g_physical_address pagingGetCurrentSpace()
{
	g_physical_address directory;
	asm volatile("mov %%cr3, %0" : "=r"(directory));
	return directory;
}
