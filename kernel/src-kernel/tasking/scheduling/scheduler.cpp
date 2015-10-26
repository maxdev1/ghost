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
		milliseconds(0), taskList(0), current(0), coreId(coreId) {
}

/**
 *
 */
void g_scheduler::lock() {
	taskListLock.lock();
}

/**
 *
 */
void g_scheduler::unlock() {
	taskListLock.unlock();
}

/**
 *
 */
g_cpu_state* g_scheduler::switchTask(g_cpu_state* cpuState) {

	lock();

	if (current) {
		current->value->cpuState = cpuState;
	}

	do {
		selectNext();
	} while (!applySwitch());
	++current->value->rounds;

	unlock();

	return current->value->cpuState;
}

/**
 *
 */
void g_scheduler::add(g_thread* t) {

	lock();

	g_list_entry<g_thread*>* entry = new g_list_entry<g_thread*>;
	entry->value = t;
	entry->next = taskList;
	taskList = entry;
	g_log_debug("%! task %i assigned to core %i", "scheduler", t->id, coreId);

	unlock();
}

/**
 *
 */
bool g_scheduler::killAllThreadsOf(g_process* process) {

	bool still_has_living_threads = false;
	lock();

	// kill all threads
	auto entry = taskList;
	while (entry) {
		g_thread* thr = entry->value;

		if (thr->process->main->id == process->main->id && thr->alive) {
			thr->alive = false;
			still_has_living_threads = true;
		}

		entry = entry->next;
	}

	g_log_debug("%! waiting for all threads of process %i to exit: %s", "scheduler", current->value->id,
			(still_has_living_threads ? "all finished" : "still waiting"));

	unlock();
	return still_has_living_threads;
}

/**
 *
 */
g_thread* g_scheduler::getCurrent() {
	return current->value;
}

/**
 *
 */
uint32_t g_scheduler::getLoad() {

	uint32_t load = 0;

	// TODO improve load calculation
	g_list_entry<g_thread*> *entry = taskList;
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

		g_list_entry<g_thread*> *entry = taskList;
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

		g_list_entry<g_thread*> *entry = taskList;
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
void g_scheduler::sleep(g_thread* task, uint64_t sleepies) {
	if (!task->isWaiting()) {
		task->wait(new g_waiter_sleep(this, sleepies));
	}
}

/**
 *
 */
void g_scheduler::selectNext() {

	if (current == 0) {
		current = taskList;
	} else {
		current = current->next;
		if (current == 0) {
			current = taskList;
		}
	}

	// If none could be selected, this is a fatal error
	if (current == 0) {
		g_kernel::panic("%! core %i has nothing to do", "scheduler", coreId);
	}
}

/**
 *
 */
bool g_scheduler::applySwitch() {

	g_address_space::switch_to_space(current->value->process->pageDirectory);
	g_gdt_manager::setTssEsp0(current->value->kernelStackEsp0);

	// Eliminate if dead
	if (!current->value->alive) {

		// If the current task is not a process, we can immediately remove it.
		// Otherwise, we have to check if all the processes child threads are dead,
		// once this is done, the process is also kill.
		if (current->value->type != g_thread_type::THREAD_MAIN || g_tasking::killAllThreadsOf(current->value->process)) {
			deleteCurrent();
		}

		return false;
	}

	/*
	 TODO
	 Hier war das Problem, dass wenn ich einen IDLE-Prozess übersprungen hab während alle
	 anderen "waiting" oder nicht "alive" waren, alles komplett gefreggt ist. Allerdings
	 hab ich grad keinen Nerv mir das weiter anzuschauen, deswegen bleibt das hier ^-^
	 */

	// Skip idler if possible
	if (current->value->priority == g_thread_priority::IDLE) {

		// Check if any other process is available (not idling or waiting)
		g_list_entry<g_thread*> *n = taskList;
		while (n) {
			if (n->value->priority != g_thread_priority::IDLE && n->value->alive && !n->value->isWaiting()) {
				// skip the idler
				return false;
			}

			n = n->next;
		}
	}

	// Waiting must be done after the switch because it accesses userspace data
	bool keepWaiting = handleWaiting();
	if (keepWaiting) {
		return false;
	}

	// Set segments for user thread, set segment to user segment
	g_gdt_manager::setUserThreadAddress(current->value->user_thread_addr);
	current->value->cpuState->gs = 0x30; // User pointer segment

	// Switch successful
	return true;
}

/**
 *
 */
void g_scheduler::deleteCurrent() {

	g_list_entry<g_thread*> *oldEntry = current;

	// Remove it from the task list
	if (taskList == oldEntry) {
		taskList = oldEntry->next;

	} else {

		g_list_entry<g_thread*> *entry = taskList;
		do {
			if (entry->next == oldEntry) {
				entry->next = oldEntry->next;
			}
		} while ((entry = entry->next) != 0);
	}

	current = oldEntry->next;

	// Delete the task
	g_thread_manager::deleteTask(oldEntry->value);
	delete oldEntry;
}

/**
 * 
 */
void g_scheduler::print_waiter_deadlock_warning() {

	char* taskName = (char*) "?";
	if (current->value->getIdentifier() != 0) {
		taskName = (char*) current->value->getIdentifier();
	}

	g_log_debug("%! thread %i (process %i, named '%s') waits for '%s'", "deadlock-detector", current->value->id, current->value->process->main->id, taskName,
			current->value->waitManager->debug_name());
}

/**
 *
 */
bool g_scheduler::handleWaiting() {

	// check if the current task must wait
	if (current->value->isWaiting()) {

		// call the wait handler
		bool keepWaiting = current->value->checkWaiting();
		if (keepWaiting) {

			// increase wait counter for deadlock warnings
			current->value->waitCount++;
			if (current->value->waitCount % 500000 == 0) {
				print_waiter_deadlock_warning();
			}
			return true;
		} else {

			// reset wait counter & remove wait handler
			current->value->waitCount = 0;
			current->value->unwait();
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
	lock();

	g_list_entry<g_thread*>* entry = taskList;
	while (entry) {
		if (entry->value->alive && entry->value->id == id) {
			thr = entry->value;
			break;
		}

		entry = entry->next;
	}

	unlock();
	return thr;
}

/**
 *
 */
g_thread* g_scheduler::getTaskByIdentifier(const char* identifier) {

	g_thread* thr = 0;
	lock();

	g_list_entry<g_thread*>* entry = taskList;
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

	unlock();
	return thr;
}
