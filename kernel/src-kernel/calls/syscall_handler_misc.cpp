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
#include <ghost/kernquery.h>
#include <system/pci/pci.hpp>

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

/**
 * Kernquery handler
 */
G_SYSCALL_HANDLER(kernquery) {

	g_syscall_kernquery* data = (g_syscall_kernquery*) G_SYSCALL_DATA(current_thread->cpuState);

	// get the short query command
	uint16_t queryCommand = data->command;

	if (queryCommand == G_KERNQUERY_PCI_COUNT) {
		data->status = G_KERNQUERY_STATUS_SUCCESSFUL;
		g_kernquery_pci_count_data* buf = (g_kernquery_pci_count_data*) data->buffer;
		buf->count = g_pci::getDeviceCount();

	} else if (queryCommand == G_KERNQUERY_PCI_GET) {
		data->status = G_KERNQUERY_STATUS_SUCCESSFUL;

		g_kernquery_pci_get_data* buf = (g_kernquery_pci_get_data*) data->buffer;
		g_pci_header* pciHeader = g_pci::getDeviceAt(buf->position);

		if (pciHeader == nullptr) {
			buf->found = false;
		} else {
			buf->found = true;

			buf->bus = pciHeader->bus;
			buf->slot = pciHeader->slot;
			buf->function = pciHeader->function;

			buf->vendorId = pciHeader->vendorId;
			buf->deviceId = pciHeader->deviceId;

			buf->classCode = pciHeader->classCode;
			buf->subclassCode = pciHeader->subclassCode;
			buf->progIf = pciHeader->progIf;
		}

	} else if (queryCommand == G_KERNQUERY_TASK_COUNT) {
		data->status = G_KERNQUERY_STATUS_SUCCESSFUL;
		g_kernquery_task_count_data* buf = (g_kernquery_task_count_data*) data->buffer;
		buf->count = g_tasking::count();

	} else if (queryCommand == G_KERNQUERY_TASK_LIST) {
		data->status = G_KERNQUERY_STATUS_SUCCESSFUL;
		g_kernquery_task_list_data* buf = (g_kernquery_task_list_data*) data->buffer;
		buf->filled_ids = g_tasking::get_task_ids(buf->id_buffer, buf->id_buffer_size);

	} else if (queryCommand == G_KERNQUERY_TASK_GET_BY_ID) {
		data->status = G_KERNQUERY_STATUS_SUCCESSFUL;

		g_kernquery_task_get_data* buf = (g_kernquery_task_get_data*) data->buffer;
		g_thread* thread = g_tasking::getTaskById(buf->id);

		if (thread) {
			buf->id = thread->id;
			if (thread->process->main == nullptr) {
				buf->parent = -1;
			} else {
				buf->parent = thread->process->main->id;
			}
			buf->type = thread->type;
			buf->memory_used = g_thread_manager::getMemoryUsage(thread);

			// copy identifier
			const char* thread_ident = thread->getIdentifier();
			if (thread_ident == nullptr) {
				buf->identifier[0] = 0;
			} else {
				// do safe copy
				int max = 512;
				for (int i = 0; i < max; i++) {
					buf->identifier[i] = thread_ident[i];
					if (buf->identifier[i] == 0) {
						break;
					}
					if (i == max - 1) {
						buf->identifier[i] = 0;
						break;
					}
				}
			}

			// copy process source
			char* source_path = thread->process->source_path;
			if (source_path == nullptr) {
				buf->source_path[0] = 0;
			} else {
				g_string::copy(buf->source_path, source_path);
			}
		} else {
			data->status = G_KERNQUERY_STATUS_UNKNOWN_ID;
		}

	} else {
		data->status = G_KERNQUERY_STATUS_UNKNOWN_ID;
	}

	return current_thread;
}

