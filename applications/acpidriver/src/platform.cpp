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

#include "platform.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ghost.h>
#include <uacpi/kernel_api.h>

#define G_UACPI_KERNEL_LOG false

static uint64_t acpiRsdp;
static uint64_t acpiTscBase;
static uint64_t acpiTimeBase;
static uint64_t acpiTicksPerMs;

static uint64_t acpiPlatformTsc()
{
	uint32_t low;
	uint32_t high;
	asm volatile("rdtsc" : "=a"(low), "=d"(high));
	return (uint64_t(high) << 32) | low;
}

bool acpiPlatformInterruptsEnabled()
{
	uint64_t flags;
	asm volatile("pushfq; popq %0" : "=r"(flags));
	return flags & (1 << 9);
}

bool acpiPlatformStart(uint64_t rsdp)
{
	acpiRsdp = rsdp;

	if(!acpiPlatformPciInitialize())
		return false;

	auto before = acpiPlatformTsc();
	auto start = g_millis();
	g_sleep(10);

	auto elapsed = g_millis() - start;
	acpiTscBase = acpiPlatformTsc();
	acpiTimeBase = g_millis() * 1000000ULL;

	acpiTicksPerMs = (acpiTscBase - before) / (elapsed ? elapsed : 1);
	if(!acpiTicksPerMs)
		acpiTicksPerMs = 1;

	return acpiPlatformEventsInitialize();
}

extern "C" uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr* address)
{
	*address = acpiRsdp;
	return acpiRsdp ? UACPI_STATUS_OK : UACPI_STATUS_NOT_FOUND;
}

extern "C" void* uacpi_kernel_map(uacpi_phys_addr address, uacpi_size length)
{
	if(!length || length > UINT32_MAX - 4096)
		return UACPI_MAP_FAILED;

	auto mapped = g_acpi_map(address, length);
	return mapped ? mapped : UACPI_MAP_FAILED;
}

extern "C" void uacpi_kernel_unmap(void* address, uacpi_size)
{
	g_unmap(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(address) & ~uintptr_t(4095)));
}

#if G_UACPI_KERNEL_LOG
extern "C" void uacpi_kernel_log(uacpi_log_level, const uacpi_char* text)
{
	// cut off newline and limit
	char buf[128];
	auto len = strlen(text);
	len = len > sizeof(buf) - 2 ? sizeof(buf - 2) : len;
	memcpy(buf, text, len);
	if(len)
		buf[len - 1] = 0;

	klog("uacpi: %s", buf);
}
#else
extern "C" void uacpi_kernel_log(uacpi_log_level, const uacpi_char*) {}
#endif

extern "C" void* uacpi_kernel_alloc(uacpi_size size) { return malloc(size); }

extern "C" void uacpi_kernel_free(void* memory) { free(memory); }

extern "C" uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot()
{
	auto ticks = acpiPlatformTsc() - acpiTscBase;
	return acpiTimeBase + ticks / acpiTicksPerMs * 1000000ULL + ticks % acpiTicksPerMs * 1000000ULL / acpiTicksPerMs;
}

extern "C" void uacpi_kernel_stall(uacpi_u8 microseconds)
{
	auto until = uacpi_kernel_get_nanoseconds_since_boot() + microseconds * 1000ULL;
	while(uacpi_kernel_get_nanoseconds_since_boot() < until)
		asm volatile("pause");
}

extern "C" void uacpi_kernel_sleep(uacpi_u64 milliseconds)
{
	if(acpiPlatformInterruptsEnabled())
		g_sleep(milliseconds);
	else
	{
		for(uint64_t i = 0; i < milliseconds * 10; ++i)
			uacpi_kernel_stall(100);
	}
}
