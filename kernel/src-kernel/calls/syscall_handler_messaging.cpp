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

#include <tasking/communication/message_controller.hpp>
#include <logger/logger.hpp>
#include <tasking/tasking.hpp>
#include <tasking/thread_manager.hpp>
#include <tasking/wait/waiter_send_message.hpp>
#include <tasking/wait/waiter_receive_message.hpp>

/**
 *
 */
G_SYSCALL_HANDLER(send_message) {

	g_syscall_send_message* data = (g_syscall_send_message*) G_SYSCALL_DATA(current_thread->cpuState);

	// send the message
	data->status = g_message_controller::send_message(data->receiver, current_thread->id, data->buffer, data->length, data->transaction);

	// move receiver to top of wait queue
	if (data->status == G_MESSAGE_SEND_STATUS_SUCCESSFUL) {
		g_thread* receiver = g_tasking::getTaskById(data->receiver);
		if (receiver) {
			g_tasking::increaseWaitPriority(receiver);
		}

		// check if block
	} else if (data->mode == G_MESSAGE_SEND_MODE_BLOCKING && data->status == G_MESSAGE_SEND_STATUS_QUEUE_FULL) {
		current_thread->wait(new g_waiter_send_message(data));
		return g_tasking::schedule();
	}

	return current_thread;
}

/**
 *
 */
G_SYSCALL_HANDLER(receive_message) {

	g_syscall_receive_message* data = (g_syscall_receive_message*) G_SYSCALL_DATA(current_thread->cpuState);

	data->status = g_message_controller::receive_message(current_thread->id, data->buffer, data->maximum, data->transaction);

	if (data->status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		// there was a message, immediate return
		return current_thread;
	}

	if (data->status == G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY) {
		// check mode to see what to do
		if (data->mode == G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) {
			return current_thread;
		}

		// perform blocking
		current_thread->wait(new g_waiter_receive_message(data));
		return g_tasking::schedule();
	}

	// something went wrong, immediate return
	return current_thread;
}

