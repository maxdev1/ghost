/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "kernel/memory/memory_user.hpp"
#include "kernel/memory/constants.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/tasking/task.hpp"
#include "kernel/tasking/tasking_memory.hpp"

struct g_user_memory
{
	g_task* task;
	g_user_memory* next;
	g_address address;
	size_t length;
	uint32_t retained;
	bool writable;
	bool ready;
	g_physical_address* pages;
	void* buffer;
};

g_user_memory* memoryUserAcquire(g_task* task, g_address address, size_t length, bool writable)
{
	bool trusted = task->securityLevel == G_SECURITY_LEVEL_KERNEL && address > G_MEM_LOWER_HALF_END;
	if(trusted ? length > UINTPTR_MAX - address
			   : address > G_MEM_LOWER_HALF_END || length > G_MEM_LOWER_HALF_END - address + 1)
		return nullptr;

	auto pages = length && !trusted ? ((address & G_PAGE_ALIGN_MASK) + length - 1) / G_PAGE_SIZE + 1 : 0;
	if(pages > (UINT32_MAX - sizeof(g_user_memory)) / sizeof(g_physical_address))
		return nullptr;

	auto memory = (g_user_memory*) heapTryAllocate(sizeof(g_user_memory) + pages * sizeof(g_physical_address));
	if(!memory)
		return nullptr;
	memory->buffer = nullptr;
	memory->pages = (g_physical_address*) (memory + 1);
	memory->task = task;
	memory->address = address;
	memory->length = length;
	memory->retained = 0;
	memory->writable = writable;
	memory->ready = false;
	memory->next = task->userMemory;
	task->userMemory = memory;

	while(memory->retained < pages)
	{
		auto page = G_PAGE_ALIGN_DOWN(address) + memory->retained * G_PAGE_SIZE;
		mutexAcquire(&task->process->lock);
		auto entry = pagingVirtualToPageEntry(page);
		if(!(entry & G_PAGE_PRESENT))
		{
			mutexRelease(&task->process->lock);
			if(taskingMemoryHandleStackOverflow(task, page) || memoryOnDemandHandlePageFault(task, page))
				continue;
			memoryUserRelease(memory);
			return nullptr;
		}

		auto required = G_PAGE_PRESENT | G_PAGE_USER_FLAG | (writable ? G_PAGE_WRITABLE_FLAG : 0);
		if((entry & required) != required || (entry & (G_PAGE_UNMANAGED_FLAG | G_PAGE_LARGE_PAGE_FLAG)))
		{
			mutexRelease(&task->process->lock);
			memoryUserRelease(memory);
			return nullptr;
		}

		auto physical = entry & G_PAGE_PHYSICAL_MASK;
		pageReferenceTrackerIncrement(physical);
		memory->pages[memory->retained++] = physical;
		mutexRelease(&task->process->lock);
	}

	memory->ready = true;
	return memory;
}

void memoryUserRelease(g_user_memory* memory)
{
	if(!memory)
		return;

	auto link = &memory->task->userMemory;
	while(*link && *link != memory)
		link = &(*link)->next;
	*link = memory->next;

	for(uint32_t i = 0; i < memory->retained; ++i)
		memoryPhysicalFree(memory->pages[i]);
	if(memory->buffer)
		heapFree(memory->buffer);
	heapFree(memory);
}

void memoryUserReleaseAll(g_task* task)
{
	while(task->userMemory)
		memoryUserRelease(task->userMemory);
}

static bool memoryUserCopy(g_user_memory* memory, size_t offset, void* buffer, size_t length, bool toUser);

bool memoryUserCopyPinned(g_task* task, g_address address, void* buffer, size_t length, bool toUser)
{
	for(auto memory = task->userMemory; memory; memory = memory->next)
		if(memory->ready && (!toUser || memory->writable) && address >= memory->address &&
		   address - memory->address <= memory->length && length <= memory->length - (address - memory->address))
			return memoryUserCopy(memory, address - memory->address, buffer, length, toUser);
	return false;
}

static bool memoryUserCopy(g_user_memory* memory, size_t offset, void* buffer, size_t length, bool toUser)
{
	if(!memory || !memory->ready || (toUser && !memory->writable) || offset > memory->length ||
	   length > memory->length - offset)
		return false;

	auto bytes = static_cast<uint8_t*>(buffer);
	if(memory->address > G_MEM_LOWER_HALF_END)
	{
		auto kernel = (uint8_t*) (memory->address + offset);
		for(size_t i = 0; i < length; ++i)
		{
			if(toUser)
				kernel[i] = bytes[i];
			else
				bytes[i] = kernel[i];
		}
		return true;
	}
	offset += memory->address & G_PAGE_ALIGN_MASK;
	while(length)
	{
		auto count = G_PAGE_SIZE - (offset & G_PAGE_ALIGN_MASK);
		if(count > length)
			count = length;
		auto physical =
				(uint8_t*) G_MEM_PHYS_TO_VIRT(memory->pages[offset / G_PAGE_SIZE]) + (offset & G_PAGE_ALIGN_MASK);
		for(size_t i = 0; i < count; ++i)
		{
			if(toUser)
				physical[i] = bytes[i];
			else
				bytes[i] = physical[i];
		}
		bytes += count;
		offset += count;
		length -= count;
	}
	return true;
}

bool memoryCopyUserString(g_task* task, const char* source, char* target, size_t capacity)
{
	auto address = (g_address) source;
	for(size_t i = 0; i < capacity; ++i)
	{
		if(i > UINTPTR_MAX - address || !memoryCopyFromUser(task, target + i, address + i, 1))
			return false;
		if(!target[i])
			return true;
	}
	return false;
}

bool memoryUserRead(g_user_memory* memory, void* target, size_t length)
{
	return memoryUserCopy(memory, 0, target, length, false);
}

bool memoryUserWrite(g_user_memory* memory, const void* source, size_t length)
{
	return memoryUserCopy(memory, 0, (void*) source, length, true);
}

void* memoryUserBuffer(g_user_memory* memory)
{
	if(!memory || memory->length > INT32_MAX)
		return nullptr;
	if(!memory->buffer)
		memory->buffer = heapTryAllocate(memory->length ? memory->length : 1);
	return memory->buffer;
}

static bool memoryCopyUser(g_task* task, void* target, const void* source, size_t length, bool toUser)
{
	auto address = (g_address) (toUser ? target : source);
	if(task->securityLevel == G_SECURITY_LEVEL_KERNEL && address > G_MEM_LOWER_HALF_END)
	{
		if(length > UINTPTR_MAX - address)
			return false;
		for(size_t i = 0; i < length; ++i)
			((uint8_t*) target)[i] = ((const uint8_t*) source)[i];
		return true;
	}
	if(address > G_MEM_LOWER_HALF_END || length > G_MEM_LOWER_HALF_END - address + 1)
		return false;

	if(memoryUserCopyPinned(task, address, toUser ? const_cast<void*>(source) : target, length, toUser))
		return true;

	auto kernelTarget = static_cast<uint8_t*>(target);
	auto kernelSource = static_cast<const uint8_t*>(source);
	while(length)
	{
		mutexAcquire(&task->process->lock);
		auto entry = pagingVirtualToPageEntry(address);
		if(!(entry & G_PAGE_PRESENT))
		{
			mutexRelease(&task->process->lock);
			if(!taskingMemoryHandleStackOverflow(task, address) && !memoryOnDemandHandlePageFault(task, address))
				return false;
			continue;
		}

		uint64_t required = G_PAGE_PRESENT | G_PAGE_USER_FLAG | (toUser ? G_PAGE_WRITABLE_FLAG : 0);
		if((entry & required) != required || (entry & (G_PAGE_UNMANAGED_FLAG | G_PAGE_LARGE_PAGE_FLAG)))
		{
			mutexRelease(&task->process->lock);
			return false;
		}

		auto offset = address & G_PAGE_ALIGN_MASK;
		auto count = length < G_PAGE_SIZE - offset ? length : G_PAGE_SIZE - offset;
		auto physical = (void*) (G_MEM_PHYS_TO_VIRT((entry & G_PAGE_PHYSICAL_MASK) + offset));
		if(toUser)
		{
			for(size_t i = 0; i < count; ++i)
				((uint8_t*) physical)[i] = kernelSource[i];
			kernelSource += count;
		}
		else
		{
			for(size_t i = 0; i < count; ++i)
				kernelTarget[i] = ((const uint8_t*) physical)[i];
			kernelTarget += count;
		}

		mutexRelease(&task->process->lock);
		address += count;
		length -= count;
	}

	return true;
}

bool memoryCopyFromUser(g_task* task, void* target, g_address source, size_t length)
{
	return memoryCopyUser(task, target, (const void*) source, length, false);
}

bool memoryCopyToUser(g_task* task, g_address target, const void* source, size_t length)
{
	return memoryCopyUser(task, (void*) target, source, length, true);
}
