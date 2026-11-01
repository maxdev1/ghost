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

#include "kernel/filesystem/filesystem_taskdelegate.hpp"
#include "kernel/memory/constants.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/tasking/tasking_memory.hpp"

bool filesystemDelegateCopyFromCaller(g_task* task, void* buffer, const void* caller, size_t length)
{
	if(reinterpret_cast<g_address>(caller) <= G_MEM_LOWER_HALF_END)
		return memoryCopyFromUser(task, buffer, reinterpret_cast<g_address>(caller), length);
	memoryCopy(buffer, caller, length);
	return true;
}

bool filesystemDelegateCopyToCaller(g_task* task, void* caller, const void* buffer, size_t length)
{
	if(reinterpret_cast<g_address>(caller) <= G_MEM_LOWER_HALF_END)
		return memoryCopyToUser(task, reinterpret_cast<g_address>(caller), buffer, length);
	memoryCopy(caller, buffer, length);
	return true;
}

bool filesystemDelegateCopyName(g_task* task, const char* name, char* copy)
{
	for(size_t i = 0; i < G_FILENAME_MAX; ++i)
	{
		auto address = reinterpret_cast<g_address>(name);
		if(address > G_MEM_LOWER_HALF_END - i || !memoryCopyFromUser(task, copy + i, address + i, 1))
			return false;
		if(!copy[i])
			return i && !(i == 1 && copy[0] == '.') && !(i == 2 && copy[0] == '.' && copy[1] == '.');
		if(copy[i] == '/')
			return false;
	}
	return false;
}

/**
 * The kernel retains one reference to each isolated page. Internal user aliases
 * cannot be released with g_unmap; no caller buffer is exposed to late writes.
 */
bool filesystemDelegateMapPages(g_task* task, g_fs_task_delegate* delegate)
{
	delegate->controlPage = memoryPhysicalTryAllocate();
	delegate->ioPage = memoryPhysicalTryAllocate();
	if(!delegate->controlPage || !delegate->ioPage)
	{
		filesystemDelegateReleasePages(task, delegate);
		return false;
	}

	memorySetBytes(reinterpret_cast<void*>(G_MEM_PHYS_TO_VIRT(delegate->controlPage)), 0, G_PAGE_SIZE);
	memorySetBytes(reinterpret_cast<void*>(G_MEM_PHYS_TO_VIRT(delegate->ioPage)), 0, G_PAGE_SIZE);

	mutexAcquire(&task->process->lock);
	delegate->mapping =
			addressRangePoolAllocate(task->process->virtualRangePool, 2, G_PROC_VIRTUAL_RANGE_FLAG_INTERNAL);

	uint32_t installed = 0;
	if(delegate->mapping)
	{
		g_physical_address pages[] = {delegate->controlPage, delegate->ioPage};
		for(; installed < 2; ++installed)
		{
			pageReferenceTrackerIncrement(pages[installed]);
			if(!pagingMapPage(delegate->mapping + installed * G_PAGE_SIZE, pages[installed], G_PAGE_TABLE_USER_DEFAULT,
							  G_PAGE_USER_DEFAULT))
			{
				memoryPhysicalFree(pages[installed]);
				break;
			}
		}
		if(installed != 2)
		{
			for(uint32_t i = 0; i < installed; ++i)
			{
				pagingUnmapPage(delegate->mapping + i * G_PAGE_SIZE);
				memoryPhysicalFree(pages[i]);
			}
			addressRangePoolFree(task->process->virtualRangePool, delegate->mapping);
			delegate->mapping = 0;
		}
	}
	mutexRelease(&task->process->lock);

	if(!delegate->mapping)
		filesystemDelegateReleasePages(task, delegate);
	return installed == 2;
}

void filesystemDelegateReleasePages(g_task* task, g_fs_task_delegate* delegate)
{
	if(delegate->mapping)
	{
		mutexAcquire(&task->process->lock);
		auto back = taskingMemoryTemporarySwitchTo(task->process->pageSpace);

		g_physical_address pages[] = {delegate->controlPage, delegate->ioPage};
		for(uint32_t i = 0; i < 2; ++i)
		{
			pagingUnmapPage(delegate->mapping + i * G_PAGE_SIZE);
			memoryPhysicalFree(pages[i]);
		}

		addressRangePoolFree(task->process->virtualRangePool, delegate->mapping);
		taskingMemoryTemporarySwitchBack(back);
		mutexRelease(&task->process->lock);
		delegate->mapping = 0;
	}
	if(delegate->controlPage)
		memoryPhysicalFree(delegate->controlPage);
	if(delegate->ioPage)
		memoryPhysicalFree(delegate->ioPage);
	delegate->controlPage = delegate->ioPage = 0;
}
