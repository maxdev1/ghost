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

#include <tasking/communication/message_controller.hpp>
#include <logger/logger.hpp>

static g_message_queue* firstQueue = 0;

/**
 *
 */
g_message_send_status g_message_controller::send(uint32_t task, g_message& message) {

	g_message_queue* foundQueue = 0;

	// Search for queue
	if (firstQueue != 0) {
		g_message_queue* n = firstQueue;
		do {
			if (n->taskId == task) {
				foundQueue = n;
				break;
			}
		} while ((n = n->next) != 0);
	}

	// Create a new queue
	if (foundQueue == 0) {
		foundQueue = new g_message_queue;
		foundQueue->taskId = task;
		foundQueue->count = 0;
		foundQueue->next = firstQueue;
		firstQueue = foundQueue;
	}

	// Add to queue
	if (foundQueue->count < G_MESSAGE_QUEUE_SIZE) {
		foundQueue->messages[foundQueue->count++] = message;
		return G_MESSAGE_SEND_STATUS_SUCCESSFUL;
	}

	return G_MESSAGE_SEND_STATUS_QUEUE_FULL;
}

/**
 *
 */
g_message_receive_status g_message_controller::receive(uint32_t task, g_message& target) {

	g_message_queue* foundQueue = 0;

	// Search for queue
	if (firstQueue != 0) {
		g_message_queue* n = firstQueue;
		do {
			if (n->taskId == task) {
				foundQueue = n;
				break;
			}
		} while ((n = n->next) != 0);
	}

	// Take from queue
	if (foundQueue != 0 && foundQueue->count > 0) {
		target = foundQueue->messages[--foundQueue->count];

		return G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
	}

	return G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
}

/**
 *
 */
g_message_receive_status g_message_controller::receiveWithTopic(uint32_t task, uint32_t topic, g_message& target) {

	g_message_queue* foundQueue = 0;

	// Search for queue
	if (firstQueue != 0) {
		g_message_queue* n = firstQueue;
		do {
			if (n->taskId == task) {
				foundQueue = n;
				break;
			}
		} while ((n = n->next) != 0);
	}

	// Take from queue
	if (foundQueue != 0 && foundQueue->count > 0) {

		int32_t indexToRemove = -1;

		for (int32_t i = 0; i < foundQueue->count; i++) {
			if (foundQueue->messages[i].topic == topic) {
				target = foundQueue->messages[i];
				indexToRemove = i;
				break;
			}
		}

		if (indexToRemove != -1) {
			// copy all objects behind the removed position one to the left
			for (int32_t i = indexToRemove; i < foundQueue->count - 1; i++) {
				foundQueue->messages[i] = foundQueue->messages[i + 1];
			}

			// decrease number of queued messages
			--foundQueue->count;

			return G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
		}
	}

	return G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
}

