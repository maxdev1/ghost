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

#include "kernel/utils/debug.hpp"
#include "kernel/system/timing/pit.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/memory/constants.hpp"

void debugHardSleep(uint64_t millis)
{
	for(int i = 0; i < millis; i++)
	{
		pitPrepareSleep(1000); // 1ms
		pitPerformSleep();
	}
}

void hexDumpRow(void* location, int bytes, bool center = false)
{
	auto centerStr = center ? "<-" : "";

	if(bytes == 1)
	{
		auto loc8 = (uint8_t*) location;
		logInfo("%# %x: %h %h %h %h %h %h %h %h %c%c%c%c%c%c%c%c %s", location,
		        loc8[0], loc8[1], loc8[2], loc8[3], loc8[4], loc8[5], loc8[6], loc8[7],
		        loc8[0], loc8[1], loc8[2], loc8[3], loc8[4], loc8[5], loc8[6], loc8[7],
		        centerStr);
	}
	else if(bytes == 8)
	{
		auto loc64 = (uint64_t*) location;
		logInfo("%# %x: %x %s", location, *loc64, centerStr);
	}
}

void hexDump(void* location, int minus, int plus, int bytes)
{
	for(int i = minus; i > 0; i--)
		hexDumpRow((uint8_t*) location - i * bytes, bytes);

	hexDumpRow(location, bytes, true);

	for(int i = 1; i < plus + 1; ++i)
		hexDumpRow((uint8_t*) location + i * bytes, bytes);
}

void hexDump8(void* location, int minus, int plus)
{
	hexDump(location, minus, plus, 1);
}

void hexDump64(void* location, int minus, int plus)
{
	hexDump(location, minus, plus, 8);
}

void debugDumpPageSpace()
{
	logInfo("%! debug logging initial space:", "paging");
	auto pml4 = (g_address*) G_MEM_PHYS_TO_VIRT(pagingGetCurrentSpace());
	for(int first = 0; first < 512; first++)
	{
		if(!pml4[first])
			continue;

		logInfo("%# %i: %h -> %h %s", first, G_PML4_VIRT_ADDRESS(first, 0, 0, 0), pml4[first],
		        pml4[first] & G_PAGE_USER_FLAG ? "user" :"kernel");
		auto pdpt = (g_address*) G_MEM_PHYS_TO_VIRT(G_PAGE_ALIGN_DOWN(pml4[first]));
		for(int second = 0; second < 512; second++)
		{
			if(!pdpt[second])
				continue;

			logInfo("%# %i > %i: %h -> %h %s", first, second, G_PML4_VIRT_ADDRESS(first, second, 0, 0), pdpt[second],
			        pdpt[second]& G_PAGE_USER_FLAG ? "user":"kernel");
			auto pd = (g_address*) G_MEM_PHYS_TO_VIRT(G_PAGE_ALIGN_DOWN(pdpt[second]));
			for(int third = 0; third < 512; third++)
			{
				if(!pd[third])
					continue;

				logInfo("%# %i > %i > %i: %h -> %h %s", first, second, third,
				        G_PML4_VIRT_ADDRESS(first, second, third, 0),
				        pd[third], pd[third]& G_PAGE_USER_FLAG ? "user":"kernel");
				if(pd[third] & G_PAGE_LARGE_PAGE_FLAG)
				{
					logInfo("%#    up to %h", G_PML4_VIRT_ADDRESS(first, second, third, 0) + 0x200000);
				}
				else
				{
					auto pt = (g_address*) G_MEM_PHYS_TO_VIRT(G_PAGE_ALIGN_DOWN(pd[third]));
					for(int fourth = 0; fourth < 512; fourth++)
					{
						if(!pt[fourth])
							continue;

						logInfo("%# %i > %i > %i > %i: %h -> %h %s", first, second, third, fourth,
						        G_PML4_VIRT_ADDRESS(first, second, third, fourth),
						        pt[fourth], pt[fourth]& G_PAGE_USER_FLAG ? "user":"kernel");
					}
				}
			}
		}
	}
}

void debugPagingPrintValues(g_virtual_address addr)
{
	logInfo("%! debug output for %x", "paging", addr);

	auto pml4 = (g_address*) G_MEM_PHYS_TO_VIRT(pagingGetCurrentSpace());
	uint64_t pml4Index = G_PML4_INDEX(addr);
	logInfo("%# in PML4: %x (NX? %i)", pml4[pml4Index], ((1ULL << 63) & pml4[pml4Index]));
	uint64_t pdptAddr = pml4[pml4Index] & ~G_PAGE_ALIGN_MASK;
	if(!pdptAddr)
		return;

	auto pdpt = (g_address*) G_MEM_PHYS_TO_VIRT(pdptAddr);
	uint64_t pdptIndex = G_PDPT_INDEX(addr);
	logInfo("%# in PDPT: %x (NX? %i)", pdpt[pdptIndex], ((1ULL << 63) & pdpt[pdptIndex]));
	uint64_t pdAddr = pdpt[pdptIndex] & ~G_PAGE_ALIGN_MASK;
	if(!pdAddr)
		return;

	auto pd = (g_address*) G_MEM_PHYS_TO_VIRT(pdAddr);
	uint64_t pdIndex = G_PD_INDEX(addr);
	logInfo("%# in PD:   %x (NX? %i)", pd[pdIndex], ((1ULL << 63) & pd[pdIndex]));
	uint64_t ptAddr = pd[pdIndex] & ~G_PAGE_ALIGN_MASK;
	if(!ptAddr)
		return;

	auto pt = (g_address*) G_MEM_PHYS_TO_VIRT(ptAddr);
	uint64_t ptIndex = G_PT_INDEX(addr);
	logInfo("%# in PT:   %x (NX? %i)", pt[ptIndex], ((1ULL << 63) & pt[ptIndex]));
}
