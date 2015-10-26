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

#include <logger/logger.hpp>
#include <tasking/thread_manager.hpp>
#include <tasking/tasking.hpp>
#include <utils/string.hpp>
#include <memory/physical/pp_allocator.hpp>
#include <system/system.hpp>

/**
 * Writes a message to the system log.
 */
G_SYSCALL_HANDLER(log) {
	g_process* process = current_thread->process;
	g_syscall_log* data = (g_syscall_log*) G_SYSCALL_DATA(current_thread->cpuState);

	// % signs are not permitted, because the internal logger would get confused.
	uint32_t len = g_string::length(data->message);
	for (uint32_t i = 0; i < len; i++) {
		if (data->message[i] == '%') {
			data->message[i] = '!';
		}
	}

	const char* task_ident = current_thread->getIdentifier();
	if (task_ident == 0) {
		task_ident = current_thread->process->main->getIdentifier();
	}

	// If the task has an identifier, do log with name:
	const char* prefix = "-";
	if (task_ident != 0) {
		int header_len = g_string::length(prefix);
		int ident_len = g_string::length(task_ident);
		char loggie[header_len + ident_len + 1];
		g_string::concat(prefix, task_ident, loggie);

		g_log_info("%! (%i:%i) %s", loggie, process->main->id, current_thread->id, data->message);
	} else {
		g_log_info("%! (%i:%i) %s", prefix, process->main->id, current_thread->id, data->message);
	}

	return current_thread;
}

/**
 * Sets the log output to the screen enabled or disabled.
 */
G_SYSCALL_HANDLER(set_video_log) {
	g_syscall_set_video_log* data = (g_syscall_set_video_log*) G_SYSCALL_DATA(current_thread->cpuState);

	g_logger::setVideo(data->enabled);

	return current_thread;
}

/**
 * Test call handler
 */
G_SYSCALL_HANDLER(test) {

	g_syscall_test* data = (g_syscall_test*) G_SYSCALL_DATA(current_thread->cpuState);

	if (data->test == 1) {
		data->result = g_pp_allocator::getFreePageCount();
		g_log_info("free pages: %i", data->result);
	} else {
		data->result = 0;
	}

	return current_thread;
}

