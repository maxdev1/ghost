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

#ifndef __LIBAPI_INTERNAL__
#define __LIBAPI_INTERNAL__

#include "ghost.h"

__BEGIN_C

/**
 * The address of this function is inserted as the return address for signal & irq handlers.
 * It does nothing but calling the <g_restore_interrupted_state> function.
 */
void __g_restore_interrupted_state_callback();

/**
 * Util functions
 */
void* __g_memcpy(void* dest, const void* src, size_t num);
size_t __g_strlen(const char* s);

/**
 * Simplified function used in the atomic wait API functions.
 */
g_bool __g_atomic_lock(g_atom* atom_1, g_atom* atom_2, bool set_on_finish, bool is_try, g_bool has_timeout, uint64_t timeout);

__END_C

#endif
