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

#include "kernel/system/interrupts/irq.hpp"
#include "kernel/system/acpi_driver.hpp"
#include "kernel/system/interrupts/apic/ioapic.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/interrupts/irq_route.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/scheduler/scheduler.hpp"

struct g_irq_binding
{
	g_irq_handle handle = G_IRQ_HANDLE_NONE;
	g_irq_route route{};
	g_irq_source source = 0;
	g_pid process = G_PID_NONE;
	g_tid owner = G_TID_NONE;
	bool waiting = false;
	bool pending = false;
	bool masked = true;
	bool retiring = false;
};

static g_irq_binding irqBindings[G_IRQ_VECTOR_COUNT];
static g_mutex irqLock;
static uint64_t irqGeneration;
static uint32_t irqRetiringCount;

void irqInitialize() { mutexInitializeGlobal(&irqLock, __func__); }

/**
 * Keep the original masked RTE in place until the destination BSP has seen
 * the old vector leave both interrupt controllers. A timer tick guarantees a
 * recheck even if the old interrupt was never queued. No caller waits here.
 */
static void irqFinishRetirementLocked(g_irq_binding& slot)
{
	if(slot.retiring && irqRouteIsIdleOnBsp(slot.route))
	{
		irqRouteRestore(slot.route, g_irq_end::RELEASE);
		slot = {};
		__atomic_sub_fetch(&irqRetiringCount, 1, __ATOMIC_RELAXED);
	}
}

void irqReapRetired()
{
	if(!__atomic_load_n(&irqRetiringCount, __ATOMIC_RELAXED) || processorGetCurrentId() != 0)
		return;
	g_mutex_guard guard(&irqLock);
	for(auto& slot: irqBindings)
		if(slot.retiring)
			irqFinishRetirementLocked(slot);
}

g_irq_status irqInstall(g_task* task, g_irq_source source, uint32_t line, g_irq_handle* out)
{
	*out = G_IRQ_HANDLE_NONE;
	if(task->securityLevel > G_SECURITY_LEVEL_DRIVER ||
	   (source == G_IRQ_SOURCE_ACPI && task->process->id != acpiDriverGetService()))
		return G_IRQ_NOT_PERMITTED;

	g_irq_route route;
	auto status = irqRouteResolve(source, line, &route);
	if(status != G_IRQ_SUCCESSFUL)
		return status;

	g_mutex_guard guard(&irqLock);
	for(auto& occupied: irqBindings)
	{
		if(occupied.route.gsi != route.gsi && occupied.route.irqOffset != route.irqOffset)
			continue;
		if(occupied.retiring)
			irqFinishRetirementLocked(occupied);
		if(occupied.handle || occupied.retiring)
			return G_IRQ_BUSY;
	}
	if(irqGeneration == (UINT64_MAX >> 8))
		return G_IRQ_BUSY;

	status = irqRouteInstall(&route);
	if(status != G_IRQ_SUCCESSFUL)
		return status;

	auto& slot = irqBindings[route.irqOffset];
	slot.handle = (++irqGeneration << 8) | route.irqOffset;
	slot.route = route;
	slot.source = source;
	slot.process = task->process->id;
	slot.owner = task->id;
	*out = slot.handle;
	return G_IRQ_SUCCESSFUL;
}

static g_irq_status irqLookupLocked(g_task* task, g_irq_handle handle, g_irq_binding** result)
{
	uint8_t vector = handle & 0xff;
	if(!vector || vector >= G_IRQ_VECTOR_COUNT || !(handle >> 8))
		return G_IRQ_INVALID_ARGUMENT;

	auto& slot = irqBindings[vector];
	if(slot.handle != handle)
		return G_IRQ_RELEASED;
	if(slot.process != task->process->id || task->securityLevel > G_SECURITY_LEVEL_DRIVER)
		return G_IRQ_NOT_PERMITTED;

	*result = &slot;
	return G_IRQ_SUCCESSFUL;
}

static void irqSetMaskedLocked(g_irq_binding& slot, bool masked)
{
	if(slot.masked != masked)
	{
		irqRouteMask(slot.route, masked);
		slot.masked = masked;
	}
}

g_irq_status irqComplete(g_task* task, g_irq_handle handle)
{
	g_mutex_guard guard(&irqLock);
	g_irq_binding* slot;
	auto status = irqLookupLocked(task, handle, &slot);
	if(status != G_IRQ_SUCCESSFUL)
		return status;
	if(!slot->route.level || !slot->pending)
		irqSetMaskedLocked(*slot, false);
	return G_IRQ_SUCCESSFUL;
}

g_irq_status irqMask(g_task* task, g_irq_handle handle)
{
	g_mutex_guard guard(&irqLock);
	g_irq_binding* slot;
	auto status = irqLookupLocked(task, handle, &slot);
	if(status == G_IRQ_SUCCESSFUL)
		irqSetMaskedLocked(*slot, true);
	return status;
}

static void irqDropLocked(g_irq_binding& slot, g_irq_end ending)
{
	if(!ioapicAreAvailable())
	{
		irqRouteRestore(slot.route, ending);
		slot = {};
		return;
	}

	g_irq_route route = slot.route;
	irqRouteMask(route, true);
	slot = {};
	slot.route = route;
	slot.retiring = true;
	__atomic_add_fetch(&irqRetiringCount, 1, __ATOMIC_RELAXED);
	irqFinishRetirementLocked(slot);
}

g_irq_status irqRelease(g_task* task, g_irq_handle handle)
{
	g_task* waiter = nullptr;
	{
		g_mutex_guard guard(&irqLock);
		g_irq_binding* slot;
		auto status = irqLookupLocked(task, handle, &slot);
		if(status != G_IRQ_SUCCESSFUL)
			return status;
		if(slot->waiting)
			waiter = taskingAcquireTask(slot->owner);
		irqDropLocked(*slot, g_irq_end::RELEASE);
	}
	if(waiter)
	{
		taskingWake(waiter);
		taskingReleaseTask(waiter);
	}
	return G_IRQ_SUCCESSFUL;
}

void irqTaskStopped(g_task* task)
{
	g_mutex_guard guard(&irqLock);
	for(auto& slot: irqBindings)
		if(slot.handle && slot.owner == task->id)
			irqDropLocked(slot, g_irq_end::OWNER_EXIT);
}

/**
 * Check pending delivery before checking the deadline. The wait predicate
 * publishes the waiter under the same lock used to latch the IRQ. A spurious
 * wake retries against the original deadline and never extends the timeout.
 */
g_irq_status irqWait(g_task* task, g_irq_handle handle, uint32_t timeoutMs)
{
	uint64_t deadline = timeoutMs ? clockGetMilliseconds() + timeoutMs : 0;
	g_irq_status result;
	while(taskingWaitIf(
			[&]()
			{
				{
					g_mutex_guard guard(&irqLock);
					g_irq_binding* slot;
					result = irqLookupLocked(task, handle, &slot);
					if(result != G_IRQ_SUCCESSFUL)
						return false;
					if(slot->owner != task->id || task->terminationRequested)
					{
						result = G_IRQ_NOT_PERMITTED;
						return false;
					}
					slot->waiting = false;
					if(slot->pending)
					{
						slot->pending = false;
						result = G_IRQ_DELIVERED;
						return false;
					}
					if(timeoutMs && clockGetMilliseconds() >= deadline)
					{
						result = G_IRQ_TIMED_OUT;
						return false;
					}
					taskingPrepareWait(task, __func__);
					slot->waiting = true;
				}
				if(timeoutMs)
					clockWaitForTime(task->id, deadline);
				return true;
			},
			[&]()
			{
				if(timeoutMs)
					clockUnwaitForTime(task);
			}))
	{
	}
	return result;
}

/**
 * Latency-sensitive ISA handlers can yield to an input consumer before they
 * wait again. Hand off only to a runnable local task. Leave unrelated mutex
 * and I/O waits intact and let the scheduler return to the interrupted task.
 */
static void irqHandoff(g_task* interrupted, g_task* handler)
{
	if(!handler->assignment || handler->assignment->processor != processorGetCurrentId() || handler == interrupted)
		return;
	{
		g_mutex_guard guard(&handler->lock);
		if(handler->status != G_TASK_STATUS_RUNNING)
			return;
	}

	taskingSetCurrent(handler);
	if(interrupted)
		schedulerPreferTask(interrupted);
}

void irqHandle(g_task* interrupted, uint8_t vector)
{
	g_task* handler = nullptr;
	bool waiting = false;
	bool handoff = false;
	{
		g_mutex_guard guard(&irqLock);
		if(vector < G_IRQ_VECTOR_COUNT)
		{
			auto& slot = irqBindings[vector];
			if(slot.handle)
			{
				if(slot.route.level)
					irqSetMaskedLocked(slot, true);
				slot.pending = true;
				waiting = slot.waiting;
				slot.waiting = false;
				handoff = slot.source == G_IRQ_SOURCE_ISA;
				if(waiting || handoff)
					handler = taskingAcquireTask(slot.owner);
			}
		}
		interruptsSendEndOfInterrupt(vector);

		if(vector < G_IRQ_VECTOR_COUNT && irqBindings[vector].retiring)
			irqFinishRetirementLocked(irqBindings[vector]);
	}
	if(handler)
	{
		if(waiting)
			taskingWake(handler);
		if(handoff)
			irqHandoff(interrupted, handler);
		taskingReleaseTask(handler);
	}
}
