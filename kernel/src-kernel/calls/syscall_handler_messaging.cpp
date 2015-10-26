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
#include <tasking/wait/waiter_recv_msg.hpp>
#include <tasking/wait/waiter_recv_topic_msg.hpp>
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
			g_tasking::pushInWait(receiver);
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

/**
 * Sends a message to the task with the "taskId", reading the contents from
 * the "message". This adds the message to the tasks incoming message queue.
 */
G_SYSCALL_HANDLER(send_msg) {

	g_syscall_send_msg* data = (g_syscall_send_msg*) G_SYSCALL_DATA(current_thread->cpuState);

	// Get executing task id and store it in the data
	data->message->sender = current_thread->id;

	// Send the message
	data->sendResult = g_message_controller::send(data->taskId, data->message);

	return current_thread;
}

/**
 * Receives the next message in the incoming message queue of the task with
 * the "taskId". This only works if the task with the given id is a thread of
 * the current process.
 */
G_SYSCALL_HANDLER(recv_msg) {

	g_process* process = current_thread->process;
	g_syscall_recv_msg* data = (g_syscall_recv_msg*) G_SYSCALL_DATA(current_thread->cpuState);

	uint32_t requestedTaskId = data->taskId;

	// Check if this task is allowed to receive the message
	if (requestedTaskId == current_thread->id || requestedTaskId == process->main->id) {

		g_message message;
		int status = g_message_controller::receive(requestedTaskId, message);

		if (status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
			// There was a message - immediate return
			*data->message = message;
			data->receiveResult = G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
			return current_thread;

		} else if (status == G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY) {

			// check mode to see what to do
			if (data->mode == G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) {
				data->receiveResult = G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
				return current_thread;

			} else {
				// append wait info and switch
				current_thread->wait(new g_waiter_recv_msg(data));
				return g_tasking::schedule();
			}

		} else {
			// Something went wrong, immediate return
			data->receiveResult = G_MESSAGE_RECEIVE_STATUS_FAILED;
			return current_thread;
		}
	} else {
		// Not permitted
		data->receiveResult = G_MESSAGE_RECEIVE_STATUS_FAILED_NOT_PERMITTED;
		return current_thread;
	}
}

/**
 * Receives the first message in the incoming message queue of the task with
 * the "taskId" that has the "topic". Same rules as for normal message
 * receiving apply.
 */
G_SYSCALL_HANDLER(recv_topic_msg) {

	g_process* process = current_thread->process;
	g_syscall_recv_topic_msg* data = (g_syscall_recv_topic_msg*) G_SYSCALL_DATA(current_thread->cpuState);

	uint32_t requestedTaskId = data->taskId;

	// Check if this task is allowed to receive the message
	if (requestedTaskId == current_thread->id || requestedTaskId == process->main->id) {

		g_message message;
		int status = g_message_controller::receiveWithTopic(requestedTaskId, data->topic, message);

		if (status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
			// There was a message - immediate return
			*data->message = message;
			data->receiveResult = G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
			return current_thread;

		} else if (status == G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY) {

			// check mode to see what to do
			if (data->mode == G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) {
				data->receiveResult = G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
				return current_thread;

			} else {
				// append wait info and switch
				current_thread->wait(new g_waiter_recv_topic_msg(data));
				return g_tasking::schedule();
			}

		} else {
			// Something went wrong, immediate return
			data->receiveResult = G_MESSAGE_RECEIVE_STATUS_FAILED;
			return current_thread;
		}
	} else {
		// Not permitted
		data->receiveResult = G_MESSAGE_RECEIVE_STATUS_FAILED_NOT_PERMITTED;
		return current_thread;
	}
}

