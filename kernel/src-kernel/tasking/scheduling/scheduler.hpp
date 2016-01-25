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

/**
 * The scheduler is responsible for determining which task is the next one to
 * be scheduled and to apply the actual switching.
 */
class g_scheduler {
private:
	uint64_t milliseconds;

	g_global_recursive_lock model_lock;
	g_list_entry<g_thread*>* task_list;
	g_list_entry<g_thread*>* current_task;

	uint32_t coreId;

	void selectNextTask();
	bool applyTaskSwitch();

	bool checkWaitingState();
	void deleteCurrent();

public:
	g_scheduler(uint32_t coreId);

	g_processor_state* schedule(g_processor_state* cpuState);
	void add(g_thread* t);

	uint32_t calculateLoad();

	g_thread* getCurrent();
	g_thread* getTaskById(g_tid id);
	g_thread* getTaskByIdentifier(const char* identifier);

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
