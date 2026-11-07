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
	bool armed;
	g_irq_handle binding;
	g_irq_status status;
	g_user_mutex resume;
};

static constexpr unsigned G_ACPI_IRQ_COUNT = 96;
static g_acpi_irq* acpiIrqs[G_ACPI_IRQ_COUNT];
static bool acpiIrqsPaused;
static g_user_mutex acpiIrqLock;
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
	acpiIrqLock = g_mutex_initialize();
	acpiWorkLock = g_mutex_initialize();
	acpiWorkReady = g_mutex_initialize();
	if(acpiIrqLock && acpiWorkLock && acpiWorkReady)
	{
		g_mutex_acquire(acpiWorkReady);
		if(g_create_task_a(reinterpret_cast<void*>(acpiPlatformWorkThread), 0) != G_TID_NONE)
			return true;
	}

	if(acpiIrqLock)
		g_mutex_destroy(acpiIrqLock);
	if(acpiWorkLock)
		g_mutex_destroy(acpiWorkLock);
	if(acpiWorkReady)
		g_mutex_destroy(acpiWorkReady);

	acpiIrqLock = acpiWorkLock = acpiWorkReady = 0;

	return false;
}

static g_irq_status acpiPlatformArm(g_acpi_irq* irq)
{
	if(irq->armed || acpiIrqsPaused || irq->stop)
		return G_IRQ_SUCCESSFUL;
	auto status = g_irq_complete(irq->binding);
	irq->armed = status == G_IRQ_SUCCESSFUL;
	return status;
}

static void acpiPlatformRelease(g_acpi_irq* irq)
{
	if(irq->binding != G_IRQ_HANDLE_NONE)
	{
		g_irq_release(irq->binding);
		irq->binding = G_IRQ_HANDLE_NONE;
	}
	irq->armed = false;
}

static void acpiPlatformIrqThread(void* context)
{
	auto irq = static_cast<g_acpi_irq*>(context);
	g_irq_handle binding = G_IRQ_HANDLE_NONE;
	auto status = g_irq_bind(G_IRQ_SOURCE_ACPI, irq->irq, &binding);

	g_mutex_acquire(acpiIrqLock);
	irq->binding = binding;
	if(status == G_IRQ_SUCCESSFUL)
		status = acpiPlatformArm(irq);
	if(status != G_IRQ_SUCCESSFUL)
		acpiPlatformRelease(irq);
	irq->status = status;
	memoryAtomicStore(&irq->ready, true, G_MEMORY_ORDER_RELEASE);
	g_mutex_release(acpiIrqLock);

	if(status != G_IRQ_SUCCESSFUL)
		return;

	for(;;)
	{
		g_mutex_acquire(acpiIrqLock);

		bool stop = irq->stop;
		bool paused = acpiIrqsPaused;
		status = acpiPlatformArm(irq);

		g_mutex_release(acpiIrqLock);

		if(stop || status != G_IRQ_SUCCESSFUL)
			break;
		if(paused)
		{
			g_mutex_acquire(irq->resume);
			continue;
		}

		if(g_irq_wait(binding, 0) != G_IRQ_DELIVERED)
			break;

		g_mutex_acquire(acpiIrqLock);

		irq->armed = false;
		bool dispatch = !irq->stop && !acpiIrqsPaused;
		if(dispatch)
			memoryAtomicAddFetch(&acpiIrqActive, 1, G_MEMORY_ORDER_RELAXED);

		g_mutex_release(acpiIrqLock);

		if(dispatch)
		{
			irq->handler(irq->context);
			memoryAtomicSubFetch(&acpiIrqActive, 1, G_MEMORY_ORDER_RELEASE);
		}
	}

	g_mutex_acquire(acpiIrqLock);
	acpiPlatformRelease(irq);
	g_mutex_release(acpiIrqLock);
}

extern "C" uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 number, uacpi_interrupt_handler handler,
															   uacpi_handle context, uacpi_handle* out)
{
	if(!number || number >= G_ACPI_IRQ_COUNT || !handler || !out)
		return UACPI_STATUS_INVALID_ARGUMENT;
	*out = nullptr;

	auto irq = static_cast<g_acpi_irq*>(calloc(1, sizeof(g_acpi_irq)));
	if(!irq)
		return UACPI_STATUS_OUT_OF_MEMORY;
	irq->irq = number;
	irq->handler = handler;
	irq->context = context;
	irq->resume = g_mutex_initialize();
	if(!irq->resume)
	{
		free(irq);
		return UACPI_STATUS_OUT_OF_MEMORY;
	}

	g_mutex_acquire(irq->resume);
	g_mutex_acquire(acpiIrqLock);
	if(acpiIrqs[number])
	{
		g_mutex_release(acpiIrqLock);
		g_mutex_destroy(irq->resume);
		free(irq);
		return UACPI_STATUS_ALREADY_EXISTS;
	}
	acpiIrqs[number] = irq;
	g_mutex_release(acpiIrqLock);

	irq->thread = g_create_task_da(reinterpret_cast<void*>(acpiPlatformIrqThread), irq, 0);
	if(irq->thread != G_TID_NONE)
	{
		while(!memoryAtomicLoad(&irq->ready, G_MEMORY_ORDER_ACQUIRE))
			g_sleep(1);

		if(irq->status == G_IRQ_SUCCESSFUL)
		{
			*out = irq;
			return UACPI_STATUS_OK;
		}
		g_join(irq->thread);
	}

	auto result = irq->thread == G_TID_NONE ? UACPI_STATUS_INTERNAL_ERROR : UACPI_STATUS_UNIMPLEMENTED;
	g_mutex_acquire(acpiIrqLock);
	acpiIrqs[number] = nullptr;
	g_mutex_release(acpiIrqLock);
	g_mutex_destroy(irq->resume);
	free(irq);
	return result;
}

extern "C" uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler, uacpi_handle handle)
{
	auto irq = static_cast<g_acpi_irq*>(handle);

	g_mutex_acquire(acpiIrqLock);
	irq->stop = true;
	acpiPlatformRelease(irq);
	g_mutex_release(irq->resume);
	g_mutex_release(acpiIrqLock);
	g_join(irq->thread);

	g_mutex_acquire(acpiIrqLock);
	acpiIrqs[irq->irq] = nullptr;
	g_mutex_release(acpiIrqLock);
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

bool acpiPlatformPause(bool paused)
{
	if(!acpiIrqLock)
		return true;
	bool success = true;
	g_mutex_acquire(acpiIrqLock);
	acpiIrqsPaused = paused;
	for(auto irq: acpiIrqs)
	{
		if(!irq || irq->stop)
			continue;

		if(irq->binding == G_IRQ_HANDLE_NONE)
		{
			if(memoryAtomicLoad(&irq->ready, G_MEMORY_ORDER_ACQUIRE))
				success = false;
			continue;
		}

		if(paused)
		{
			if(g_irq_mask(irq->binding) != G_IRQ_SUCCESSFUL)
				success = false;
			irq->armed = false;
		}
		else
		{
			if(acpiPlatformArm(irq) != G_IRQ_SUCCESSFUL)
				success = false;
			g_mutex_release(irq->resume);
		}
	}
	g_mutex_release(acpiIrqLock);

	if(paused)
	{
		uint64_t started = g_millis();
		while(memoryAtomicLoad(&acpiIrqActive, G_MEMORY_ORDER_ACQUIRE))
		{
			if(g_millis() - started >= G_ACPI_TIMEOUT_MS)
				return false;
			g_sleep(1);
		}
	}
	return success;
}
