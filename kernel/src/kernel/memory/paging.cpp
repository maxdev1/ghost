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
#include "kernel/logger/logger.hpp"
#include "kernel/memory/constants.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/panic.hpp"

g_physical_address pagingVirtualToPageEntry(g_virtual_address addr)
{
	auto pml4 = (g_address*) G_MEM_PHYS_TO_VIRT(pagingGetCurrentSpace());
	uint64_t pml4Index = G_PML4_INDEX(addr);
	uint64_t pdptAddr = __atomic_load_n(&pml4[pml4Index], __ATOMIC_ACQUIRE) & ~G_PAGE_ALIGN_MASK;
	if(!pdptAddr)
		return 0;

	auto pdpt = (g_address*) G_MEM_PHYS_TO_VIRT(pdptAddr);
	uint64_t pdptIndex = G_PDPT_INDEX(addr);
	uint64_t pdAddr = __atomic_load_n(&pdpt[pdptIndex], __ATOMIC_ACQUIRE) & ~G_PAGE_ALIGN_MASK;
	if(!pdAddr)
		return 0;

	auto pd = (g_address*) G_MEM_PHYS_TO_VIRT(pdAddr);
	uint64_t pdIndex = G_PD_INDEX(addr);
	uint64_t pdValue = __atomic_load_n(&pd[pdIndex], __ATOMIC_ACQUIRE);
	uint64_t ptAddr = pdValue & ~G_PAGE_ALIGN_MASK;
	if(!ptAddr)
		return 0;

	if(pdValue & G_PAGE_LARGE_PAGE_FLAG)
		return pdValue & ~G_PAGE_ALIGN_MASK;

	auto pt = (g_address*) G_MEM_PHYS_TO_VIRT(ptAddr);
	uint64_t ptIndex = G_PT_INDEX(addr);
	return __atomic_load_n(&pt[ptIndex], __ATOMIC_ACQUIRE);
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
	uint64_t pdptAddr = __atomic_load_n(&pml4[pml4Index], __ATOMIC_ACQUIRE) & addressMask4KiB;
	if(!pdptAddr)
		return 0;

	auto pdpt = (g_address*) G_MEM_PHYS_TO_VIRT(pdptAddr);
	uint64_t pdptIndex = G_PDPT_INDEX(addr);
	uint64_t pdptValue = __atomic_load_n(&pdpt[pdptIndex], __ATOMIC_ACQUIRE);
	if(!pdptValue)
		return 0;
	if(pdptValue & G_PAGE_LARGE_PAGE_FLAG)
		return (pdptValue & addressMask1GiB) + (addr & offsetMask1GiB);

	uint64_t pdAddr = pdptValue & addressMask4KiB;
	if(!pdAddr)
		return 0;

	auto pd = (g_address*) G_MEM_PHYS_TO_VIRT(pdAddr);
	uint64_t pdIndex = G_PD_INDEX(addr);
	uint64_t pdValue = __atomic_load_n(&pd[pdIndex], __ATOMIC_ACQUIRE);
	if(!pdValue)
		return 0;
	if(pdValue & G_PAGE_LARGE_PAGE_FLAG)
		return (pdValue & addressMask2MiB) + (addr & offsetMask2MiB);

	uint64_t ptAddr = pdValue & addressMask4KiB;
	if(!ptAddr)
		return 0;

	auto pt = (g_address*) G_MEM_PHYS_TO_VIRT(ptAddr);
	uint64_t ptIndex = G_PT_INDEX(addr);
	uint64_t ptValue = __atomic_load_n(&pt[ptIndex], __ATOMIC_ACQUIRE);
	if(!ptValue)
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
		auto physical = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
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
	return (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(entry & 0x000FFFFFFFFFF000ULL);
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
	auto pd = _pagingGetOrCreateTable(pdpt, G_PDPT_INDEX(virt), pdFlags);
	auto pt = _pagingGetOrCreateTable(pd, G_PD_INDEX(virt), ptFlags);

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

	pagingInvalidatePage(virt);
	return true;
}

void pagingUnmapPage(g_virtual_address virt)
{
	auto pml4 = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(pagingGetCurrentSpace());
	uint64_t pml4Index = G_PML4_INDEX(virt);
	if(!__atomic_load_n(&pml4[pml4Index], __ATOMIC_ACQUIRE))
		return;

	auto pdpt = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(__atomic_load_n(&pml4[pml4Index], __ATOMIC_ACQUIRE) & ~G_PAGE_ALIGN_MASK);
	uint64_t pdptIndex = G_PDPT_INDEX(virt);
	if(!__atomic_load_n(&pdpt[pdptIndex], __ATOMIC_ACQUIRE))
		return;

	auto pd = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(__atomic_load_n(&pdpt[pdptIndex], __ATOMIC_ACQUIRE) & ~G_PAGE_ALIGN_MASK);
	uint64_t pdIndex = G_PD_INDEX(virt);
	if(!__atomic_load_n(&pd[pdIndex], __ATOMIC_ACQUIRE))
		return;

	auto pt = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(__atomic_load_n(&pd[pdIndex], __ATOMIC_ACQUIRE) & ~G_PAGE_ALIGN_MASK);
	uint64_t ptIndex = G_PT_INDEX(virt);
	if(!__atomic_load_n(&pt[ptIndex], __ATOMIC_ACQUIRE))
		return;

	__atomic_store_n(&pt[ptIndex], 0, __ATOMIC_RELEASE);
	pagingInvalidatePage(virt);
}

g_physical_address pagingGetCurrentSpace()
{
	g_physical_address directory;
	asm volatile("mov %%cr3, %0" : "=r"(directory));
	return directory;
}
