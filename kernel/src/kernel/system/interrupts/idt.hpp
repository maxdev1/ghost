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

#ifndef __KERNEL_IDT__
#define __KERNEL_IDT__

#include <ghost/stdint.h>

/**
 * Flags for flag section of IDT descriptors
 */
#define G_IDT_FLAGS_SEGMENT_PRESENT          0b10000000

#define G_IDT_FLAGS_PRIVILEGE_RING0          0b00000000
#define G_IDT_FLAGS_PRIVILEGE_RING1          0b00100000
#define G_IDT_FLAGS_PRIVILEGE_RING2          0b01000000
#define G_IDT_FLAGS_PRIVILEGE_RING3          0b01100000

#define G_IDT_FLAGS_GATE_TYPE_TASK           0b00000101
#define G_IDT_FLAGS_GATE_TYPE_INTERRUPT_16   0b00000110
#define G_IDT_FLAGS_GATE_TYPE_INTERRUPT_32   0b00001110
#define G_IDT_FLAGS_GATE_TYPE_TRAP_16        0b00000111
#define G_IDT_FLAGS_GATE_TYPE_TRAP_32        0b00001111


#define G_IDT_FLAGS_INTERRUPT_GATE_KERNEL   (G_IDT_FLAGS_SEGMENT_PRESENT | G_IDT_FLAGS_GATE_TYPE_INTERRUPT_32 | G_IDT_FLAGS_PRIVILEGE_RING0)
#define G_IDT_FLAGS_INTERRUPT_GATE_USER     (G_IDT_FLAGS_SEGMENT_PRESENT | G_IDT_FLAGS_GATE_TYPE_INTERRUPT_32 | G_IDT_FLAGS_PRIVILEGE_RING3)

/**
 * Structure of the IDT pointer
 */
struct g_idt_pointer
{
	uint16_t limit;
	uint32_t base;
}__attribute__((packed));

/**
 * Structure of an IDT entry
 */
struct g_idt_entry
{
	uint16_t baseLow;
	uint16_t kernelSegment;
	uint8_t zero;
	uint8_t flags;
	uint16_t baseHigh;
}__attribute__((packed));

/**
 * Loads the interrupt descriptor table, using the IDT pointer structure at the given address
 *
 * @param idtPointerAddress	the address of the IDT pointer to load from
 * @see assembly
 */
extern "C" void _loadIdt(uint32_t idtPointerAddress);

/**
 * Installs the interrupt descriptor table.
 */
void idtPrepare();

/**
 * Installs the interrupt descriptor table.
 *
 * @reentrancy the same table is loaded on each core, therefore no locking necessary
 */
void idtLoad();

/**
 * Fills the given values into the given IDT entry
 *
 * @param index			gate index
 * @param base			gate base address
 * @param flags			the flags to apply
 */
void idtCreateGate(uint32_t index, void* base, uint8_t flags);

#endif
