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

#include "kernel/tasking/tasking.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/gdt.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/kernel.hpp"
#include "shared/logger/logger.hpp"

g_tasking_local* taskingLocal;

static g_mutex taskingIdLock = 0;
static g_tid taskingIdNext = 0;

int as[4] =
{ 0, 0, 0, 0 };
int bs[4] =
{ 0, 0, 0, 0 };

void test()
{
	for(;;)
	{
		as[processorGetCurrentId()]++;

		if(processorGetCurrentId() == 0 && as[processorGetCurrentId()] % 100000 == 0)
		{
			logInfo("The processors are counting... a(%i %i %i %i), b(%i %i %i %i)", as[0], as[1], as[2], as[3], bs[0], bs[1], bs[2], bs[3]);
		}
	}
}
void test2()
{
	for(;;)
	{
		bs[processorGetCurrentId()]++;
	}
}

void taskingInitializeLocal(g_tasking_local* local)
{
	local->lock = 0;
	local->list = 0;
	local->current = 0;

	// Create idle thread
	taskingAssign(local, taskingCreateThread((g_virtual_address) test, G_SECURITY_LEVEL_KERNEL));
	taskingAssign(local, taskingCreateThread((g_virtual_address) test2, G_SECURITY_LEVEL_KERNEL));
}

void taskingInitializeBsp()
{
	taskingLocal = (g_tasking_local*) heapAllocate(sizeof(g_tasking_local) * processorGetNumberOfCores());

	g_tasking_local* local = &taskingLocal[processorGetCurrentId()];
	taskingInitializeLocal(local);
}

void taskingInitializeAp()
{
	g_tasking_local* local = &taskingLocal[processorGetCurrentId()];
	taskingInitializeLocal(local);
}

g_physical_address taskingCreatePageDirectory()
{
	g_page_directory directoryCurrent = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;

	g_physical_address directoryPhys = (g_physical_address) bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
	g_virtual_address directoryVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
	g_page_directory directoryTemp = (g_page_directory) directoryVirt;
	pagingMapPage(directoryVirt, directoryPhys);

	// clone kernel space mappings
	for(uint32_t ti = 0; ti < 1024; ti++)
	{
		if(!((directoryCurrent[ti] & G_PAGE_ALIGN_MASK) & G_PAGE_TABLE_USERSPACE))
			directoryTemp[ti] = directoryCurrent[ti];
		else
			directoryTemp[ti] = 0;
	}

	// clone mappings for the lowest 4 MiB TODO
	directoryTemp[0] = directoryCurrent[0];

	// recursive self-map
	directoryTemp[1023] = directoryPhys | DEFAULT_KERNEL_TABLE_FLAGS;

	pagingUnmapPage(directoryVirt);
	return directoryPhys;
}

void taskingApplySecurityLevel(g_processor_state* state, g_security_level securityLevel)
{
	if(securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		state->cs = G_GDT_DESCRIPTOR_KERNEL_CODE | G_SEGMENT_SELECTOR_RING0;
		state->ss = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->ds = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->es = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->fs = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->gs = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
	} else
	{
		state->cs = G_GDT_DESCRIPTOR_USER_CODE | G_SEGMENT_SELECTOR_RING3;
		state->ss = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->ds = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->es = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->fs = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->gs = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
	}

	if(securityLevel <= G_SECURITY_LEVEL_DRIVER)
	{
		state->eflags |= 0x3000; // IOPL 3
	}
}

g_task* taskingCreateThread(g_virtual_address entry, g_security_level level)
{
	g_task* task = (g_task*) heapAllocate(sizeof(g_task));
	task->id = taskingGetNextId();
	task->pageDirectory = taskingCreatePageDirectory();
	task->securityLevel = level;

	// Switch to task directory
	g_physical_address currentDir = (g_physical_address) pagingGetCurrentSpace();
	pagingSwitchSpace(task->pageDirectory);

	if(level == G_SECURITY_LEVEL_KERNEL)
	{
		task->kernelStack0 = 0;
		task->virtualRangePool = 0;
	} else
	{
		// User-space processes get a kernel stack
		g_physical_address kernPhys = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		g_virtual_address kernVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
		pagingMapPage(kernVirt, kernPhys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
		task->kernelStack0 = kernVirt + G_PAGE_SIZE;

		// Create range pool
		task->virtualRangePool = (g_address_range_pool*) heapAllocate(sizeof(g_address_range_pool));
		addressRangePoolInitialize(task->virtualRangePool);
		addressRangePoolAddRange(task->virtualRangePool, G_CONST_USER_VIRTUAL_RANGES_START, G_CONST_USER_VIRTUAL_RANGES_END);
	}

	// Create main stack
	g_physical_address stackPhys = (g_physical_address) bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
	g_virtual_address stackVirt;

	if(level == G_SECURITY_LEVEL_KERNEL)
	{
		stackVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
		pagingMapPage(stackVirt, stackPhys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	} else
	{
		stackVirt = addressRangePoolAllocate(task->virtualRangePool, 1);
		pagingMapPage(stackVirt, stackPhys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	}
	task->mainStack0 = stackVirt + G_PAGE_SIZE;

	// Initialize task state
	memorySetBytes((void*) &task->state, 0, sizeof(g_processor_state));
	task->state.esp = task->mainStack0 - sizeof(g_processor_state);
	task->state.eip = entry;
	task->state.eflags = 0x200;
	taskingApplySecurityLevel(&task->state, level);

	// Switch back
	pagingSwitchSpace(currentDir);
	return task;
}

void taskingAssign(g_tasking_local* local, g_task* task)
{
	mutexAcquire(&local->lock);

	g_task_entry* newEntry = (g_task_entry*) heapAllocate(sizeof(g_task_entry));
	newEntry->task = task;
	newEntry->next = local->list;
	local->list = newEntry;

	mutexRelease(&local->lock);
}

void taskingStore(g_virtual_address esp)
{
	g_tasking_local* local = taskingGetLocal();

	if(local->current)
	{
		// For kernel tasks - ESP and SS were not pushed by the CPU
		if(local->current->securityLevel == G_SECURITY_LEVEL_KERNEL)
		{
			memoryCopy(&local->current->state, (void*) esp, sizeof(g_processor_state) - sizeof(uint32_t) * 2);
			local->current->state.esp = esp;
		} else
		{
			memoryCopy(&local->current->state, (void*) esp, sizeof(g_processor_state));
		}
	} else
	{
		taskingSchedule();
	}
}

g_virtual_address taskingRestore(g_virtual_address esp)
{
	g_tasking_local* local = taskingGetLocal();
	if(!local->current)
		kernelPanic("%! tried to restore without a current task", "tasking");

	pagingSwitchSpace(local->current->pageDirectory);

	gdtSetTssEsp0(local->current->kernelStack0);

	// For kernel tasks we will return to kernel stack
	if(local->current->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		esp = local->current->state.esp;
		memoryCopy((void*) esp, &local->current->state, sizeof(g_processor_state) - sizeof(uint32_t) * 2);
		// TODO ss
	} else
	{
		memoryCopy((void*) esp, &local->current->state, sizeof(g_processor_state));
	}

	return esp;
}

void taskingSchedule()
{
	// TODO
	g_tasking_local* local = taskingGetLocal();
	mutexAcquire(&local->lock);

	if(local->current)
	{
		// Just find current entry and take next
		g_task_entry* entry = local->list;
		while(entry)
		{
			if(entry->task == local->current)
			{
				break;
			}
			entry = entry->next;
		}
		if(entry)
		{
			entry = entry->next;

			if(entry)
			{
				local->current = entry->task;
			} else
			{
				local->current = local->list->task;
			}
		} else
		{
			local->current = local->list->task;
		}

	} else
	{
		local->current = local->list->task;
	}

	mutexRelease(&local->lock);
}

g_tasking_local* taskingGetLocal()
{
	return &taskingLocal[processorGetCurrentId()];
}

g_tid taskingGetNextId()
{
	mutexAcquire(&taskingIdLock);
	g_tid next = taskingIdNext++;
	mutexRelease(&taskingIdLock);
	return next;
}

