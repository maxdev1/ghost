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

#include <cstdlib>
#include <ghost.h>
#include <uacpi/kernel_api.h>
#include "platform.hpp"

struct g_acpi_work
{
	uacpi_work_handler handler;
	uacpi_handle context;
	g_acpi_work* next;
};

struct g_acpi_irq
{
	uacpi_interrupt_handler handler;
	uacpi_handle context;
	uint32_t irq;
	g_tid thread;
	bool ready;
	bool stop;
	g_acpi_status status;
	g_user_mutex resume;
};

static constexpr unsigned G_ACPI_IRQ_COUNT = 96;
static g_acpi_irq* acpiIrqs[G_ACPI_IRQ_COUNT];
static bool acpiIrqsPaused;
static unsigned acpiIrqActive;

static g_user_mutex acpiWorkLock;
static g_user_mutex acpiWorkReady;
static g_acpi_work* acpiWorkHead;
static g_acpi_work* acpiWorkTail;
static unsigned acpiWorkActive;

static void acpiPlatformWorkThread()
{
	for(;;)
	{
		g_mutex_acquire(acpiWorkLock);
		auto work = acpiWorkHead;
		uacpi_work_handler handler = nullptr;
		uacpi_handle context = nullptr;
		if(work)
		{
			acpiWorkHead = work->next;
			if(!acpiWorkHead)
				acpiWorkTail = nullptr;
			handler = work->handler;
			context = work->context;
			++acpiWorkActive;
		}
		g_mutex_release(acpiWorkLock);
		if(!work)
		{
			g_mutex_acquire(acpiWorkReady);
			continue;
		}
		handler(context);
		g_mutex_acquire(acpiWorkLock);
		--acpiWorkActive;
		g_mutex_release(acpiWorkLock);
	}
}

bool acpiPlatformEventsInitialize()
{
	acpiWorkLock = g_mutex_initialize();
	acpiWorkReady = g_mutex_initialize();
	g_mutex_acquire(acpiWorkReady);
	return g_create_task_a(reinterpret_cast<void*>(acpiPlatformWorkThread), 0) != G_TID_NONE;
}

static void acpiPlatformIrqThread(void* context)
{
	auto irq = static_cast<g_acpi_irq*>(context);
	irq->status = g_acpi_configure_irq(irq->irq, G_ACPI_PLATFORM_IRQ_INSTALL);
	__atomic_store_n(&irq->ready, true, __ATOMIC_RELEASE);

	if(irq->status != G_ACPI_SUCCESSFUL)
		return;

	while(!__atomic_load_n(&irq->stop, __ATOMIC_ACQUIRE))
	{
		if(__atomic_load_n(&acpiIrqsPaused, __ATOMIC_ACQUIRE))
		{
			g_mutex_acquire(irq->resume);
			continue;
		}

		if(g_acpi_configure_irq(irq->irq, G_ACPI_PLATFORM_IRQ_REARM) != G_ACPI_SUCCESSFUL)
			break;
		if(g_acpi_configure_irq(irq->irq, G_ACPI_PLATFORM_IRQ_WAIT) != G_ACPI_SUCCESSFUL)
			break;

		__atomic_add_fetch(&acpiIrqActive, 1, __ATOMIC_ACQUIRE);
		if(!__atomic_load_n(&irq->stop, __ATOMIC_ACQUIRE) && !__atomic_load_n(&acpiIrqsPaused, __ATOMIC_ACQUIRE))
			irq->handler(irq->context);
		__atomic_sub_fetch(&acpiIrqActive, 1, __ATOMIC_RELEASE);
	}
	g_acpi_configure_irq(irq->irq, G_ACPI_PLATFORM_IRQ_REMOVE);
}

extern "C" uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 number, uacpi_interrupt_handler handler,
															   uacpi_handle context, uacpi_handle* out)
{
	if(!number || number >= G_ACPI_IRQ_COUNT || acpiIrqs[number])
		return UACPI_STATUS_INVALID_ARGUMENT;

	auto irq = static_cast<g_acpi_irq*>(calloc(1, sizeof(g_acpi_irq)));
	if(!irq)
		return UACPI_STATUS_OUT_OF_MEMORY;

	irq->irq = number;
	irq->handler = handler;
	irq->context = context;
	irq->resume = g_mutex_initialize();
	g_mutex_acquire(irq->resume);
	irq->thread = g_create_task_da(reinterpret_cast<void*>(acpiPlatformIrqThread), irq, 0);
	if(irq->thread == G_TID_NONE)
	{
		free(irq);
		return UACPI_STATUS_INTERNAL_ERROR;
	}

	while(!__atomic_load_n(&irq->ready, __ATOMIC_ACQUIRE))
		g_sleep(1);

	if(irq->status != G_ACPI_SUCCESSFUL)
	{
		g_join(irq->thread);
		free(irq);
		return UACPI_STATUS_UNIMPLEMENTED;
	}

	acpiIrqs[number] = irq;
	*out = irq;
	return UACPI_STATUS_OK;
}

extern "C" uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler, uacpi_handle handle)
{
	auto irq = static_cast<g_acpi_irq*>(handle);
	__atomic_store_n(&irq->stop, true, __ATOMIC_RELEASE);
	g_acpi_configure_irq(irq->irq, G_ACPI_PLATFORM_IRQ_REMOVE);
	g_mutex_release(irq->resume);
	g_join(irq->thread);
	acpiIrqs[irq->irq] = nullptr;
	g_mutex_destroy(irq->resume);
	free(irq);
	return UACPI_STATUS_OK;
}

extern "C" uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request*)
{
	return UACPI_STATUS_UNIMPLEMENTED;
}

extern "C" uacpi_handle uacpi_kernel_create_work_item() { return malloc(sizeof(g_acpi_work)); }

extern "C" void uacpi_kernel_free_work_item(uacpi_handle work) { free(work); }

extern "C" void uacpi_kernel_schedule_work(uacpi_work_type, uacpi_handle handle, uacpi_work_handler handler,
										   uacpi_handle context)
{
	auto work = static_cast<g_acpi_work*>(handle);
	g_mutex_acquire(acpiWorkLock);
	*work = {handler, context, nullptr};

	if(acpiWorkTail)
		acpiWorkTail->next = work;
	else
		acpiWorkHead = work;

	acpiWorkTail = work;
	g_mutex_release(acpiWorkLock);
	g_mutex_release(acpiWorkReady);
}

extern "C" uacpi_status uacpi_kernel_wait_for_work_completion()
{
	auto deadline = g_millis() + G_ACPI_TIMEOUT_MS;
	for(;;)
	{
		g_mutex_acquire(acpiWorkLock);
		bool done = !acpiWorkHead && !acpiWorkActive;
		g_mutex_release(acpiWorkLock);

		if(done)
			return UACPI_STATUS_OK;

		if(g_millis() >= deadline)
			return UACPI_STATUS_TIMEOUT;

		g_sleep(1);
	}
}

void acpiPlatformPause(bool paused)
{
	__atomic_store_n(&acpiIrqsPaused, paused, __ATOMIC_RELEASE);
	for(auto irq: acpiIrqs)
	{
		if(!irq)
			continue;

		g_acpi_configure_irq(irq->irq, paused ? G_ACPI_PLATFORM_IRQ_MASK : G_ACPI_PLATFORM_IRQ_REARM);

		if(!paused)
			g_mutex_release(irq->resume);
	}

	if(paused)
	{
		while(__atomic_load_n(&acpiIrqActive, __ATOMIC_ACQUIRE))
			g_sleep(1);
	}
}
