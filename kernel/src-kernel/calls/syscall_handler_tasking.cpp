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

#include <calls/syscall_handler.hpp>

#include <kernel.hpp>
#include <logger/logger.hpp>
#include <tasking/tasking.hpp>
#include <filesystem/filesystem.hpp>
#include <tasking/thread_manager.hpp>
#include <tasking/wait/waiter_wait_for_irq.hpp>
#include <tasking/wait/waiter_atomic_wait.hpp>
#include <tasking/wait/waiter_join.hpp>
#include <system/interrupts/handling/interrupt_request_handler.hpp>

/**
 * Yields
 */
G_SYSCALL_HANDLER(yield) {
	return g_tasking::switchTask(state);
}

/**
 * Sets the status of the current task to dead and yields. On the next call
 * to the scheduler this process is removed completely.
 */
G_SYSCALL_HANDLER(exit) {

	g_thread* task = g_tasking::getCurrentThread();
	task->alive = false;
	g_log_debug("%! task %i exited faithfully with code %i", "syscall", task->id, ((g_syscall_exit*) G_SYSCALL_DATA(state))->code);

	return g_tasking::switchTask(state);
}

/**
 * Kills a process
 */
G_SYSCALL_HANDLER(kill) {

	g_syscall_kill* data = (g_syscall_kill*) G_SYSCALL_DATA(state);
	g_thread* thr = g_tasking::getTaskById(data->pid);
	thr->process->main->alive = false;

	// switch, might be suicide
	return g_tasking::switchTask(state);
}

/**
 * Lets the current process sleep for the specified number of milliseconds. If the caller
 * specifies a negative number, the process doesn't sleep and returns normally.
 */
G_SYSCALL_HANDLER(sleep) {

	g_thread* process = g_tasking::getCurrentThread();
	g_syscall_sleep* data = (g_syscall_sleep*) G_SYSCALL_DATA(state);

	if (data->milliseconds > 0) {
		g_tasking::getCurrentScheduler()->sleep(process, data->milliseconds);
	}

	return g_tasking::switchTask(state);

}

/**
 * Returns the id of the current process. If the executing task is a thread, the id of the
 * root process is returned.
 */
G_SYSCALL_HANDLER(get_pid) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_get_pid* data = (g_syscall_get_pid*) G_SYSCALL_DATA(state);

	data->id = task->process->main->id;

	return state;

}

/**
 * Returns the id of the current task.
 */
G_SYSCALL_HANDLER(get_tid) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_get_pid* data = (g_syscall_get_pid*) G_SYSCALL_DATA(state);

	data->id = task->id;

	return state;

}

/**
 * Returns the process id for a task id.
 */
G_SYSCALL_HANDLER(get_pid_for_tid) {

	g_syscall_get_pid_for_tid* data = (g_syscall_get_pid_for_tid*) G_SYSCALL_DATA(state);

	g_thread* task = g_tasking::getTaskById(data->tid);
	data->pid = task->process->main->id;

	return state;

}

/**
 * The interrupt polling mechanism allows programs to wait until an interrupt is
 * fired. If the interrupt was already fired, this call immediately returns. Otherwise,
 * the task waits until the interrupt happens.
 */
G_SYSCALL_HANDLER(wait_for_irq) {

	g_thread* thread = g_tasking::getCurrentThread();
	g_process* process = thread->process;
	g_syscall_wait_for_irq* data = (g_syscall_wait_for_irq*) G_SYSCALL_DATA(state);

	// Only driver level
	if (process->securityLevel == G_SECURITY_LEVEL_DRIVER) {
		bool fired = g_interrupt_request_handler::pollIrq(data->irq);

		if (fired) {
			return state;

		} else {
			g_thread* task = g_tasking::getCurrentThread();
			task->wait(new g_waiter_wait_for_irq(data->irq));
			return g_tasking::switchTask(state);
		}
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(atomic_wait) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_atomic_lock* data = (g_syscall_atomic_lock*) G_SYSCALL_DATA(state);

	bool* atom = (bool*) data->atom;
	bool set_on_finish = data->set_on_finish;
	bool try_only = data->try_only;

	if (try_only) {
		if (*atom) {
			data->was_set = false;
		} else {
			*atom = true;
			data->was_set = true;
		}
		return state;
	}

	if (*atom) {
		task->wait(new g_waiter_atomic_wait(atom, set_on_finish));
		return g_tasking::switchTask(state);
	} else {
		if (set_on_finish) {
			*atom = true;
		}
		return state;
	}
}

/**
 *
 */
G_SYSCALL_HANDLER(task_id_register) {

	g_thread* task = g_tasking::getCurrentThread();
	g_task_id_register* data = (g_task_id_register*) G_SYSCALL_DATA(state);

	data->successful = g_tasking::registerTaskForIdentifier(task, data->identifier);

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(task_id_get) {

	g_syscall_task_id_get* data = (g_syscall_task_id_get*) G_SYSCALL_DATA(state);

	data->resultTaskId = -1;

	g_thread* resultTask = g_tasking::getTaskByIdentifier((const char*) data->identifier);
	if (resultTask != 0) {
		data->resultTaskId = resultTask->id;
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(millis) {

	g_syscall_millis* data = (g_syscall_millis*) G_SYSCALL_DATA(state);

	data->millis = g_tasking::getCurrentScheduler()->getMilliseconds();

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(fork) {

	g_syscall_fork* data = (g_syscall_fork*) G_SYSCALL_DATA(state);
	g_thread* current = g_tasking::getCurrentThread();
	g_thread* forked = g_tasking::fork();
	if (forked) {
		// clone file descriptors
		g_filesystem::process_forked(current->process->main->id, forked->process->main->id);

		// return forked id in target process
		data->forkedId = forked->id;

		// switch to clone for a moment, set return value to 0
		auto cur = g_address_space::get_current_space();
		g_address_space::switch_to_space(forked->process->pageDirectory);
		data->forkedId = 0;
		g_address_space::switch_to_space(cur);
	} else {
		data->forkedId = -1;
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(join) {

	g_syscall_join* data = (g_syscall_join*) G_SYSCALL_DATA(state);

	g_thread* current = g_tasking::getCurrentThread();
	current->wait(new g_waiter_join(data->taskId));

	return g_tasking::switchTask(state);
}

/**
 *
 */
G_SYSCALL_HANDLER(register_irq_handler) {

	g_syscall_register_irq_handler* data = (g_syscall_register_irq_handler*) G_SYSCALL_DATA(state);
	g_thread* task = g_tasking::getCurrentThread();

	if (task->process->securityLevel <= G_SECURITY_LEVEL_DRIVER) {
		g_interrupt_request_handler::set_handler(data->irq, task->id, data->handler, data->callback);
		data->status = G_REGISTER_IRQ_HANDLER_STATUS_SUCCESSFUL;
	} else {
		data->status = G_REGISTER_IRQ_HANDLER_STATUS_NOT_PERMITTED;
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(restore_interrupted_state) {

	g_thread* thread = g_tasking::getCurrentThread();
	thread->restore_interrupted_state();
	return g_tasking::switchTask(state);
}

/**
 *
 */
G_SYSCALL_HANDLER(register_signal_handler) {

	g_syscall_register_signal_handler* data = (g_syscall_register_signal_handler*) G_SYSCALL_DATA(state);
	g_thread* task = g_tasking::getCurrentThread();

	if (data->signal >= 0 && data->signal < SIG_COUNT) {
		g_signal_handler* handler = &(task->process->signal_handlers[data->signal]);

		data->previous_handler = handler->handler;

		handler->handler = data->handler;
		handler->callback = data->callback;
		handler->thread_id = task->id;

		data->status = G_REGISTER_SIGNAL_HANDLER_STATUS_SUCCESSFUL;
		g_log_debug("%! signal handler %h registered for signal %i", "syscall", data->handler, data->signal);

	} else {
		data->previous_handler = -1;
		data->status = G_REGISTER_SIGNAL_HANDLER_STATUS_INVALID_SIGNAL;
		g_log_debug("%! failed to register signal handler %h for invalid signal %i", "syscall", data->handler, data->signal);
	}

	return state;
}
/**
 *
 */
G_SYSCALL_HANDLER(raise_signal) {

	g_syscall_raise_signal* data = (g_syscall_raise_signal*) G_SYSCALL_DATA(state);
	g_thread* thread = g_tasking::getCurrentThread();

	if (data->signal >= 0 && data->signal < SIG_COUNT) {

		// get main thread by id
		g_thread* target_thread = 0;

		if (thread->id == data->process) {
			target_thread = thread;
		} else if (thread->process->main->id == data->process) {
			target_thread = thread->process->main;
		} else {
			target_thread = g_tasking::getTaskById(data->process);
		}

		if (target_thread == 0) {
			// target process doesn't exist
			data->status = G_RAISE_SIGNAL_STATUS_INVALID_TARGET;

		} else {
			// raise the signal
			target_thread->raise_signal(data->signal);
			data->status = G_RAISE_SIGNAL_STATUS_SUCCESSFUL;
		}

	} else {
		// signal doesn't exist
		data->status = G_RAISE_SIGNAL_STATUS_INVALID_SIGNAL;
	}

	return g_tasking::switchTask(state);
}

