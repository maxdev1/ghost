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

#ifndef GHOST_INTERRUPTS_IDT_IDT
#define GHOST_INTERRUPTS_IDT_IDT

#include "ghost/stdint.h"
#include <system/interrupts/descriptors/idt.hpp>

/**
 * Interrupt descriptor table setup class
 */
class g_idt {
public:

	/**
	 * Installs the interrupt descriptor table.
	 */
	static void prepare();

	/**
	 * Installs the interrupt descriptor table.
	 *
	 * @reentrancy the same table is loaded on each core, therefore no locking necessary
	 */
	static void load();

	/**
	 * Fills the given values into the given IDT entry
	 *
	 * @param index			gate index
	 * @param base			gate base address
	 * @param kernelSegment kernel code segment descriptor index
	 * @param flags			the flags to apply
	 */
	static void createGate(uint32_t index, uint32_t base, uint16_t kernelSegment, uint8_t flags);

};

#endif
