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

#include <ghost.h>
#include <ghostuser/ui/listener.hpp>
#include <ghostuser/ui/ui.hpp>
#include <ghostuser/utils/logger.hpp>
#include <ghostuser/utils/value_placer.hpp>
#include <map>
#include <deque>

/**
 * Input and output pipe for the window server communication
 */
static g_fd g_ui_channel_in = -1;
static g_fd g_ui_channel_out = -1;

/**
 * Global ready indicator
 */
bool g_ui_ready = false;

/**
 * Next assignable transaction id
 */
static g_ui_transaction_id next_transaction = 1;

/**
 * Map of running transactions
 */
static std::map<g_ui_transaction_id, g_ui_transaction_data*>* transaction_map = 0;

/**
 * Map of listeners
 */
static std::map<uint32_t, g_listener*> listeners;

/**
 * Used by event dispatch thread
 */
static uint8_t event_dispatch_events_empty = true;
static uint8_t event_dispatch_locked = false;
static std::deque<g_ui_event_dispatch_data> event_dispatch_queue;

/**
 * Opens a connection to the window server.
 */
g_ui_open_status g_ui::open() {

	// check if already open
	if (g_ui_ready) {
		return G_UI_OPEN_STATUS_EXISTING;
	}

	// get window managers id
	g_tid window_mgr = g_task_get_id(G_WINDOW_MANAGER_IDENTIFIER);
	if (window_mgr == -1) {
		g_logger::log("failed to retrieve task id of window server");
		return G_UI_OPEN_STATUS_COMMUNICATION_FAILED;
	}

	// open in/out pipes
	g_fs_pipe_status status;

	g_fd g_ui_channel_out_read;
	g_pipe_s(&g_ui_channel_out, &g_ui_channel_out_read, &status);
	g_fd g_ui_channel_in_write;
	g_pipe_s(&g_ui_channel_in_write, &g_ui_channel_in, &status);

	if (status == G_FS_PIPE_ERROR) {
		g_logger::log("failed to open UI communication pipe");
		return G_UI_OPEN_STATUS_COMMUNICATION_FAILED;
	}

	// tell window manager to open
	uint32_t topic = g_ipc_next_topic();

	g_message_empty(open_request);
	open_request.type = G_UI_COMMAND_OPEN_REQUEST;
	open_request.topic = topic;
	open_request.parameterA = g_ui_channel_out_read;
	open_request.parameterB = g_ui_channel_in_write;

	auto request_status = g_send_msg(window_mgr, &open_request);
	if (request_status != G_MESSAGE_SEND_STATUS_SUCCESSFUL) {
		g_logger::log("failed to send UI-open request to window server");
		return G_UI_OPEN_STATUS_COMMUNICATION_FAILED;
	}

	// wait for response
	g_message_empty(open_response);
	auto response_status = g_recv_topic_msg(g_get_tid(), topic, &open_response);
	if (response_status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		g_logger::log("failed to receive UI-open response from window server");
		return G_UI_OPEN_STATUS_COMMUNICATION_FAILED;
	}

	// check response message
	if (open_response.type != G_UI_COMMAND_OPEN_RESPONSE) {
		g_logger::log("window servers UI-open response was not a proper 'opened'-response");
		return G_UI_OPEN_STATUS_COMMUNICATION_FAILED;
	}

	// start asynchronous receiver
	g_create_thread((void*) &asynchronous_receiver_thread);
	g_create_thread((void*) &event_dispatch_thread);

	// mark UI as ready
	g_logger::log("successfully opened UI in window server");
	g_ui_ready = true;
	return G_UI_OPEN_STATUS_SUCCESSFUL;
}

/**
 *
 */
void g_ui::event_dispatch_thread() {

	while (true) {
		// wait for events
		g_atomic_block(&event_dispatch_events_empty);

		// lock
		g_atomic_lock(&event_dispatch_locked);

		// call listener
		g_ui_event_dispatch_data& e = event_dispatch_queue.back();
		event_dispatch_queue.pop_back();
		e.listener->event_received(e.data, e.length);

		// check if empty
		if (event_dispatch_queue.empty()) {
			event_dispatch_events_empty = true;
		}

		// unlock
		event_dispatch_locked = false;
	}
}

/**
 *
 */
void g_ui::event_dispatch_queue_add(const g_ui_event_dispatch_data& data) {
	event_dispatch_queue.push_front(data);
	event_dispatch_events_empty = false;
}

/**
 * Waits for responses from the window manager and stores them
 * in the transaction message map.
 */
void g_ui::asynchronous_receiver_thread() {

	while (true) {

		// TODO properly check if each read/write was successful & validate the data length

		// read the id
		uint32_t idlen = sizeof(g_ui_transaction_id);
		uint8_t id[idlen];
		g_read(g_ui_channel_in, &id, idlen);
		g_ui_transaction_id transaction = *((g_ui_transaction_id*) id);

		// read the length
		uint32_t lenlen = sizeof(uint32_t);
		uint8_t len[lenlen];
		g_read(g_ui_channel_in, &len, lenlen);
		uint32_t length = *((uint32_t*) len);

		// read the data
		uint8_t* data = new uint8_t[length];
		int32_t data_read = 0;
		while (data_read < length) {
			data_read += g_read(g_ui_channel_in, &data[data_read], length - data_read);
		}

		// no transaction? -> event
		if (transaction == 0) {

			// get id from data
			g_value_placer data_reader(data);
			uint32_t listener_id = data_reader.get<uint32_t>();

			// notify listener
			if (listeners.count(listener_id) > 0) {
				g_listener* listener = listeners.at(listener_id);

				// add event to dispatch queue
				g_ui_event_dispatch_data ldata;
				ldata.listener = listener;
				ldata.data = data;
				ldata.length = length;
				event_dispatch_queue_add(ldata);
			}

		} else {
			// does map even exist?
			if (transaction_map == 0) {
				g_logger::log("transaction map did not exist when receiving request");
				break;
			}

			// check if data exists
			if (transaction_map->count(transaction) < 1) {
				g_logger::log("transaction map did not contain data for a request that was received");
				break;
			}

			// update the data
			g_ui_transaction_data* transaction_data = transaction_map->at(transaction);
			transaction_data->data = data;
			transaction_data->length = length;
			transaction_data->waiting = false;
		}
	}
}

/**
 * Sends a message (thread-safe) to the window manager.
 */
g_ui_transaction_id g_ui::send(uint8_t* data, uint32_t length) {

	// check if ready
	if (!g_ui_ready) {
		return -1;
	}

	static uint8_t sending_locked = false;

	// lock
	g_atomic_lock(&sending_locked);

	// create transaction
	g_ui_transaction_id transaction = next_transaction++;

	// create data for response
	g_ui_transaction_data* response_data = new g_ui_transaction_data;
	response_data->data = 0;
	response_data->length = 0;
	response_data->waiting = true;

	// insert to map
	if (transaction_map == 0) {
		transaction_map = new std::map<g_ui_transaction_id, g_ui_transaction_data*>();
	}
	transaction_map->insert(std::make_pair(transaction, response_data));

	// write transaction id
	uint32_t idlen = sizeof(g_ui_transaction_id);
	uint8_t idbytes[idlen];
	*((g_ui_transaction_id*) idbytes) = transaction;
	g_write(g_ui_channel_out, idbytes, idlen);

	// write length
	uint32_t lenlen = sizeof(uint32_t);
	uint8_t lenbytes[lenlen];
	*((uint32_t*) lenbytes) = length;
	g_write(g_ui_channel_out, lenbytes, lenlen);

	// write data
	int32_t written = 0;
	while (written < length) {
		written += g_write(g_ui_channel_out, &data[written], length - written);
	}

	// unlock
	sending_locked = false;

	return transaction;
}

/**
 * Waits for the response on a specific transaction. On success, the output parameters are
 * filled with buffers containing the data.
 */
bool g_ui::receive(g_ui_transaction_id transaction, uint8_t** out_data, uint32_t* out_length) {

	// does map even exist?
	if (transaction_map == 0) {
		return false;
	}

	// does transaction exist?
	if (transaction_map->count(transaction) < 1) {
		return false;
	}

	// take data
	g_ui_transaction_data* data = transaction_map->at(transaction);

	// block until received
	g_atomic_block(&data->waiting);

	// set out parameters
	*out_data = data->data;
	*out_length = data->length;

	// remove entry
	transaction_map->erase(transaction);
	delete data;

	return true;
}

/**
 *
 */
void g_ui::add_listener(g_listener* l) {
	listeners.insert(std::make_pair(l->getListenerId(), l));
}

/**
 *
 */
void g_ui::remove_listener(g_listener* l) {
	listeners.erase(l->getListenerId());
}
