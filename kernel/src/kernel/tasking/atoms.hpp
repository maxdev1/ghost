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
	g_atom_waiter* waiters;
};

/**
 * Initializes the atoms.
 */
void atomicInitialize();

/**
 * Creates a new atom that can then be locked with the other functions.
 */
g_atom atomicCreate();

/**
 * Attempts to lock the atom and returns whether locking was successful. If it is not a try
 * and the lock is already set, the task is put to sleep.
 */
bool atomicLock(g_task* task, g_atom atom, bool isTry, bool setOnFinish);

/**
 * Unlocks the atom and wakes the next waiting task.
 */
void atomicUnlock(g_atom atom);

/**
 * Removes the task from a wait queue, for example in case of timeouts.
 */
void atomicRemoveFromWaiters(g_atom atom, g_tid task);

#endif
