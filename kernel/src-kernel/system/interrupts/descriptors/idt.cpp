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

#include <system/interrupts/descriptors/idt.hpp>
#include <system/interrupts/descriptors/idt_pointer.hpp>
#include <system/interrupts/descriptors/idt_entry.hpp>
#include <system/interrupts/descriptors/idt_mounter.hpp>
#include <system/interrupts/handling/interrupt_dispatcher.hpp>
#include <system/system.hpp>
#include <logger/logger.hpp>

/**
 * IDT pointer structure
 */
g_idt_pointer idtPointer;

/**
 * Interrupt descriptor table (consisting of 256 IDT entries)
 */
g_idt_entry idt[256];

/**
 * 
 */
void g_idt::createGate(uint32_t index, uint32_t base, uint16_t kernelSegment, uint8_t flags) {
	idt[index].baseLow = base & 0xFFFF;
	idt[index].baseHigh = (base >> 16) & 0xFFFF;
	idt[index].kernelSegment = kernelSegment;
	idt[index].zero = 0;
	idt[index].flags = flags;
}

/**
 * 
 */
void g_idt::prepare() {

	// Create the IDT pointer
	idtPointer.limit = (sizeof(g_idt_entry) * 256) - 1;
	idtPointer.base = (uint32_t) & idt;

	// Empty the IDT
	uint8_t* idtp = (uint8_t*) (&idt);
	for (uint32_t i = 0; i < sizeof(g_idt_entry) * 256; i++) {
		idtp[i] = 0;
	}

	// Install interrupt handlers
	g_interrupt_dispatcher::install();
}

/**
 * 
 */
void g_idt::load() {
	// Load the IDT
	g_log_debug("%! descriptor table lays at %h", "idt", &idt); g_log_debug("%! pointer at %h, base %h, limit %h", "idt", &idtPointer, idtPointer.base, idtPointer.limit);
	_loadIdt((uint32_t) & idtPointer);
	g_log_debug("%! loaded on core %i", "idt", g_system::currentProcessorId());
}

