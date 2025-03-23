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
#include "shared/logger/logger.hpp"
#include "shared/memory/constants.hpp"

g_physical_address pagingVirtualToPageEntry(g_virtual_address addr)
{
	auto pml4 = (g_address*) G_MEM_PHYS_TO_VIRT(pagingGetCurrentSpace());
	uint64_t pml4Index = G_PML4_INDEX(addr);
	uint64_t pdptAddr = pml4[pml4Index] & ~G_PAGE_ALIGN_MASK;
	if(!pdptAddr)
		return 0;

	auto pdpt = (g_address*) G_MEM_PHYS_TO_VIRT(pdptAddr);
	uint64_t pdptIndex = G_PDPT_INDEX(addr);
	uint64_t pdAddr = pdpt[pdptIndex] & ~G_PAGE_ALIGN_MASK;
	if(!pdAddr)
		return 0;

	auto pd = (g_address*) G_MEM_PHYS_TO_VIRT(pdAddr);
	uint64_t pdIndex = G_PD_INDEX(addr);
	uint64_t pdValue = pd[pdIndex];
	uint64_t ptAddr = pdValue & ~G_PAGE_ALIGN_MASK;
	if(!ptAddr)
		return 0;

	if(pdValue & G_PAGE_LARGE_PAGE_FLAG)
		return pdValue & ~G_PAGE_ALIGN_MASK;

	auto pt = (g_address*) G_MEM_PHYS_TO_VIRT(ptAddr);
	uint64_t ptIndex = G_PT_INDEX(addr);
	return pt[ptIndex];
}

g_physical_address pagingVirtualToPhysical(g_virtual_address addr)
{
	return pagingVirtualToPageEntry(addr) & ~G_PAGE_ALIGN_MASK;
}
