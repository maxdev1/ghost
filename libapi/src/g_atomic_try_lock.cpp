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
#include "__internal.h"

/**
 *
 */
g_bool g_atomic_try_lock(g_atom* atom) {
	return __g_atomic_lock(atom, nullptr, true, true, false, 0);
}

/**
 *
 */
g_bool g_atomic_try_lock_dual(g_atom* atom_1, g_atom* atom_2) {
	return __g_atomic_lock(atom_1, atom_2, true, true, false, 0);
}

/**
 *
 */
g_bool g_atomic_try_lock_to(g_atom* atom, uint64_t timeout) {
	return __g_atomic_lock(atom, nullptr, true, true, true, timeout);
}

/**
 *
 */
g_bool g_atomic_try_lock_dual_to(g_atom* atom_1, g_atom* atom_2, uint64_t timeout) {
	return __g_atomic_lock(atom_1, atom_2, true, true, true, timeout);
}
