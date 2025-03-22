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

#include "kernel/memory/gdt.hpp"
#include "kernel/utils/debug.hpp"
#include "shared/logger/logger.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/system/processor/processor.hpp"
#include "shared/memory/memory.hpp"

static g_gdt** gdtList;

void _gdtWriteEntry(g_gdt_descriptor* entry, uint64_t base, uint64_t limit, uint8_t access, uint8_t granularity);
void _gdtWriteTssEntry(g_gdt_tss_descriptor* entry, uint64_t base, uint64_t limit, uint8_t access, uint8_t granularity);

void gdtInitialize()
{
	uint32_t cores = processorGetNumberOfProcessors();
	gdtList = (g_gdt**) heapAllocate(sizeof(g_gdt*) * cores);

	for(uint32_t i = 0; i < cores; i++)
	{
		// TODO still no aligned allocator
		auto gdtMemory = heapAllocateClear(sizeof(g_gdt) * 2);
		gdtList[i] = (g_gdt*) G_ALIGN_UP((g_address) gdtMemory, 0x10);
	}
}

void gdtInitializeLocal()
{
	g_gdt* localGdt = gdtList[processorGetCurrentId()];
	localGdt->ptr.limit = sizeof(g_gdt_descriptor) * G_GDT_BASE_LEN + sizeof(g_gdt_tss_descriptor) - 1;
	localGdt->ptr.base = (g_address) &localGdt->entry;

	// Null descriptor, position 0x00
	_gdtWriteEntry(&localGdt->entry[0], 0, 0, 0, 0);

	// Kernel code segment descriptor, position 0x08
	_gdtWriteEntry(&localGdt->entry[1], 0, 0xFFFFFFFF, G_ACCESS_BYTE__KERNEL_CODE_SEGMENT,
	               G_GDT_GRANULARITY_4KB | G_GDT_GRANULARITY_64BIT);

	// Kernel data segment descriptor, position 0x10
	_gdtWriteEntry(&localGdt->entry[2], 0, 0xFFFFFFFF, G_ACCESS_BYTE__KERNEL_DATA_SEGMENT,
	               G_GDT_GRANULARITY_4KB | G_GDT_GRANULARITY_64BIT);

	// User code segment descriptor, position 0x18
	_gdtWriteEntry(&localGdt->entry[3], 0, 0xFFFFFFFF, G_ACCESS_BYTE__USER_CODE_SEGMENT,
	               G_GDT_GRANULARITY_4KB | G_GDT_GRANULARITY_64BIT);

	// User data segment descriptor, position 0x20
	_gdtWriteEntry(&localGdt->entry[4], 0, 0xFFFFFFFF, G_ACCESS_BYTE__USER_DATA_SEGMENT,
	               G_GDT_GRANULARITY_4KB | G_GDT_GRANULARITY_64BIT);

	// TSS descriptor, position 0x28
	_gdtWriteTssEntry(&localGdt->entryTss, (g_address) &localGdt->tss, sizeof(g_tss) - 1,
	                  G_ACCESS_BYTE__TSS_386_SEGMENT, 0);
	memorySetBytes(&localGdt->tss, 0, sizeof(g_tss));
	localGdt->tss.rsp0 = 0;

	logInfo("gdt last entry: %x", &localGdt->entry[4]);
	logInfo("gdt tss entry:  %x", &localGdt->entryTss);
	logInfo("tss:            %x", &localGdt->tss);
	hexDump8(&localGdt->tss);

	// Load the GDT & TSS
	asm volatile("lgdt %0" : : "m" (localGdt->ptr));
	asm volatile(
		"movw $0x10, %%ax;"
		"movw %%ax, %%ds;"
		"movw %%ax, %%es;"
		"movw %%ax, %%fs;"
		"movw %%ax, %%gs;"
		"movw %%ax, %%ss;"
		"pushq $0x08;"
		"pushq $reloadCS;"
		"lretq;"
		"reloadCS:"
		: : : "ax"
	);
	asm volatile("ltr %0"::"r"((uint16_t) G_GDT_DESCRIPTOR_TSS) : "memory");
}

void gdtSetTssRsp0(g_address rsp0)
{
	gdtList[processorGetCurrentId()]->tss.rsp0 = rsp0;
}

g_address gdtGetTssRsp0()
{
	return gdtList[processorGetCurrentId()]->tss.rsp0;
}

void gdtSetTlsAddresses(g_user_threadlocal* userThreadLocal, g_kernel_threadlocal* kernelThreadLocal)
{
	// TODO define constant IA32_FS_Base = 0xC0000100 and IA32_GS_BASE = 0xC0000101
	auto userAddress = (g_address) userThreadLocal;
	processorWriteMsr(0xC0000100, userAddress & 0xFFFFFFFF, userAddress >> 32);

	auto kernelAddress = (g_address) kernelThreadLocal;
	processorWriteMsr(0xC0000101, kernelAddress & 0xFFFFFFFF, kernelAddress >> 32);
}

void _gdtWriteEntry(g_gdt_descriptor* entry, uint64_t base, uint64_t limit, uint8_t access, uint8_t granularity)
{
	entry->baseLow = (base & 0xFFFF);
	entry->baseMiddle = (base >> 16) & 0xFF;
	entry->baseHigh = (base >> 24) & 0xFF;
	entry->limitLow = (limit & 0xFFFF);
	entry->granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);
	entry->access = access;
}

void _gdtWriteTssEntry(g_gdt_tss_descriptor* entry, uint64_t base, uint64_t limit, uint8_t access, uint8_t granularity)
{
	_gdtWriteEntry(&entry->main, base, limit, access, granularity);
	entry->baseUpper = (base >> 32) & 0xFFFFFFFF;
}
