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

#include "kernel/system/acpi/platform.hpp"
#include <ghost/system/acpi.h>
#include "kernel/memory/memory.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/system/acpi_driver.hpp"
#include "kernel/system/interrupts/apic/ioapic.hpp"
#include "kernel/system/interrupts/pic.hpp"
#include "kernel/system/interrupts/requests.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/tasking/tasking.hpp"

static uint64_t acpiPlatformRsdp;
static g_tid acpiPlatformIrqs[96];
static bool acpiPlatformPending[96];
static g_mutex acpiPlatformLock;

void acpiPlatformInitialize(uint64_t rsdp)
{
	acpiPlatformRsdp = rsdp;
	for(auto& owner: acpiPlatformIrqs)
		owner = G_TID_NONE;
	mutexInitializeGlobal(&acpiPlatformLock, __func__);
}

uint64_t acpiPlatformGetRsdp() { return acpiPlatformRsdp; }

static void acpiPlatformMask(uint32_t irq, bool masked)
{
	if(ioapicAreAvailable())
	{
		if(masked)
			ioapicMaskIrq(irq);
		else
			ioapicUnmaskIrq(irq);
	}
	else
	{
		if(masked)
			picMaskIrq(irq);
		else
			picUnmaskIrq(irq);
	}
}

/**
 * Mask a level-triggered SCI until its driver has cleared the source. The normal
 * IRQ waiter is woken before EOI; the driver explicitly rearms after handling it.
 */
bool acpiPlatformHandleIrq(g_task*, uint8_t irq)
{
	if(irq >= 96)
		return false;
	mutexAcquire(&acpiPlatformLock);
	bool owned = acpiPlatformIrqs[irq] != G_TID_NONE;
	g_task* target = nullptr;
	if(owned)
	{
		acpiPlatformMask(irq, true);
		acpiPlatformPending[irq] = true;
		target = taskingAcquireTask(acpiPlatformIrqs[irq]);
	}
	mutexRelease(&acpiPlatformLock);
	if(target)
	{
		taskingWake(target);
		taskingReleaseTask(target);
	}
	return owned;
}

void acpiPlatformTaskStopped(g_task* task)
{
	g_interrupt_guard interrupts;
	g_mutex_guard guard(&acpiPlatformLock);
	for(uint32_t irq = 1; irq < 96; ++irq)
		if(acpiPlatformIrqs[irq] == task->id)
		{
			acpiPlatformMask(irq, true);
			acpiPlatformIrqs[irq] = G_TID_NONE;
			acpiPlatformPending[irq] = false;
		}
}

static g_acpi_status acpiPlatformValidateIrq(g_task* task, uint32_t irq)
{
	if(!irq || irq >= 96 || (!ioapicAreAvailable() && irq >= 16))
		return G_ACPI_INVALID_ARGUMENT;
	if(task->process->id != acpiDriverGetService())
		return G_ACPI_NOT_PERMITTED;
	return G_ACPI_SUCCESSFUL;
}

static g_task* acpiPlatformAcquireIrqOwner(g_task* task, uint32_t irq)
{
	auto owner = taskingAcquireTask(acpiPlatformIrqs[irq]);
	if(owner && owner->process == task->process)
		return owner;
	taskingReleaseTask(owner);
	return nullptr;
}

static g_acpi_status acpiPlatformRouteIrq(uint32_t irq)
{
	if(!ioapicAreAvailable())
	{
		acpiPlatformMask(irq, true);
		return G_ACPI_SUCCESSFUL;
	}
	auto io = ioapicGetResponsibleFor(irq);
	if(!io)
		return G_ACPI_NOT_SUPPORTED;
	auto bsp = processorGetBsp();
	uint32_t destination = bsp ? bsp->apicId : 0;
	uint64_t entry = IOAPIC_REDTBL_INTVEC_MAKE(0x20 + irq) | IOAPIC_REDTBL_INTPOL_LOW_ACTIVE |
					 IOAPIC_REDTBL_TRIGGERMOD_LEVEL | IOAPIC_REDTBL_INTMASK_MASKED | (uint64_t(destination) << 56);
	ioapicSetRedirectionEntry(io, irq - io->globalSystemInterruptBase, entry);
	return G_ACPI_SUCCESSFUL;
}

g_acpi_status acpiPlatformInstallIrq(g_task* task, uint32_t irq)
{
	auto status = acpiPlatformValidateIrq(task, irq);
	if(status != G_ACPI_SUCCESSFUL)
		return status;

	g_interrupt_guard interrupts;
	g_mutex_guard guard(&acpiPlatformLock);
	if(acpiPlatformIrqs[irq] != G_TID_NONE || requestsGetHandlerTask(irq))
		return G_ACPI_BUSY;

	status = acpiPlatformRouteIrq(irq);
	if(status != G_ACPI_SUCCESSFUL)
		return status;
	acpiPlatformPending[irq] = false;
	acpiPlatformIrqs[irq] = task->id;
	return G_ACPI_SUCCESSFUL;
}

g_acpi_status acpiPlatformRearmIrq(g_task* task, uint32_t irq)
{
	auto status = acpiPlatformValidateIrq(task, irq);
	if(status != G_ACPI_SUCCESSFUL)
		return status;
	g_interrupt_guard interrupts;
	g_mutex_guard guard(&acpiPlatformLock);
	auto owner = acpiPlatformAcquireIrqOwner(task, irq);
	if(!owner)
		return G_ACPI_NOT_PERMITTED;
	acpiPlatformPending[irq] = false;
	acpiPlatformMask(irq, false);
	taskingReleaseTask(owner);
	return G_ACPI_SUCCESSFUL;
}

g_acpi_status acpiPlatformMaskIrq(g_task* task, uint32_t irq)
{
	auto status = acpiPlatformValidateIrq(task, irq);
	if(status != G_ACPI_SUCCESSFUL)
		return status;
	g_interrupt_guard interrupts;
	g_mutex_guard guard(&acpiPlatformLock);
	auto owner = acpiPlatformAcquireIrqOwner(task, irq);
	if(!owner)
		return G_ACPI_NOT_PERMITTED;
	acpiPlatformMask(irq, true);
	taskingReleaseTask(owner);
	return G_ACPI_SUCCESSFUL;
}

g_acpi_status acpiPlatformRemoveIrq(g_task* task, uint32_t irq)
{
	auto status = acpiPlatformValidateIrq(task, irq);
	if(status != G_ACPI_SUCCESSFUL)
		return status;
	g_interrupt_guard interrupts;
	g_mutex_guard guard(&acpiPlatformLock);
	auto owner = acpiPlatformAcquireIrqOwner(task, irq);
	if(!owner)
		return G_ACPI_NOT_PERMITTED;
	acpiPlatformMask(irq, true);
	acpiPlatformPending[irq] = false;
	acpiPlatformIrqs[irq] = G_TID_NONE;
	taskingWake(owner);
	taskingReleaseTask(owner);
	return G_ACPI_SUCCESSFUL;
}

g_acpi_status acpiPlatformWait(g_task* task, uint32_t irq)
{
	if(!irq || irq >= 96)
		return G_ACPI_INVALID_ARGUMENT;
	g_acpi_status result = G_ACPI_NOT_PERMITTED;
	while(taskingWaitIf(
			[&]()
			{
				g_interrupt_guard interrupts;
				g_mutex_guard guard(&acpiPlatformLock);
				if(acpiPlatformIrqs[irq] != task->id || task->terminationRequested)
					return false;
				if(acpiPlatformPending[irq])
				{
					result = G_ACPI_SUCCESSFUL;
					return false;
				}
				taskingPrepareWait(task, __func__);
				return true;
			}))
	{
	}
	return result;
}

uint64_t acpiPlatformMap(g_task* task, uint64_t address, uint32_t length)
{
	if(!length || length > UINT32_MAX - G_PAGE_SIZE || !memoryPhysicalIsRangeAddressable(address, length))
		return 0;

	auto offset = address & (G_PAGE_SIZE - 1);
	uint32_t pages = G_PAGE_ALIGN_UP(uint64_t(length) + offset) / G_PAGE_SIZE;
	auto pool = task->process->virtualRangePool;
	g_mutex_guard guard(&task->process->lock);

	auto base = addressRangePoolAllocate(pool, pages, G_PROC_VIRTUAL_RANGE_FLAG_WEAK);
	if(!base)
		return 0;

	uint32_t mapped = 0;
	for(; mapped < pages; ++mapped)
	{
		if(!pagingMapPage(base + mapped * G_PAGE_SIZE, G_PAGE_ALIGN_DOWN(address) + mapped * G_PAGE_SIZE,
						  G_PAGE_TABLE_USER_DEFAULT,
						  G_PAGE_USER_DEFAULT | G_PAGE_CACHE_DISABLE | G_PAGE_UNMANAGED_FLAG))
			break;
	}

	if(mapped != pages)
	{
		while(mapped)
			pagingUnmapPage(base + --mapped * G_PAGE_SIZE);
		addressRangePoolFree(pool, base);
		return 0;
	}
	return base + offset;
}
