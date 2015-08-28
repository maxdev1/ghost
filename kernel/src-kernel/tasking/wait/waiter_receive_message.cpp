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

#include "tasking/wait/waiter_receive_message.hpp"
#include "tasking/communication/message_controller.hpp"
#include "logger/logger.hpp"

/**
 *
 */
bool g_waiter_receive_message::checkWaiting(g_thread* task) {

	// if break is issued, stop sleeping
	if (data->break_condition != nullptr && *data->break_condition) {
		data->status = G_MESSAGE_RECEIVE_STATUS_INTERRUPTED;
		return false;
	}

	data->status = g_message_controller::receive_message(task->id, data->buffer, data->maximum, data->transaction);

	// if queue is empty, continue sleeping
	if (data->status == G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY) {
		return true;
	}

	// stop sleeping
	return false;
}

