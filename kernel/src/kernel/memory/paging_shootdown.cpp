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

#include "kernel/memory/paging_shootdown.hpp"
#include "kernel/memory/constants.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/panic.hpp"
#include "kernel/system/interrupts/apic/lapic.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/spinlock.hpp"

/**
 * xAPIC IDs are eight bits
 */
struct alignas(64) g_tlb_mailbox
{
	g_physical_address space;
	g_virtual_address address;
	uint64_t requested;
	uint64_t completed;
	uint32_t pages;
	uint32_t apicId;
	bool processing;
	bool global;
};

static g_tlb_mailbox mailboxes[256];
static g_spinlock shootdownLock;
static uint32_t cpuCount;
static uint64_t sequence;
static bool enabled;

static void pagingInvalidatePages(g_virtual_address address, uint32_t pages, bool global)
{
	// Ordinary user pages are non-global, reloading CR3 invalidates their translations
	// and paging-structure caches in one operation, including on the sender
	if(!global && pages > 1 && address <= G_MEM_LOWER_HALF_END &&
	   pages <= (G_MEM_LOWER_HALF_END - address) / G_PAGE_SIZE)
		pagingSwitchToSpace(pagingGetCurrentSpace());
	else
		for(uint32_t page = 0; page < pages; ++page)
			pagingInvalidatePage(address + (g_virtual_address) page * G_PAGE_SIZE);
}

void pagingInitializeSmp()
{
	cpuCount = processorGetNumberOfProcessors();
	if(cpuCount > 256)
		panic("%! too many CPUs for xAPIC TLB invalidation", "paging");

	for(auto cpu = processorGetList(); cpu; cpu = cpu->next)
	{
		if(cpu->id >= cpuCount || cpu->apicId > 255)
			panic("%! invalid xAPIC CPU topology", "paging");
		mailboxes[cpu->id].apicId = cpu->apicId;
	}

	memoryAtomicStore(&enabled, cpuCount > 1, G_MEMORY_ORDER_RELEASE);
}

/**
 * An IPI can arrive after polling has acknowledged its request. The busy exchange
 * serializes consumers without blocking interrupt entry. On release, exchange
 * again before checking for a request received while the consumer was busy.
 */
static bool pagingServiceShootdown(bool ipi)
{
	if(!memoryAtomicLoad(&enabled, G_MEMORY_ORDER_ACQUIRE))
		return false;

	g_interrupt_guard interrupts;

	auto cpu = processorGetCurrentId();
	auto& mailbox = mailboxes[cpu];

	auto observed = memoryAtomicLoad(&mailbox.requested, G_MEMORY_ORDER_ACQUIRE);
	if(observed == memoryAtomicLoad(&mailbox.completed, G_MEMORY_ORDER_ACQUIRE))
		return ipi && observed != 0;

	if(memoryAtomicExchange(&mailbox.processing, true, G_MEMORY_ORDER_ACQ_REL))
		return observed != 0;

	bool handled = observed != 0;
	for(;;)
	{
		const auto request = memoryAtomicLoad(&mailbox.requested, G_MEMORY_ORDER_ACQUIRE);
		if(request != memoryAtomicLoad(&mailbox.completed, G_MEMORY_ORDER_RELAXED))
		{
			if(mailbox.global || mailbox.address > G_MEM_LOWER_HALF_END || pagingGetCurrentSpace() == mailbox.space)
				pagingInvalidatePages(mailbox.address, mailbox.pages, mailbox.global);

			memoryAtomicStore(&mailbox.completed, request, G_MEMORY_ORDER_RELEASE);
			handled = true;
		}

		memoryAtomicExchange(&mailbox.processing, false, G_MEMORY_ORDER_ACQ_REL);
		if(memoryAtomicLoad(&mailbox.requested, G_MEMORY_ORDER_ACQUIRE) ==
		   memoryAtomicLoad(&mailbox.completed, G_MEMORY_ORDER_ACQUIRE))
			return handled;

		if(memoryAtomicExchange(&mailbox.processing, true, G_MEMORY_ORDER_ACQ_REL))
			return true;
	}
}

bool pagingHandleShootdownIpi() { return pagingServiceShootdown(true); }

void pagingPollShootdown() { pagingServiceShootdown(false); }

void pagingSynchronizePage(g_virtual_address address) { pagingSynchronizePages(address, 1); }

void pagingSynchronizePages(g_virtual_address address, uint32_t pages, bool global)
{
	if(!pages)
		return;
	pagingInvalidatePages(address, pages, global);
	if(!memoryAtomicLoad(&enabled, G_MEMORY_ORDER_ACQUIRE))
		return;

	g_interrupt_guard interrupts;

	while(!__sync_bool_compare_and_swap(&shootdownLock, 0, 1))
	{
		pagingPollShootdown();
		asm volatile("pause");
	}
	const auto current = processorGetCurrentId();
	const auto space = pagingGetCurrentSpace();
	const auto request = ++sequence;
	for(uint32_t cpu = 0; cpu < cpuCount; ++cpu)
	{
		if(cpu == current)
			continue;

		auto& mailbox = mailboxes[cpu];
		mailbox.space = space;
		mailbox.address = address;
		mailbox.pages = pages;
		mailbox.global = global;
		memoryAtomicStore(&mailbox.requested, request, G_MEMORY_ORDER_RELEASE);
	}

	// Publish every request before waking the other CPUs. With single broadcast
	// we avoid serializing one emulated ICR transaction per CPU on each unmap:
	lapicSendIpiAllOthers(G_INTR_TLB_SHOOTDOWN);

	for(uint32_t cpu = 0; cpu < cpuCount; ++cpu)
		if(cpu != current)
		{
			uint32_t waits = 0;
			uint32_t retries = 0;
			while(memoryAtomicLoad(&mailboxes[cpu].completed, G_MEMORY_ORDER_ACQUIRE) != request)
			{
				asm volatile("pause");
				++waits;
				if(retries < 3 && waits == (retries + 1) * 1000000)
				{
					if(memoryAtomicLoad(&mailboxes[cpu].completed, G_MEMORY_ORDER_ACQUIRE) != request)
					{
						++retries;
						lapicSendIpi(mailboxes[cpu].apicId, G_INTR_TLB_SHOOTDOWN);
					}
				}
			}
		}

	G_SPINLOCK_RELEASE(shootdownLock);
}
