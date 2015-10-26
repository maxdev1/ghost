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

#ifndef SCHEDULER_HPP_
#define SCHEDULER_HPP_

#include <system/processor_state.hpp>

#include "ghost/stdint.h"
#include <utils/list_entry.hpp>
#include <tasking/thread.hpp>
#include <system/smp/global_recursive_lock.hpp>

typedef g_list_entry<g_thread*> g_task_entry;

/**
 * The scheduler is responsible for determining which task is the next one to
 * be scheduled and to apply the actual switching.
 */
class g_scheduler {
private:
	uint64_t milliseconds;
	uint32_t coreId;

	g_global_recursive_lock model_lock;

	g_task_entry* wait_queue;
	g_task_entry* run_queue;
	g_task_entry* idle_entry;
	g_task_entry* current_entry;

	void selectNextTask();
	bool applyTaskSwitch();

	void checkWaitingState(g_thread* thread);
	void deleteCurrent();

public:
	g_scheduler(uint32_t coreId);

	g_thread* save(g_processor_state* cpuState);
	g_thread* schedule();
	void add(g_thread* t);

	void pushInWait(g_thread* thread);

	uint32_t calculateLoad();

	g_thread* getCurrent();
	g_thread* getTaskById(g_tid id);
	g_thread* getTaskByIdentifier(const char* identifier);

	void switchSpace(g_thread* thread);
	bool eliminateIfDead(g_thread* thread);
	void finishSwitch(g_thread* thread);

	void moveToRunQueue(g_thread* thread);
	void moveToWaitQueue(g_thread* thread);
	g_task_entry* removeFromQueue(g_task_entry** queue_head, g_thread* thread);

	/**
	 * Sets all threads of the given process to !alive and returns
	 * whether there where any threads that were still alive.
	 */
	bool killAllThreadsOf(g_process* process);

	void updateMilliseconds();
	uint64_t getMilliseconds();

	void printWaiterDeadlockWarning();
};

#endif
