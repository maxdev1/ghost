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

#include "shared/memory/paging.hpp"
#include "shared/logger/logger.hpp"
#include "shared/memory/constants.hpp"
#include "shared/memory/memory.hpp"
#include "shared/panic.hpp"

// Switches to a new paging structure (by loading CR3)
void pagingSwitchToSpace(g_physical_address dir)
{
	asm volatile("mov %0, %%cr3" : : "b"(dir));
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


	// Get PDPT from PML4
	uint64_t pml4Index = G_PML4_INDEX(virt);
	volatile uint64_t* pdpt;
	if(!pml4[pml4Index])
	{
		g_physical_address newPdpt = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		pml4[pml4Index] = newPdpt | pdptFlags;

		pdpt = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(newPdpt);
		for(int i = 0; i < 512; i++)
			pdpt[i] = 0;
	}
	else
	{
		pdpt = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(pml4[pml4Index] & ~G_PAGE_ALIGN_MASK);
	}

	// Get PD from PDPT
	uint64_t pdptIndex = G_PDPT_INDEX(virt);
	volatile uint64_t* pd;
	if(!pdpt[pdptIndex])
	{
		g_physical_address newPd = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		pdpt[pdptIndex] = newPd | pdFlags;

		pd = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(newPd);
		for(int i = 0; i < 512; i++)
			pd[i] = 0;
	}
	else
	{
		pd = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(pdpt[pdptIndex] & ~G_PAGE_ALIGN_MASK);
	}

	// Get PT from PD
	uint64_t pdIndex = G_PD_INDEX(virt);
	volatile uint64_t* pt;
	if(!pd[pdIndex])
	{
		g_physical_address newPt = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		pd[pdIndex] = newPt | ptFlags;

		pt = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(newPt);
		for(int i = 0; i < 512; i++)
			pt[i] = 0;
	}
	else
	{
		pt = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(pd[pdIndex] & ~G_PAGE_ALIGN_MASK);
	}

	// Write page into page table
	uint64_t ptIndex = G_PT_INDEX(virt);
	if(!pt[ptIndex] || allowOverride)
	{
		pt[ptIndex] = phys | pageFlags;
		pagingInvalidatePage(virt);
		return true;
	}

	return false;
}

void pagingUnmapPage(g_virtual_address virt)
{
	auto pml4 = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(pagingGetCurrentSpace());
	uint64_t pml4Index = G_PML4_INDEX(virt);
	if(!pml4[pml4Index])
		return;

	auto pdpt = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(pml4[pml4Index] & ~G_PAGE_ALIGN_MASK);
	uint64_t pdptIndex = G_PDPT_INDEX(virt);
	if(!pdpt[pdptIndex])
		return;

	auto pd = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(pdpt[pdptIndex] & ~G_PAGE_ALIGN_MASK);
	uint64_t pdIndex = G_PD_INDEX(virt);
	if(!pd[pdIndex])
		return;

	auto pt = (volatile uint64_t*) G_MEM_PHYS_TO_VIRT(pd[pdIndex] & ~G_PAGE_ALIGN_MASK);
	uint64_t ptIndex = G_PT_INDEX(virt);
	if(!pt[ptIndex])
		return;

	pt[ptIndex] = 0;
	pagingInvalidatePage(virt);
}

g_physical_address pagingGetCurrentSpace()
{
	g_physical_address directory;
	asm volatile("mov %%cr3, %0" : "=r"(directory));
	return directory;
}
