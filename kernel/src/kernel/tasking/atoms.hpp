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

#ifndef __KERNEL_ATOMS__
#define __KERNEL_ATOMS__

#include "ghost/types.h"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/tasking/tasking.hpp"

struct g_atom_waiter
{
	g_tid task;
	g_atom_waiter* next;
};

struct g_atom_entry
{
	g_mutex lock;
	int value;

    bool reentrant;
    g_tid owner;

	g_atom_waiter* waiters;
};

typedef uint32_t g_atom_lock_status;
#define G_ATOM_LOCK_STATUS_SET      ((g_atom_lock_status) 1)
#define G_ATOM_LOCK_STATUS_TIMEOUT  ((g_atom_lock_status) 2)
#define G_ATOM_LOCK_STATUS_NOT_SET  ((g_atom_lock_status) 3)

/**
 * Initializes the atoms.
 */
void atomicInitialize();

/**
 * Creates a new atom that can then be locked with the other functions.
 */
g_atom atomicCreate(bool reentrant);

/**
 * Destroys an atom.
 */
void atomicDestroy(g_atom atom);

/**
 *
 */
g_atom_lock_status atomicLock(g_task* task, g_atom atom, uint64_t timeout, bool trying);

/**
 *
 */
g_atom_lock_status atomicTryLock(g_task* task, g_atom atom);

/**
 * Unlocks the atom and wakes the next waiting task.
 */
void atomicUnlock(g_atom atom);

/**
 * Removes the task from a wait queue, for example in case of timeouts.
 */
void atomicUnwaitForLock(g_atom atom, g_tid task);

/**
 * Adds the task to the wait queue for the atom.
 */
void atomicWaitForLock(g_atom atom, g_tid task);

#endif
