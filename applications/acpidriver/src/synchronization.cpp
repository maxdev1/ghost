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

extern "C" uacpi_handle uacpi_kernel_create_semaphore(uacpi_u32 units)
{
	auto value = static_cast<uint32_t*>(malloc(sizeof(uint32_t)));
	if(value)
		*value = units;
	return value;
}

extern "C" void uacpi_kernel_free_semaphore(uacpi_handle value) { free(value); }

extern "C" uacpi_status uacpi_kernel_wait_for_semaphore(uacpi_handle handle, uacpi_u16 timeout)
{
	auto value = static_cast<uint32_t*>(handle);
	auto deadline = uacpi_kernel_get_nanoseconds_since_boot() + uint64_t(timeout) * 1000000ULL;
	for(;;)
	{
		auto units = __atomic_load_n(value, __ATOMIC_ACQUIRE);
		if(units && __atomic_compare_exchange_n(value, &units, units - 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
			return UACPI_STATUS_OK;

		if(timeout != 0xffff && (!timeout || uacpi_kernel_get_nanoseconds_since_boot() >= deadline))
			return UACPI_STATUS_TIMEOUT;

		if(acpiPlatformInterruptsEnabled())
			g_sleep(1);
		else
			asm volatile("pause");
	}
}

extern "C" void uacpi_kernel_signal_semaphore(uacpi_handle handle)
{
	__atomic_add_fetch(static_cast<uint32_t*>(handle), 1, __ATOMIC_RELEASE);
}

extern "C" uacpi_thread_id uacpi_kernel_get_thread_id() { return reinterpret_cast<void*>(uintptr_t(g_get_tid())); }

extern "C" uacpi_interrupt_state uacpi_kernel_disable_interrupts()
{
	uint64_t flags;
	asm volatile("pushfq; popq %0; cli" : "=r"(flags)::"memory");
	return flags;
}

extern "C" void uacpi_kernel_restore_interrupts(uacpi_interrupt_state flags)
{
	if(flags & (1 << 9))
		asm volatile("sti" ::: "memory");
}

extern "C" uacpi_handle uacpi_kernel_create_spinlock() { return uacpi_kernel_create_semaphore(0); }

extern "C" void uacpi_kernel_free_spinlock(uacpi_handle value) { free(value); }

extern "C" uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle handle)
{
	auto flags = uacpi_kernel_disable_interrupts();

	while(__atomic_exchange_n(static_cast<uint32_t*>(handle), 1, __ATOMIC_ACQUIRE))
		asm volatile("pause");

	return flags;
}

extern "C" void uacpi_kernel_unlock_spinlock(uacpi_handle handle, uacpi_cpu_flags flags)
{
	__atomic_store_n(static_cast<uint32_t*>(handle), 0, __ATOMIC_RELEASE);
	uacpi_kernel_restore_interrupts(flags);
}
