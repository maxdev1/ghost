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

#include "ghost/user.h"

/**
 *
 */
uint8_t g_atomic_try_lock(uint8_t* atom) {
	return g_atomic_try_lock_dual(atom, 0);
}

/**
 *
 */
uint8_t g_atomic_try_lock_dual(uint8_t* atom_1, uint8_t* atom_2) {
	g_syscall_atomic_lock data;
	data.atom_1 = atom_1;
	data.atom_2 = atom_2;
	data.try_only = true;
	g_syscall(G_SYSCALL_ATOMIC_LOCK, (uint32_t) &data);
	return data.was_set;
}
