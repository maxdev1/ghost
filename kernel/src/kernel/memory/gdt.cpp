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
#include "kernel/memory/heap.hpp"
#include "kernel/system/processor/processor.hpp"
#include "shared/memory/gdt_mounter.hpp"

static g_gdt_list_entry** gdtList;

void gdtPrepare()
{
	uint32_t cores = processorGetNumberOfProcessors();
	gdtList = (g_gdt_list_entry**) heapAllocate(sizeof(g_gdt_list_entry*) * cores);

	for(uint32_t i = 0; i < cores; i++)
		gdtList[i] = (g_gdt_list_entry*) heapAllocate(sizeof(g_gdt_list_entry));

}

void gdtInitialize()
{
	g_gdt_list_entry* localGdt = gdtList[processorGetCurrentId()];

	localGdt->ptr.limit = (sizeof(g_gdt_entry) * G_GDT_NUM_ENTRIES) - 1;
	localGdt->ptr.base = (uint32_t) &localGdt->entry;

	// Null descriptor, position 0x00
	gdtCreateGate(&localGdt->entry[0], 0, 0, 0, 0);

	// Kernel code segment descriptor, position 0x08
	gdtCreateGate(&localGdt->entry[1], 0, 0xFFFFFFFF, G_ACCESS_BYTE__KERNEL_CODE_SEGMENT, 0xCF);

	// Kernel data segment descriptor, position 0x10
	gdtCreateGate(&localGdt->entry[2], 0, 0xFFFFFFFF, G_ACCESS_BYTE__KERNEL_DATA_SEGMENT, 0xCF);

	// User code segment descriptor, position 0x18
	gdtCreateGate(&localGdt->entry[3], 0, 0xFFFFFFFF, G_ACCESS_BYTE__USER_CODE_SEGMENT, 0xCF);

	// User data segment descriptor, position 0x20
	gdtCreateGate(&localGdt->entry[4], 0, 0xFFFFFFFF, G_ACCESS_BYTE__USER_DATA_SEGMENT, 0xCF);

	// TSS descriptor, position 0x28
	gdtCreateGate(&localGdt->entry[5], (uint32_t) &localGdt->tss, sizeof(g_tss), G_ACCESS_BYTE__TSS_386_SEGMENT, 0x40);
	localGdt->tss.ss0 = G_GDT_DESCRIPTOR_KERNEL_DATA; // kernel data segment
	localGdt->tss.esp0 = 0; // will later be initialized

	// User thread pointer segment 0x30
	gdtCreateGate(&localGdt->entry[6], 0, 0xFFFFFFFF, G_ACCESS_BYTE__USER_DATA_SEGMENT, 0xCF);

	_loadGdt((uint32_t) &localGdt->ptr);
	_loadTss(G_GDT_DESCRIPTOR_TSS);
}

void gdtSetTssEsp0(uint32_t esp0)
{
	gdtList[processorGetCurrentId()]->tss.esp0 = esp0;
}

void gdtSetUserThreadObjectAddress(g_virtual_address userThreadObjectAddress)
{
	g_gdt_list_entry* list_entry = gdtList[processorGetCurrentId()];
	gdtCreateGate(&list_entry->entry[6], userThreadObjectAddress, 0xFFFFFFFF, G_ACCESS_BYTE__USER_DATA_SEGMENT, 0xCF);
}
