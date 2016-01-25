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

#include <tasking/scheduling/scheduler.hpp>
#include <tasking/wait/waiter_sleep.hpp>
#include <system/interrupts/lapic.hpp>
#include <logger/logger.hpp>
#include <tasking/thread_manager.hpp>
#include <memory/address_space.hpp>
#include <memory/gdt/gdt_manager.hpp>
#include <utils/string.hpp>
#include <kernel.hpp>
#include "debug/debug_interface_kernel.hpp"

/**
 *
 */
g_scheduler::g_scheduler(uint32_t coreId) :
		milliseconds(0), task_list(0), current_task(0), coreId(coreId) {
}

/**
 *
 */
void g_scheduler::add(g_thread* t) {

	model_lock.lock();

	g_list_entry<g_thread*>* entry = new g_list_entry<g_thread*>;
	entry->value = t;
	entry->next = task_list;
	task_list = entry;
	t->scheduler = this;
	g_log_debug("%! task %i assigned to core %i", "scheduler", t->id, coreId);

	model_lock.unlock();
}

/**
 *
 */
bool g_scheduler::killAllThreadsOf(g_process* process) {

	bool still_has_living_threads = false;
	model_lock.lock();

	// kill all threads
	auto entry = task_list;
	while (entry) {
		g_thread* thr = entry->value;

		if (thr->process->main->id == process->main->id && thr->alive) {
			thr->alive = false;
			still_has_living_threads = true;
		}

		entry = entry->next;
	}

	g_log_debug("%! waiting for all threads of process %i to exit: %s", "scheduler", current_task->value->id,
			(still_has_living_threads ? "all finished" : "still waiting"));

	model_lock.unlock();
	return still_has_living_threads;
}

/**
 *
 */
g_thread* g_scheduler::getCurrent() {
	return current_task->value;
}

/**
 *
 */
uint32_t g_scheduler::calculateLoad() {

	uint32_t load = 0;

	// TODO improve load calculation
	g_list_entry<g_thread*> *entry = task_list;
	while (entry) {
		++load;
		entry = entry->next;
	}

	return load;
}

/**
 *
 */
void g_scheduler::updateMilliseconds() {
	milliseconds += APIC_MILLISECONDS_PER_TICK;

#if G_DEBUG_THREAD_DUMPING
	// debug dump
	static uint64_t last_debugout_millis = 0;
	if (milliseconds - last_debugout_millis > 10000) {
		last_debugout_millis = milliseconds;

		g_list_entry<g_thread*> *entry = task_list;
		g_log_info("%! core %i thread list:", "scheduler", this->coreId);
		while (entry) {
			g_thread* thr = entry->value;
			g_log_info("%# - %i:%i, eip: %h, waiter: %s, name: %s, rounds: %i", thr->process->main->id, thr->id, thr->cpuState->eip,
					(thr->waitManager == nullptr ? "-" : thr->waitManager->debug_name()), (thr->getIdentifier() == 0 ? "-" : thr->getIdentifier()),
					thr->rounds);
			entry = entry->next;
		}
	}
#endif

#if G_DEBUG_INTERFACE_MODE == G_DEBUG_INTERFACE_MODE_FULL
	static uint64_t lastProcessorTimeUpdate = 0;
	if (milliseconds - lastProcessorTimeUpdate > 500) {
		lastProcessorTimeUpdate = milliseconds;

		g_list_entry<g_thread*> *entry = task_list;
		while (entry) {
			g_thread* thr = entry->value;
			G_DEBUG_INTERFACE_TASK_SET_ROUNDS(thr->id, thr->rounds);
			thr->rounds = 0;
			entry = entry->next;
		}
	}
#endif
}

/**
 *
 */
uint64_t g_scheduler::getMilliseconds() {
	return milliseconds;
}

/**
 *
 */
g_processor_state* g_scheduler::schedule(g_processor_state* cpuState) {

	model_lock.lock();

	if (current_task) {
		current_task->value->cpuState = cpuState;
	}

	do {
		selectNextTask();
	} while (!applyTaskSwitch());
	++current_task->value->rounds;

	model_lock.unlock();

	return current_task->value->cpuState;
}

/**
 *
 */
void g_scheduler::selectNextTask() {

	if (current_task == 0) {
		current_task = task_list;

	} else {
		current_task = current_task->next;
		if (current_task == 0) {
			current_task = task_list;
		}
	}

	// If none could be selected, this is a fatal error
	if (current_task == 0) {
		g_kernel::panic("%! core %i has nothing to do", "scheduler", coreId);
	}
}

/**
 *
 */
bool g_scheduler::applyTaskSwitch() {

	// switch to it's space
	g_address_space::switch_to_space(current_task->value->process->pageDirectory);
	g_gdt_manager::setTssEsp0(current_task->value->kernelStackEsp0);

	// Eliminate if dead
	if (!current_task->value->alive) {
		// Threads can be deleted immediately. For processes, all associated threads must be deleted first.
		// This checks if the task is either no main thread, or all associated threads are already dead.
		if ((current_task->value->type != g_thread_type::THREAD_MAIN) || g_tasking::killAllThreadsOf(current_task->value->process)) {
			deleteCurrent();
		}

		return false;
	}

	// skip idler if possible
	if (current_task->value->priority == g_thread_priority::IDLE) {

		// check if any other process is available (not idling or waiting)
		g_list_entry<g_thread*> *n = task_list;
		while (n) {
			if (n->value->priority != g_thread_priority::IDLE && n->value->alive && n->value->waitManager == nullptr) {
				// skip the idler
				return false;
			}

			n = n->next;
		}
	}

	// Waiting must be done after the switch as it accesses user-space data.
	if (checkWaitingState()) {
		return false;
	}

	// Set segments for user thread, set segment to user segment
	g_gdt_manager::setUserThreadAddress(current_task->value->user_thread_addr);
	current_task->value->cpuState->gs = 0x30; // User pointer segment

	// Switch successful
	return true;
}

/**
 *
 */
void g_scheduler::deleteCurrent() {

	g_list_entry<g_thread*> *oldEntry = current_task;

	// Remove it from the task list
	if (task_list == oldEntry) {
		task_list = oldEntry->next;

	} else {

		g_list_entry<g_thread*> *entry = task_list;
		do {
			if (entry->next == oldEntry) {
				entry->next = oldEntry->next;
			}
		} while ((entry = entry->next) != 0);
	}

	current_task = oldEntry->next;

	// Delete the task
	g_thread_manager::deleteTask(oldEntry->value);
	delete oldEntry;
}

/**
 * 
 */
void g_scheduler::printWaiterDeadlockWarning() {

	char* taskName = (char*) "?";
	if (current_task->value->getIdentifier() != 0) {
		taskName = (char*) current_task->value->getIdentifier();
	}

	g_log_debug("%! thread %i (process %i, named '%s') waits for '%s'", "deadlock-detector", current_task->value->id, current_task->value->process->main->id,
			taskName, current_task->value->waitManager->debug_name());
}

/**
 *
 */
bool g_scheduler::checkWaitingState() {
	g_thread* thread = current_task->value;

	// check if task must continue waiting
	if (thread->waitManager != nullptr) {
		if (thread->checkWaiting()) {

			// increase wait counter for deadlock warnings
			thread->waitCount++;
			if (thread->waitCount % 500000 == 0) {
				printWaiterDeadlockWarning();
			}
			return true;

		} else {
			// reset wait counter & remove wait handler
			thread->waitCount = 0;
			thread->unwait();
			return false;
		}
	}

	return false;
}

/**
 *
 */
g_thread* g_scheduler::getTaskById(g_tid id) {

	g_thread* thr = 0;
	model_lock.lock();

	g_list_entry<g_thread*>* entry = task_list;
	while (entry) {
		if (entry->value->alive && entry->value->id == id) {
			thr = entry->value;
			break;
		}

		entry = entry->next;
	}

	model_lock.unlock();
	return thr;
}

/**
 *
 */
g_thread* g_scheduler::getTaskByIdentifier(const char* identifier) {

	g_thread* thr = 0;
	model_lock.lock();

	g_list_entry<g_thread*>* entry = task_list;
	while (entry) {
		if (entry->value->alive) {
			const char* taskIdentifier = entry->value->getIdentifier();
			if (taskIdentifier != 0 && g_string::equals(taskIdentifier, identifier)) {
				thr = entry->value;
				break;
			}
		}
		entry = entry->next;
	}

	model_lock.unlock();
	return thr;
}
