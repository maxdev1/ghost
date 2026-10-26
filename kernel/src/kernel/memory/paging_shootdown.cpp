#include "kernel/memory/paging_shootdown.hpp"
#include "kernel/memory/constants.hpp"
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
	__atomic_store_n(&enabled, cpuCount > 1, __ATOMIC_RELEASE);
}

bool pagingHandleShootdownNmi()
{
	if(!__atomic_load_n(&enabled, __ATOMIC_ACQUIRE))
		return false;

	// GS can be changing when an NMI arrives during a context switch.
	auto cpu = processorGetCurrentIdFromApic();
	auto& mailbox = mailboxes[cpu];
	const auto request = __atomic_load_n(&mailbox.requested, __ATOMIC_ACQUIRE);

	if(request == __atomic_load_n(&mailbox.completed, __ATOMIC_RELAXED))
		return false;

	if(mailbox.address > G_MEM_LOWER_HALF_END || pagingGetCurrentSpace() == mailbox.space)
		pagingInvalidatePage(mailbox.address);

	// No mailbox data is accessed after acknowledgement: the next sender can
	// reuse it while this CPU is still returning from the previous NMI
	__atomic_store_n(&mailbox.completed, request, __ATOMIC_RELEASE);
	return true;
}

void pagingSynchronizePage(g_virtual_address address)
{
	pagingInvalidatePage(address);
	if(!__atomic_load_n(&enabled, __ATOMIC_ACQUIRE))
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
		__atomic_store_n(&mailbox.requested, request, __ATOMIC_RELEASE);

		lapicSendNmi(mailbox.apicId);
	}

	for(uint32_t cpu = 0; cpu < cpuCount; ++cpu)
		if(cpu != current)
			while(__atomic_load_n(&mailboxes[cpu].completed, __ATOMIC_ACQUIRE) != request)
				asm volatile("pause");

	G_SPINLOCK_RELEASE(shootdownLock);
}
