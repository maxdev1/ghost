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
#include "shared/logger/logger.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/system/processor/processor.hpp"

static g_gdt** gdtList;

void _gdtCreateGate(g_gdt_descriptor* gdtEntry, uint64_t base, uint64_t limit, uint8_t access, uint8_t granularity);
void _gdtCreateGate64(g_gdt_descriptor_64* gdtEntry, uint64_t base, uint64_t limit, uint8_t access,
                      uint8_t granularity);

void gdtInitialize()
{
	uint32_t cores = processorGetNumberOfProcessors();

	// TODO aligned memory would be better
	gdtList = (g_gdt**) heapAllocate(sizeof(g_gdt*) * cores);

	for(uint32_t i = 0; i < cores; i++)
		gdtList[i] = (g_gdt*) heapAllocateClear(sizeof(g_gdt));
}

void gdtInitializeLocal()
{
	g_gdt* localGdt = gdtList[processorGetCurrentId()];
	localGdt->ptr.limit = (sizeof(g_gdt_descriptor) * G_GDT_LENGTH) - 1;
	localGdt->ptr.base = (g_address) &localGdt->entry;

	// Null descriptor, position 0x00
	_gdtCreateGate(&localGdt->entry[0], 0, 0, 0, 0);

	// Kernel code segment descriptor, position 0x08
	_gdtCreateGate(&localGdt->entry[1], 0, 0, G_ACCESS_BYTE__KERNEL_CODE_SEGMENT, 0xA0);

	// Kernel data segment descriptor, position 0x10
	_gdtCreateGate(&localGdt->entry[2], 0, 0, G_ACCESS_BYTE__KERNEL_DATA_SEGMENT, 0xC0);

	// User code segment descriptor, position 0x18
	_gdtCreateGate(&localGdt->entry[3], 0, 0, G_ACCESS_BYTE__USER_CODE_SEGMENT, 0xA0);

	// User data segment descriptor, position 0x20
	_gdtCreateGate(&localGdt->entry[4], 0, 0, G_ACCESS_BYTE__USER_DATA_SEGMENT, 0xC0);

	// TSS descriptor, position 0x28 (16-byte descriptor)
	_gdtCreateGate64((g_gdt_descriptor_64*) &localGdt->entry[5], (g_address) &localGdt->tss, sizeof(g_tss) - 1,
	                 G_ACCESS_BYTE__TSS_386_SEGMENT, 0x00);
	localGdt->tss.rsp0 = 0;
	// TODO just place a partial struct at the end of the GDT instead of createGate64, instead of setting length to 7

	auto gdtPtr = localGdt->ptr;
	asm volatile("lgdt %0" : : "m" (gdtPtr));
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

void gdtSetTlsAddresses(g_user_threadlocal* userThreadLocal, g_kernel_threadlocal* kernelThreadLocal)
{
	// TODO define constant IA32_FS_Base = 0xC0000100 and IA32_GS_BASE = 0xC0000101
	auto userAddress = (g_address) userThreadLocal;
	processorWriteMsr(0xC0000100, userAddress & 0xFFFFFFFF, userAddress >> 32);

	auto kernelAddress = (g_address) kernelThreadLocal;
	processorWriteMsr(0xC0000101, kernelAddress & 0xFFFFFFFF, kernelAddress >> 32);
}

void _gdtCreateGate(g_gdt_descriptor* entry, uint64_t base, uint64_t limit, uint8_t access, uint8_t granularity)
{
	entry->baseLow = (base & 0xFFFF);
	entry->baseMiddle = (base >> 16) & 0xFF;
	entry->baseHigh = (base >> 24) & 0xFF;
	entry->limitLow = (limit & 0xFFFF);
	entry->granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);
	entry->access = access;
}

void _gdtCreateGate64(g_gdt_descriptor_64* entry, uint64_t base, uint64_t limit, uint8_t access, uint8_t granularity)
{
	entry->baseLow = (base & 0xFFFF);
	entry->baseMiddle = (base >> 16) & 0xFF;
	entry->baseHigh = (base >> 24) & 0xFF;
	entry->limitLow = (limit & 0xFFFF);
	entry->granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);
	entry->access = access;
	entry->baseUpper = (base >> 32) & 0xFFFFFFFF;
}
