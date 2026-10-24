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

#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/system/mutex.hpp"

static g_pp_reference_count_directory directory;
static g_mutex lock;

// Sparse 4 MiB regions, hashed into the directory. Physical addresses above
// 4 GiB must not be used directly as indices into its 1024 buckets.
static g_pp_reference_count_table* _pageReferenceTrackerFind(g_physical_address address, bool create)
{
	const g_physical_address regionSize = 1024 * G_PAGE_SIZE;
	auto base = address & ~(regionSize - 1);
	auto bucket = (base / regionSize) % 1024;
	auto table = directory.tables[bucket];
	while(table && table->base != base)
		table = table->next;

	if(!table && create)
	{
		table = new g_pp_reference_count_table;
		table->base = base;
		table->next = directory.tables[bucket];
		for(uint32_t i = 0; i < 1024; ++i)
			table->referenceCount[i] = 0;
		directory.tables[bucket] = table;
	}
	return table;
}

void pageReferenceTrackerInitialize()
{
	mutexInitializeGlobal(&lock, __func__);
}

void pageReferenceTrackerIncrement(g_physical_address address)
{
	mutexAcquire(&lock);

	auto table = _pageReferenceTrackerFind(address, true);
	++table->referenceCount[(address / G_PAGE_SIZE) % 1024];

	mutexRelease(&lock);
}

int16_t pageReferenceTrackerDecrement(g_physical_address address)
{
	mutexAcquire(&lock);

	auto table = _pageReferenceTrackerFind(address, false);
	int16_t refs = 0;
	if(table)
	{
		auto& count = table->referenceCount[(address / G_PAGE_SIZE) % 1024];
		if(count > 0)
			--count;
		refs = count;
	}
	mutexRelease(&lock);

	return refs < 0 ? 0 : refs;
}
