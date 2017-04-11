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

#ifndef GHOST_TASKING_TASKING
#define GHOST_TASKING_TASKING

#include <system/processor_state.hpp>

#include "ghost/kernel.h"
#include "ghost/stdint.h"
#include <tasking/thread.hpp>
#include <tasking/process.hpp>
#include <tasking/scheduling/scheduler.hpp>

/**
 *
 */
class g_tasking {
public:

	/**
	 * Called once by the BSP
	 */
	static void initialize();

	/**
	 * Called by each core (including BSP)
	 */
	static void enableForThisCore();

	/**
	 * Saves the current CPU state and returns the current thread.
	 */
	static g_thread* save(g_processor_state* cpuState);

	/**
	 * Called to switch tasks. Function returns the task to execute next.
	 */
	static g_thread* schedule();

	/**
	 * Adds the task to the least loaded cores scheduler
	 */
	static void addTask(g_thread* proc, bool enforceCurrentCore = false);

	/**
	 * Pushes the given thread to the top of the wait queue.
	 */
	static void increaseWaitPriority(g_thread* proc);

	/**
	 * Returns the current task on the current core
	 */
	static g_thread* lastThread();

	/**
	 * Kills all threads of the given process within all schedulers.
	 * Returns true if all are dead.
	 *
	 * @param process
	 * 		the process of which the threads shall be killed
	 */
	static void remove_threads(g_process* process);

	/**
	 * Returns the current scheduler on the current core
	 */
	static g_scheduler* currentScheduler();

	/**
	 *
	 */
	static g_thread* getTaskById(uint32_t id);

	/**
	 *
	 */
	static g_thread* getTaskByIdentifier(const char* identifier);

	/**
	 *
	 */
	static bool registerTaskForIdentifier(g_thread* task, const char* newIdentifier);

	/**
	 *
	 */
	static g_thread* fork(g_thread* current_thread);

	/**
	 * Counts the number of tasks in all schedulers.
	 */
	static uint32_t count();

	/**
	 *
	 */
	static uint32_t get_task_ids(g_tid* out, uint32_t len);

};

#endif
