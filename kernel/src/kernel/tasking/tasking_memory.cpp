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

#include "kernel/tasking/tasking_memory.hpp"
#include "kernel/system/system.hpp"
#include "kernel/memory/lower_heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/memory/constants.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/panic.hpp"

bool taskingMemoryExtendHeap(g_task* task, int32_t amount, g_address* outAddress)
{
	g_process* process = task->process;
	mutexAcquire(&process->lock);
	*outAddress = G_ADDRESS_MAX;
	const g_virtual_address start = process->heap.brk ? process->heap.start : process->image.end;
	const g_virtual_address oldBrk = process->heap.brk ? process->heap.brk : start;
	const int64_t delta = amount;
	// The heap ends where the separately allocated user ranges begin. Validate
	// before unsigned arithmetic, including INT32_MIN and shrinking below start.
	if(!start || (start & G_PAGE_ALIGN_MASK) || oldBrk < start || oldBrk > G_USER_MAXIMUM_HEAP_BREAK ||
	   (delta < 0 && (uint64_t)(-delta) > oldBrk - start) ||
	   (delta >= 0 && (uint64_t)delta > G_USER_MAXIMUM_HEAP_BREAK - oldBrk))
	{
		mutexRelease(&process->lock);
		return false;
	}
	const g_virtual_address newBrk = oldBrk + delta;
	const uint32_t wantedPages = G_PAGE_ALIGN_UP(newBrk - start) / G_PAGE_SIZE;
	const uint32_t oldPages = process->heap.pages;
	auto back = taskingMemoryTemporarySwitchTo(process->pageSpace);
	uint32_t installed = oldPages;
	for(; installed < wantedPages; ++installed)
	{
		auto physical = memoryPhysicalTryAllocate();
		if(!physical)
			break;
		memorySetBytes((void*) G_MEM_PHYS_TO_VIRT(physical), 0, G_PAGE_SIZE);
		if(!pagingMapPage(start + installed * G_PAGE_SIZE, physical, G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT))
		{
			memoryPhysicalFree(physical);
			break;
		}
	}
	const bool success = installed >= wantedPages;
	// On failure undo only the pages installed by this call. The previous break
	// and contents stay valid; a conflicting mapping belongs to someone else.
	const uint32_t keepPages = success ? wantedPages : oldPages;
	while(installed > keepPages)
	{
		auto address = start + --installed * G_PAGE_SIZE;
		auto physical = pagingVirtualToPhysical(address);
		pagingUnmapPage(address);
		memoryPhysicalFree(physical);
	}
	if(success)
	{
		process->heap.start = start;
		process->heap.brk = newBrk;
		process->heap.pages = wantedPages;
		*outAddress = oldBrk;
	}
	taskingMemoryTemporarySwitchBack(back);
	mutexRelease(&process->lock);
	return success;
}

bool taskingMemoryInitialize(g_task* task)
{
	if(!taskingMemoryInitializeStacks(task))
		return false;
	taskingMemoryInitializeUtility(task);
	return taskingMemoryInitializeTls(task);
}

void taskingMemoryInitializeUtility(g_task* task)
{
	if(processorHasFeature(g_cpuid_standard_edx_feature::SSE))
	{
		// TODO Allocator not capable of aligned allocation
		task->fpu.stateMem = (uint8_t*) heapAllocate(G_SSE_STATE_SIZE + G_SSE_STATE_ALIGNMENT);
		task->fpu.state = (uint8_t*) G_ALIGN_UP((g_address) task->fpu.stateMem, G_SSE_STATE_ALIGNMENT);

		if(task->process && task->process->main && task->process->main != task)
		{
			memoryCopy(task->fpu.state, task->process->main->fpu.state, G_SSE_STATE_SIZE);
		}
		else
		{
			memoryCopy(task->fpu.state, processorGetInitialFpuState(), G_SSE_STATE_SIZE);
		}
		// The first dispatch must restore this image too, before the first save.
		task->fpu.stored = true;
	}
	else
	{
		task->fpu.stateMem = nullptr;
		task->fpu.state = nullptr;
		task->fpu.stored = false;
	}
}

bool taskingMemoryInitializeStacks(g_task* task)
{
	// Interrupt stack for ring 3 & VM86 tasks
	if(task->securityLevel != G_SECURITY_LEVEL_KERNEL)
	{
		task->interruptStack = taskingMemoryCreateStack(memoryVirtualRangePool, G_PAGE_TABLE_KERNEL_DEFAULT,
		                                                G_PAGE_KERNEL_DEFAULT, G_TASKING_MEMORY_INTERRUPT_STACK_PAGES);
		if(!task->interruptStack.start)
			return false;
	}
	else
	{
		task->interruptStack.start = 0;
		task->interruptStack.end = 0;
	}

	// Create task stack
	if(task->type == G_TASK_TYPE_VM86)
	{
		size_t stackSize = G_PAGE_SIZE;
		task->stack.start = (g_address) lowerHeapAllocate(stackSize);
		task->stack.end = task->stack.start + stackSize;
	}
	else if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		task->stack = taskingMemoryCreateStack(memoryVirtualRangePool, G_PAGE_TABLE_KERNEL_DEFAULT,
		                                       G_PAGE_KERNEL_DEFAULT, G_TASKING_MEMORY_KERNEL_STACK_PAGES);
	}
	else
	{
		task->stack = taskingMemoryCreateStack(task->process->virtualRangePool, G_PAGE_TABLE_USER_DEFAULT,
		                                       G_PAGE_USER_DEFAULT, G_TASKING_MEMORY_USER_STACK_PAGES);
	}
	return task->stack.start != 0;
}

g_stack taskingMemoryCreateStack(g_address_range_pool* addressRangePool, uint32_t tableFlags, uint32_t pageFlags,
                                 int pages)
{
	if(pages < 2)
		return {};
	g_virtual_address stackVirt = addressRangePoolAllocate(addressRangePool, pages, G_PROC_VIRTUAL_RANGE_FLAG_INTERNAL);
	if(!stackVirt)
		return {};

	// Only allocate and map the last page of the stack; when the process faults, lazy-allocate more physical space.
	// The first page of the allocated virtual range is used as a "guard page" and makes the process fault when accessed.
	g_physical_address pagePhys = memoryPhysicalTryAllocate();
	if(!pagePhys)
	{
		addressRangePoolFree(addressRangePool, stackVirt);
		return {};
	}
	memorySetBytes((void*) G_MEM_PHYS_TO_VIRT(pagePhys), 0, G_PAGE_SIZE);
	g_address stackEnd = stackVirt + pages * G_PAGE_SIZE;
	if(!pagingMapPage(stackEnd - G_PAGE_SIZE, pagePhys, tableFlags, pageFlags))
	{
		memoryPhysicalFree(pagePhys);
		addressRangePoolFree(addressRangePool, stackVirt);
		return {};
	}

	g_stack stack;
	stack.start = stackVirt;
	stack.end = stackEnd;
	return stack;
}

void taskingMemoryDestroy(g_task* task)
{
	taskingMemoryDestroyStacks(task);
	taskingMemoryDestroyUtility(task);
	taskingMemoryDestroyTls(task);
}

void taskingMemoryDestroyUtility(g_task* task)
{
	if(task->fpu.stateMem)
	{
		heapFree(task->fpu.stateMem);
		task->fpu.stateMem = nullptr;
		task->fpu.state = nullptr;
		task->fpu.stored = false;
	}
}

static void _taskingMemoryReleasePage(g_virtual_address address)
{
	uint64_t entry = pagingVirtualToPageEntry(address);
	if(!(entry & G_PAGE_PRESENT))
		return;
	pagingUnmapPage(address);
	if(!(entry & G_PAGE_UNMANAGED_FLAG))
		memoryPhysicalFree(entry & G_PAGE_PHYSICAL_MASK);
}

void taskingMemoryDestroyStacks(g_task* task)
{
	// Remove interrupt stack
	if(task->interruptStack.start)
	{
		for(g_virtual_address virt = task->interruptStack.start; virt < task->interruptStack.end; virt += G_PAGE_SIZE)
		{
			_taskingMemoryReleasePage(virt);
		}
		addressRangePoolFree(memoryVirtualRangePool, task->interruptStack.start);
	}

	// Remove task stack
	if(task->type == G_TASK_TYPE_VM86)
	{
		lowerHeapFree((void*) task->stack.start);
	}
	else if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		taskingMemoryDestroyStack(memoryVirtualRangePool, task->stack);
	}
	else
	{
		taskingMemoryDestroyStack(task->process->virtualRangePool, task->stack);
	}
}

void taskingMemoryDestroyStack(g_address_range_pool* addressRangePool, g_stack& stack)
{
	if(!stack.start)
		return;
	for(g_virtual_address page = stack.start; page < stack.end; page += G_PAGE_SIZE)
	{
		_taskingMemoryReleasePage(page);
	}

	addressRangePoolFree(addressRangePool, stack.start);
	stack = {};
}

g_physical_address taskingMemoryCreatePageSpace()
{
	auto currentPml4 = (g_address*) G_MEM_PHYS_TO_VIRT(pagingGetCurrentSpace());

	g_physical_address newPml4Phys = memoryPhysicalTryAllocate();
	if(!newPml4Phys)
		return 0;
	auto newPml4 = (g_address*) G_MEM_PHYS_TO_VIRT(newPml4Phys);

	// Copy all higher-level mappings
	for(size_t i = 0; i < 512; i++)
	{
		if(i >= 256 && currentPml4[i])
			newPml4[i] = currentPml4[i];
		else
			newPml4[i] = 0;
	}

	return newPml4Phys;
}

static void _taskingMemoryDestroyTable(g_physical_address physical, int level)
{
	auto table = (uint64_t*) G_MEM_PHYS_TO_VIRT(physical);
	for(int i = 0; i < 512; ++i)
	{
		const uint64_t entry = table[i];
		if(!(entry & G_PAGE_PRESENT))
			continue;
		table[i] = 0;
		if(level > 1)
		{
			// User mappings created by Ghost always use 4 KiB leaf pages.
			if(entry & G_PAGE_LARGE_PAGE_FLAG)
				panic("%! unexpected large user page during teardown", "tasking");
			_taskingMemoryDestroyTable(entry & G_PAGE_PHYSICAL_MASK, level - 1);
		}
		else if(!(entry & G_PAGE_UNMANAGED_FLAG))
			memoryPhysicalFree(entry & G_PAGE_PHYSICAL_MASK);
	}
	// Page-table pages are allocated without reference tracking.
	bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, physical);
}

void taskingMemoryDestroyPageSpace(g_physical_address directory)
{
	if(directory == pagingGetCurrentSpace())
		panic("%! attempted to destroy the active address space", "tasking");

	// Every task has switched away before final process teardown. User TLB entries
	// are non-global and were flushed by those CR3 switches. The upper half is shared.
	auto root = (uint64_t*) G_MEM_PHYS_TO_VIRT(directory);
	for(int i = 0; i < 256; ++i)
	{
		uint64_t entry = root[i];
		root[i] = 0;
		if(entry & G_PAGE_PRESENT)
			_taskingMemoryDestroyTable(entry & G_PAGE_PHYSICAL_MASK, 3);
	}
	memoryPhysicalFree(directory);
}

bool taskingMemoryInitializeTls(g_task* task)
{
	// Kernel thread-local storage
	if(!task->threadLocal.kernelThreadLocal)
	{
		auto kernelThreadLocal = (g_kernel_threadlocal*) heapAllocate(sizeof(g_kernel_threadlocal));
		kernelThreadLocal->processor = processorGetCurrentId();
		task->threadLocal.kernelThreadLocal = kernelThreadLocal;
	}

	// User thread-local storage from binaries
	if(!task->threadLocal.userThreadLocal)
	{
		g_process* process = task->process;
		if(process->tlsMaster.location)
		{
			// Allocate required virtual range
			uint32_t requiredSize = process->tlsMaster.size;
			uint32_t requiredPages = G_PAGE_ALIGN_UP(requiredSize) / G_PAGE_SIZE;
			if(requiredPages < 1)
				requiredPages = 1;

			g_virtual_address tlsStart = addressRangePoolAllocate(process->virtualRangePool, requiredPages,
			                                                       G_PROC_VIRTUAL_RANGE_FLAG_INTERNAL);
			if(!tlsStart)
				return false;
			g_virtual_address tlsEnd = tlsStart + requiredPages * G_PAGE_SIZE;
			task->threadLocal.start = tlsStart;
			task->threadLocal.end = tlsStart;

			for(g_virtual_address page = tlsStart; page < tlsEnd; page += G_PAGE_SIZE)
			{
				g_physical_address phys = memoryPhysicalTryAllocate();
				if(!phys)
					return false;
				memorySetBytes((void*) G_MEM_PHYS_TO_VIRT(phys), 0, G_PAGE_SIZE);
				if(!pagingMapPage(page, phys, G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT))
				{
					memoryPhysicalFree(phys);
					return false;
				}
				task->threadLocal.end = page + G_PAGE_SIZE;
			}

			// Copy TLS contents
			memorySetBytes((void*) tlsStart, 0, process->tlsMaster.size);
			memoryCopy((void*) tlsStart, (void*) process->tlsMaster.location, process->tlsMaster.size);

			// Store information
			task->threadLocal.userThreadLocal = (g_user_threadlocal*) (tlsStart + process->tlsMaster.userThreadOffset);
			task->threadLocal.userThreadLocal->self = task->threadLocal.userThreadLocal;
			task->threadLocal.start = tlsStart;
			task->threadLocal.end = tlsEnd;

			logDebug("%! created tls copy in process %i, thread %i at %h", "threadmgr", process->id, task->id,
			         task->threadLocal.start);
		}
	}
	return true;
}

void taskingMemoryDestroyTls(g_task* task)
{
	if(task->threadLocal.start)
	{
		for(g_virtual_address page = task->threadLocal.start; page < task->threadLocal.end; page += G_PAGE_SIZE)
		{
			_taskingMemoryReleasePage(page);
		}
		addressRangePoolFree(task->process->virtualRangePool, task->threadLocal.start);
	}

	heapFree(task->threadLocal.kernelThreadLocal);
}

g_physical_address taskingMemoryTemporarySwitchTo(g_physical_address pageDirectory)
{
	g_physical_address back = pagingGetCurrentSpace();
	g_tasking_local* local = taskingGetLocal();
	if(local->scheduling.current)
	{
		if(local->scheduling.current->overridePageDirectory != 0)
			panic("%! %i tried temporary directory switching twice", "tasking", local->scheduling.current->id);

		local->scheduling.current->overridePageDirectory = pageDirectory;
	}
	pagingSwitchToSpace(pageDirectory);
	return back;
}

void taskingMemoryTemporarySwitchBack(g_physical_address back)
{
	g_tasking_local* local = taskingGetLocal();
	if(local->scheduling.current)
		local->scheduling.current->overridePageDirectory = 0;
	pagingSwitchToSpace(back);
}

bool taskingMemoryHandleStackOverflow(g_task* task, g_virtual_address accessed)
{
	g_virtual_address accessedPage = G_PAGE_ALIGN_DOWN(accessed);

	// Is within range of the stack?
	if(accessedPage < task->stack.start || accessedPage >= task->stack.end)
	{
		return false;
	}

	// If guard page was accessed, let the task fault
	if(accessedPage < task->stack.start + G_PAGE_SIZE)
	{
		logInfo("%! task %i page-faulted due to overflowing into stack guard page", "pagefault", task->id);
		return false;
	}

	// Extend the stack
	uint32_t tableFlags, pageFlags;
	if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		tableFlags = G_PAGE_TABLE_KERNEL_DEFAULT;
		pageFlags = G_PAGE_KERNEL_DEFAULT;
	}
	else
	{
		tableFlags = G_PAGE_TABLE_USER_DEFAULT;
		pageFlags = G_PAGE_USER_DEFAULT;
	}

	if(pagingVirtualToPageEntry(accessedPage) & G_PAGE_PRESENT)
	{
		pagingInvalidatePage(accessedPage);
		return true;
	}
	auto physical = memoryPhysicalTryAllocate();
	if(!physical)
		return false;
	memorySetBytes((void*) G_MEM_PHYS_TO_VIRT(physical), 0, G_PAGE_SIZE);
	if(pagingMapPage(accessedPage, physical, tableFlags, pageFlags))
		return true;
	// Another fault may have installed the same page. Do not leak our copy.
	memoryPhysicalFree(physical);
	pagingInvalidatePage(accessedPage);
	return (pagingVirtualToPageEntry(accessedPage) & G_PAGE_PRESENT) != 0;
}
