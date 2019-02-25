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

void test()
{
	for(;;)
	{
		logInfo("Hello!");
	}
}

void taskingInitializeLocal(g_tasking_local* local)
{
	local->lock = 0;
	local->list = 0;
	local->current = 0;

	// Create local kernel stack
	g_physical_address kernPhys = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
	g_virtual_address kernVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
	pagingMapPage(kernVirt, kernPhys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	local->kernelStack = kernVirt + G_PAGE_SIZE;
	gdtSetTssEsp0(local->kernelStack);

	g_task* thread = taskingCreateThread((g_virtual_address) test, G_SECURITY_LEVEL_KERNEL);
	taskingAssign(local, thread);
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

g_task* taskingCreateThread(g_virtual_address entry, g_security_level level)
{
	g_task* task = (g_task*) heapAllocate(sizeof(g_task));

	// TODO

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

void taskingStore(g_processor_state* stateIn) {
	g_tasking_local* local = taskingGetLocal();

	if(local->current) 
		local->current->state = *stateIn;
	else
		taskingSchedule();
}


void taskingRestore(g_processor_state* stateOut) {
	g_tasking_local* local = taskingGetLocal();
	if(!local->current)
		kernelPanic("%! tried to restore without a current task", "tasking");

	*stateOut = local->current->state;

	pagingSwitchSpace(local->current->pageDirectory);
}


void taskingSchedule()
{
	// TODO real scheduling
}

g_tasking_local* taskingGetLocal()
{
	return &taskingLocal[processorGetCurrentId()];
}
