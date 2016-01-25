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

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_send_message* data = (g_syscall_send_message*) G_SYSCALL_DATA(state);

	// send the message
	data->status = g_message_controller::send_message(data->receiver, task->id, data->buffer, data->length, data->transaction);

	// check if block
	if (data->mode == G_MESSAGE_SEND_MODE_BLOCKING && data->status == G_MESSAGE_SEND_STATUS_QUEUE_FULL) {
		task->wait(new g_waiter_send_message(data));
		return g_tasking::schedule(state);
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(receive_message) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_receive_message* data = (g_syscall_receive_message*) G_SYSCALL_DATA(state);

	data->status = g_message_controller::receive_message(task->id, data->buffer, data->maximum, data->transaction);

	if (data->status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		// there was a message, immediate return
		return state;
	}

	if (data->status == G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY) {
		// check mode to see what to do
		if (data->mode == G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) {
			return state;
		}

		// perform blocking
		task->wait(new g_waiter_receive_message(data));
		return g_tasking::schedule(state);
	}

	// something went wrong, immediate return
	return state;
}

/**
 * Sends a message to the task with the "taskId", reading the contents from
 * the "message". This adds the message to the tasks incoming message queue.
 */
G_SYSCALL_HANDLER(send_msg) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_send_msg* data = (g_syscall_send_msg*) G_SYSCALL_DATA(state);

	// Get executing task id and store it in the data
	data->message->sender = task->id;

	// Send the message
	data->sendResult = g_message_controller::send(data->taskId, data->message);

	return state;
}

/**
 * Receives the next message in the incoming message queue of the task with
 * the "taskId". This only works if the task with the given id is a thread of
 * the current process.
 */
G_SYSCALL_HANDLER(recv_msg) {

	g_thread* task = g_tasking::getCurrentThread();
	g_process* process = task->process;
	g_syscall_recv_msg* data = (g_syscall_recv_msg*) G_SYSCALL_DATA(state);

	uint32_t requestedTaskId = data->taskId;

	// Check if this task is allowed to receive the message
	if (requestedTaskId == task->id || requestedTaskId == process->main->id) {

		g_message message;
		int status = g_message_controller::receive(requestedTaskId, message);

		if (status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
			// There was a message - immediate return
			*data->message = message;
			data->receiveResult = G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
			return state;

		} else if (status == G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY) {

			// check mode to see what to do
			if (data->mode == G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) {
				data->receiveResult = G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
				return state;

			} else {
				// append wait info and switch
				g_thread* task = g_tasking::getCurrentThread();
				task->wait(new g_waiter_recv_msg(data));
				return g_tasking::schedule(state);
			}

		} else {
			// Something went wrong, immediate return
			data->receiveResult = G_MESSAGE_RECEIVE_STATUS_FAILED;
			return state;
		}
	} else {
		// Not permitted
		data->receiveResult = G_MESSAGE_RECEIVE_STATUS_FAILED_NOT_PERMITTED;
		return state;
	}
}

/**
 * Receives the first message in the incoming message queue of the task with
 * the "taskId" that has the "topic". Same rules as for normal message
 * receiving apply.
 */
G_SYSCALL_HANDLER(recv_topic_msg) {

	g_thread* task = g_tasking::getCurrentThread();
	g_process* process = task->process;
	g_syscall_recv_topic_msg* data = (g_syscall_recv_topic_msg*) G_SYSCALL_DATA(state);

	uint32_t requestedTaskId = data->taskId;

	// Check if this task is allowed to receive the message
	if (requestedTaskId == task->id || requestedTaskId == process->main->id) {

		g_message message;
		int status = g_message_controller::receiveWithTopic(requestedTaskId, data->topic, message);

		if (status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
			// There was a message - immediate return
			*data->message = message;
			data->receiveResult = G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
			return state;

		} else if (status == G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY) {

			// check mode to see what to do
			if (data->mode == G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) {
				data->receiveResult = G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
				return state;

			} else {
				// append wait info and switch
				g_thread* task = g_tasking::getCurrentThread();
				task->wait(new g_waiter_recv_topic_msg(data));
				return g_tasking::schedule(state);
			}

		} else {
			// Something went wrong, immediate return
			data->receiveResult = G_MESSAGE_RECEIVE_STATUS_FAILED;
			return state;
		}
	} else {
		// Not permitted
		data->receiveResult = G_MESSAGE_RECEIVE_STATUS_FAILED_NOT_PERMITTED;
		return state;
	}
}

