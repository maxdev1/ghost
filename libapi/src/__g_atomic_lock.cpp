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

#include "__internal.h"
#include "ghost/user.h"

/**
 *
 */
g_bool __g_atomic_lock(g_atom* atom_1, g_atom* atom_2, bool set_on_finish, bool is_try, g_bool has_timeout, uint64_t timeout) {

	g_syscall_atomic_lock data;
	data.atom_1 = atom_1;
	data.atom_2 = atom_2;
	data.set_on_finish = set_on_finish;
	data.is_try = is_try;
	data.has_timeout = has_timeout;
	data.timeout = timeout;

	data.was_set = false;
	data.timed_out = false;

	g_syscall(G_SYSCALL_ATOMIC_LOCK, (uint32_t) &data);

	if (is_try) {
		return data.was_set;
	}
	return data.timed_out;
}
