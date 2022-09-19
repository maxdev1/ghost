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

#ifndef GHOST_API_CALLS_VM86CALLS
#define GHOST_API_CALLS_VM86CALLS

#include "ghost/ramdisk.h"

/**
 * @field interrupt
 * 		the interrupt to call
 *
 * @field in
 * 		the input registers
 *
 * @field out
 * 		the output registers
 *
 * @field status
 * 		status of the call
 */
typedef struct {
	uint32_t interrupt;
	g_vm86_registers in;
	g_vm86_registers* out;

	g_vm86_call_status status;
}__attribute__((packed)) g_syscall_call_vm86;

#endif
