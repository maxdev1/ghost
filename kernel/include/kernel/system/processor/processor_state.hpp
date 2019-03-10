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

#ifndef __KERNEL_PROCESSOR_STATE__
#define __KERNEL_PROCESSOR_STATE__

#include "ghost/stdint.h"

/**
 * Image of the stack on interrupt
 */
struct g_processor_state
{
	// Pushed by the interrupt request/routine handler
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;

	// Pushed by ISR handler if available
	uint32_t intr;
	uint32_t error;

	// Pushed by the processor
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;

	// Only pushed/popped on Ring 3 <-> Ring 0 switches
	uint32_t esp;
	uint32_t ss;
}__attribute__((packed));

/**
 * Image of the stack on interrupt from a VM86 task
 */
struct g_processor_state_vm86
{
	// The default contents are still pushed
	g_processor_state defaultFrame;

	// Additionally pushed by the processor before the other stuff
	uint32_t es;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
}__attribute__((packed));

#endif
