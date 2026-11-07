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

#ifndef GHOST_API_SYSCALL
#define GHOST_API_SYSCALL

#include "common.h"
#include "stdint.h"
#include "memory/types.h"
#include "syscall/definitions.h"

__BEGIN_C

/**
 * Invokes a kernel system call with its operation-specific data block.
 *
 * The operation determines the layout, pointer requirements and result fields. Prefer the typed
 * libapi wrappers when available.
 *
 * @param[in] call A G_SYSCALL_* operation number.
 * @param[in,out] data Address of the operation's request/response structure, or zero for
 *                     operations without one.
 * @security-level APPLICATION; the selected operation may require DRIVER or KERNEL.
 */
void g_syscall(uint32_t call, g_address data);

__END_C

#endif
