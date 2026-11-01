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
	uint32_t apicId;
};

static g_tlb_mailbox mailboxes[256];
static g_spinlock shootdownLock;
static uint32_t cpuCount;
static uint64_t sequence;
static bool enabled;

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

bool pagingHandleShootdownNmi()
{
	if(!memoryAtomicLoad(&enabled, G_MEMORY_ORDER_ACQUIRE))
		return false;

	// GS can be changing when an NMI arrives during a context switch.
	auto cpu = processorGetCurrentIdFromApic();
	auto& mailbox = mailboxes[cpu];
	const auto request = memoryAtomicLoad(&mailbox.requested, G_MEMORY_ORDER_ACQUIRE);

	if(request == memoryAtomicLoad(&mailbox.completed, G_MEMORY_ORDER_RELAXED))
		return false;

	if(mailbox.address > G_MEM_LOWER_HALF_END || pagingGetCurrentSpace() == mailbox.space)
		pagingInvalidatePage(mailbox.address);

	// No mailbox data is accessed after acknowledgement: the next sender can
	// reuse it while this CPU is still returning from the previous NMI
	memoryAtomicStore(&mailbox.completed, request, G_MEMORY_ORDER_RELEASE);
	return true;
}

void pagingSynchronizePage(g_virtual_address address)
{
	pagingInvalidatePage(address);
	if(!memoryAtomicLoad(&enabled, G_MEMORY_ORDER_ACQUIRE))
		return;

	g_interrupt_guard interrupts;

	G_SPINLOCK_ACQUIRE(shootdownLock);
	const auto current = processorGetCurrentIdFromApic();
	const auto space = pagingGetCurrentSpace();
	const auto request = ++sequence;
	for(uint32_t cpu = 0; cpu < cpuCount; ++cpu)
	{
		if(cpu == current)
			continue;

		auto& mailbox = mailboxes[cpu];
		mailbox.space = space;
		mailbox.address = address;
		memoryAtomicStore(&mailbox.requested, request, G_MEMORY_ORDER_RELEASE);

		lapicSendNmi(mailbox.apicId);
	}

	for(uint32_t cpu = 0; cpu < cpuCount; ++cpu)
		if(cpu != current)
			while(memoryAtomicLoad(&mailboxes[cpu].completed, G_MEMORY_ORDER_ACQUIRE) != request)
				asm volatile("pause");

	G_SPINLOCK_RELEASE(shootdownLock);
}
