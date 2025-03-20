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

#include "kernel/system/interrupts/idt.hpp"
#include "shared/logger/logger.hpp"
#include "kernel/memory/gdt.hpp"
#include <ghost/memory/types.h>
#include <kernel/system/processor/processor.hpp>

/**
 * IDT pointer structure
 */
g_idt_pointer idtPointer;

/**
 * Interrupt descriptor table (consisting of 256 IDT entries)
 */
__attribute__((aligned(8))) g_idt_entry idt[256];

void idtCreateGate(uint32_t index, void* base, uint8_t flags, uint8_t ist)
{
	uint64_t baseAddr = (uint64_t) base;
	idt[index].baseLow = baseAddr & 0xFFFF;
	idt[index].baseMid = (baseAddr >> 16) & 0xFFFF;
	idt[index].baseHigh = (baseAddr >> 32) & 0xFFFFFFFF;
	idt[index].kernelSegment = G_GDT_DESCRIPTOR_KERNEL_CODE;
	idt[index].reserved = 0;
	idt[index].ist = ist & 0x7; // Only use the 3 lowest bits for IST
	idt[index].flags = flags;
}

void idtCreateGate(uint32_t index, void* base, uint8_t flags)
{
	idtCreateGate(index, base, flags, 0); // Default IST value is 0
}


void idtInitialize()
{
	idtPointer.limit = (sizeof(g_idt_entry) * 256) - 1;
	idtPointer.base = (g_address) &idt;

	uint8_t* idtp = (uint8_t*) (&idt);
	for(uint32_t i = 0; i < sizeof(g_idt_entry) * 256; i++)
	{
		idtp[i] = 0;
	}
}

void idtInitializeLocal()
{
	auto idtPointerAddr = (g_address) &idtPointer;
	asm volatile (
		"lidt (%0)"
		:
		: "r" (idtPointerAddr)
		: "memory"
	);
}
