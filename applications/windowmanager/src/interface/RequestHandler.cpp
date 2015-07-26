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

#include <WindowManager.hpp>
#include <interface/RequestHandler.hpp>
#include <interface/ComponentStore.hpp>

#include <components/Window.hpp>
#include <components/Button.hpp>
#include <components/Label.hpp>
#include <components/text/TextField.hpp>
#include <components/TitledComponent.hpp>
#include <components/ActionComponent.hpp>

#include <ghostuser/utils/logger.hpp>
#include <ghostuser/tasking/ipc.hpp>
#include <ghostuser/ui/interface_specification.hpp>
#include <ghostuser/utils/value_placer.hpp>

#include <ghost/utils/local.hpp>
#include <map>
#include <deque>

/**
 * Sending messages must be protected by a lock
 */
static uint8_t sending_locked = false;

/**
 * The map of processes containing the process UI communication pipe identifiers
 * must be accessed thread-safe
 */
static uint8_t process_map_lock = false;
static std::map<g_pid, UIRegisteredProcessData> processMap;

/**
 * Event dispatch thread data
 */
static uint8_t event_dispatch_events_empty = true;
static std::deque<UIEventDispatchData> event_dispatch_queue;

/**
 *
 */
void RequestHandler::run() {

	if (!g_task_register_id(G_WINDOW_MANAGER_IDENTIFIER)) {
		g_logger::log("window manager: could not register with task identifier '%s'", (char*) G_WINDOW_MANAGER_IDENTIFIER);
		return;
	}

	uint32_t tid = g_get_tid();

	g_logger::log("window manager: ready for requests");
	while (true) {
		g_message* request = new g_message;
		g_recv_msg(tid, request);

		if (request->type == G_UI_COMMAND_OPEN_REQUEST) {
			g_create_thread_d((void*) handling_thread, (void*) request);

		} else {
			g_logger::log("window manager: received unknown command %i from task %i", request->type, request->sender);

		}
	}
}

/**
 *
 */
void RequestHandler::add_process(g_pid process, g_fd output, g_fd input) {

	g_atomic_wait(&process_map_lock);

	UIRegisteredProcessData data;
	data.pid = process;
	data.output = output;
	data.input = input;
	processMap.insert(std::make_pair(process, data));

	process_map_lock = false;
}

/**
 *
 */
void RequestHandler::remove_process(g_pid process) {

	g_atomic_wait(&process_map_lock);

	processMap.erase(process);

	process_map_lock = false;
}

/**
 *
 */
uint32_t RequestHandler::nextListenerId() {

	static uint8_t locked = false;
	static uint32_t next_id = 0;

	// lock
	g_atomic_wait(&locked);

	uint32_t id = next_id++;

	// unlock
	locked = false;

	return id;
}

/**
 *
 */
void RequestHandler::send(g_fd out, g_ui_transaction_id transaction, uint8_t* data, uint32_t length) {

	// lock
	g_atomic_wait(&sending_locked);

	// write transaction id
	uint32_t idlen = sizeof(g_ui_transaction_id);
	uint8_t idbytes[idlen];
	*((g_ui_transaction_id*) idbytes) = transaction;
	g_write(out, idbytes, idlen);

	// write length
	uint32_t lenlen = sizeof(uint32_t);
	uint8_t lenbytes[lenlen];
	*((uint32_t*) lenbytes) = length;
	g_write(out, lenbytes, lenlen);

	// write data
	uint32_t written = 0;
	while (written < length) {
		written += g_write(out, &data[written], length - written);
	}

	// unlock
	sending_locked = false;
}

/**
 *
 */
void RequestHandler::send_event(g_pid process, uint32_t listener_id, uint8_t* data, uint32_t length) {

	// lock
	g_atomic_wait(&process_map_lock);

	if (processMap.count(process) > 0) {
		UIRegisteredProcessData& procdat = processMap.at(process);

		// add event data to dispatcher queue
		UIEventDispatchData event_data;
		event_data.output = procdat.output;
		event_data.listener = listener_id;
		event_data.data = data;
		event_data.length = length;
		event_dispatch_queue_add(event_data);
	}

	// unlock
	process_map_lock = false;
}

/**
 *
 */
void RequestHandler::event_dispatch_thread() {

	// register a name
	std::stringstream namestr;
	namestr << "windowserver:event-dispatcher";
	g_task_register_id(namestr.str().c_str());

	while (true) {
		// wait for events
		g_atomic_block(&event_dispatch_events_empty);

		// lock
		g_atomic_wait(&sending_locked);

		// call listener
		UIEventDispatchData ldata = event_dispatch_queue.back();
		event_dispatch_queue.pop_back();

		// write transaction id
		uint32_t idlen = sizeof(g_ui_transaction_id);
		uint8_t idbytes[idlen];
		*((g_ui_transaction_id*) idbytes) = 0;
		g_write(ldata.output, idbytes, idlen);

		// write length
		uint32_t lenlen = sizeof(uint32_t);
		uint8_t lenbytes[lenlen];
		*((uint32_t*) lenbytes) = ldata.length + 4;
		g_write(ldata.output, lenbytes, lenlen);

		// write listener id
		uint32_t lidlen = sizeof(uint32_t);
		uint8_t lidbytes[lidlen];
		*((uint32_t*) lidbytes) = ldata.listener;
		g_write(ldata.output, lidbytes, lidlen);

		// write data
		uint32_t written = 0;
		while (written < ldata.length) {
			written += g_write(ldata.output, &ldata.data[written], ldata.length - written);
		}

		// delete the data
		delete ldata.data;

		// check if empty
		if (event_dispatch_queue.empty()) {
			event_dispatch_events_empty = true;
		}

		// unlock
		sending_locked = false;
	}
}

/**
 *
 */
void RequestHandler::event_dispatch_queue_add(const UIEventDispatchData& data) {
	event_dispatch_queue.push_front(data);
	event_dispatch_events_empty = false;
}

/**
 *
 */
void RequestHandler::handling_thread(g_message* _request) {

	// wrap in local for auto-delete
	g_local < g_message > request(_request);

	// read parameters
	g_pid requester_pid = g_get_pid_for_tid(request()->sender);
	g_fd requesters_output = request()->parameterA;
	g_fd requesters_input = request()->parameterB;

	g_pid my_pid = g_get_pid();

	// register a name
	std::stringstream namestr;
	namestr << "windowserver:handler@";
	namestr << requester_pid;
	g_task_register_id(namestr.str().c_str());

	// clone pipe ends
	g_fs_clonefd_status clone_input_status;
	g_fd requester_out = g_clone_fd_s(requesters_input, requester_pid, my_pid, &clone_input_status);
	if (clone_input_status != G_FS_CLONEFD_SUCCESSFUL) {
		g_logger::log("unable to clone input file descriptor (%i in process %i) on open request (status: %i)", requesters_input, requester_pid,
				clone_input_status);
		return;
	}

	g_fs_clonefd_status clone_output_status;
	g_fd requester_in = g_clone_fd_s(requesters_output, requester_pid, my_pid, &clone_output_status);
	if (clone_output_status != G_FS_CLONEFD_SUCCESSFUL) {
		g_logger::log("unable to clone output file descriptor (%i in process %i) on open request (status: %i)", requesters_input, requester_pid,
				clone_output_status);
		return;
	}

	// send response
	g_message_empty (response);
	response.type = G_UI_COMMAND_OPEN_RESPONSE;
	response.topic = request()->topic;
	g_send_msg(request()->sender, &response);

	// add process
	add_process(requester_pid, requester_out, requester_in);

	// start event dispatch thread
	g_create_thread((void*) &event_dispatch_thread);

	while (true) {
		// read transaction id
		uint32_t idlen = sizeof(g_ui_transaction_id);
		uint8_t id[idlen];
		g_read(requester_in, id, idlen);
		g_ui_transaction_id transaction = *((g_ui_transaction_id*) id);

		// read length
		uint32_t lenlen = sizeof(uint32_t);
		uint8_t len[lenlen];
		g_read(requester_in, len, lenlen);
		uint32_t length = *((uint32_t*) len);

		// read data
		// TODO limit data
		uint8_t* data = new uint8_t[length];
		int32_t rd = 0;
		while (rd < length) {
			rd += g_read(requester_in, &data[rd], length - rd);
		}
		g_value_placer data_reader(data);

		// handle command
		g_ui_protocol_command_id command = data_reader.get<g_ui_protocol_command_id>();

		if (command == G_UI_PROTOCOL_CREATE_WINDOW) {
			uint32_t window_id;
			g_ui_protocol_status status = createWindow(&window_id);

			// write response
			uint32_t response_len = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_CREATE_WINDOW_RESPONSE_LENGTH;
			g_local < uint8_t > response(new uint8_t[response_len]);

			g_value_placer response_writer(response());
			response_writer.put(G_UI_PROTOCOL_CREATE_WINDOW);
			response_writer.put(status);
			response_writer.put(window_id);
			send(requester_out, transaction, response(), response_len);

		} else if (command == G_UI_PROTOCOL_SET_VISIBLE) {
			uint32_t component_id = data_reader.get<uint32_t>();
			bool visible = data_reader.get<uint8_t>();

			// handle command
			g_ui_protocol_status status = setVisible(component_id, visible);

			// write response
			uint32_t response_len = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_SET_VISIBLE_RESPONSE_LENGTH;
			g_local < uint8_t > response(new uint8_t[response_len]);
			g_value_placer response_writer(response());
			response_writer.put(G_UI_PROTOCOL_SET_VISIBLE);
			response_writer.put(status);
			send(requester_out, transaction, response(), response_len);

		} else if (command == G_UI_PROTOCOL_CREATE_COMPONENT) {
			uint32_t component_type = data_reader.get<uint32_t>();

			// handle command
			uint32_t component_id;
			g_ui_protocol_status status = createComponent(component_type, &component_id);

			// write response
			uint32_t response_length = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_CREATE_COMPONENT_RESPONSE_LENGTH;
			g_local < uint8_t > response(new uint8_t[response_length]);
			g_value_placer response_writer(response());
			response_writer.put(G_UI_PROTOCOL_CREATE_COMPONENT);
			response_writer.put(status);
			response_writer.put(component_id);
			send(requester_out, transaction, response(), response_length);

		} else if (command == G_UI_PROTOCOL_ADD_COMPONENT) {
			uint32_t parent_id = data_reader.get<uint32_t>();
			uint32_t child_id = data_reader.get<uint32_t>();

			// handle command
			g_ui_protocol_status status = addComponent(parent_id, child_id);

			// write response
			uint32_t response_length = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_ADD_COMPONENT_RESPONSE_LENGTH;
			g_local < uint8_t > response(new uint8_t[response_length]);
			g_value_placer response_writer(response());
			response_writer.put(G_UI_PROTOCOL_ADD_COMPONENT);
			response_writer.put(status);
			send(requester_out, transaction, response(), response_length);

		} else if (command == G_UI_PROTOCOL_SET_TITLE) {
			uint32_t component_id = data_reader.get<uint32_t>();
			uint32_t title_length = data_reader.get<uint32_t>();
			g_local<char> title(new char[title_length]);
			data_reader.get((uint8_t*) title(), title_length);

			// handle command
			g_ui_protocol_status status = setTitle(component_id, title());

			// write response
			uint32_t response_length = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_SET_TITLE_RESPONSE_LENGTH;
			g_local < uint8_t > response(new uint8_t[response_length]);
			g_value_placer response_writer(response());
			response_writer.put(G_UI_PROTOCOL_SET_TITLE);
			response_writer.put(status);
			send(requester_out, transaction, response(), response_length);

		} else if (command == G_UI_PROTOCOL_GET_TITLE) {
			uint32_t component_id = data_reader.get<uint32_t>();

			// handle command
			std::string title;
			g_ui_protocol_status status = getTitle(component_id, title);

			int title_length = title.length() + 1;

			// write response
			uint32_t response_length = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_GET_TITLE_RESPONSE_LENGTH + title_length;
			g_local < uint8_t > response(new uint8_t[response_length]);
			g_value_placer response_writer(response());
			response_writer.put(G_UI_PROTOCOL_GET_TITLE);
			response_writer.put(status);
			response_writer.put(title_length);
			response_writer.put((uint8_t*) title.c_str(), title_length);
			send(requester_out, transaction, response(), response_length);

		} else if (command == G_UI_PROTOCOL_SET_BOUNDS) {
			uint32_t component_id = data_reader.get<uint32_t>();
			int32_t x = data_reader.get<int32_t>();
			int32_t y = data_reader.get<int32_t>();
			int32_t width = data_reader.get<int32_t>();
			int32_t height = data_reader.get<int32_t>();

			// handle command
			g_ui_protocol_status status = setBounds(component_id, x, y, width, height);

			// write response
			uint32_t response_length = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_SET_BOUNDS_RESPONSE_LENGTH;
			g_local < uint8_t > response(new uint8_t[response_length]);
			g_value_placer response_writer(response());
			response_writer.put(G_UI_PROTOCOL_SET_BOUNDS);
			response_writer.put(status);
			send(requester_out, transaction, response(), response_length);

		} else if (command == G_UI_PROTOCOL_SET_ACTION_LISTENER) {
			uint32_t component_id = data_reader.get<uint32_t>();

			// handle command
			uint32_t listener_id;
			g_ui_protocol_status status = setActionListener(requester_pid, component_id, &listener_id);

			// write response
			uint32_t response_length = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_SET_ACTION_LISTENER;
			g_local < uint8_t > response(new uint8_t[response_length]);
			g_value_placer response_writer(response());
			response_writer.put<g_ui_protocol_command_id>(G_UI_PROTOCOL_SET_ACTION_LISTENER);
			response_writer.put<g_ui_protocol_status>(status);
			response_writer.put<uint32_t>(listener_id);
			send(requester_out, transaction, response(), response_length);
		}
	}

	// TODO close all windows
	// TODO remove listeners
	remove_process(requester_pid);
}

/**
 *
 */
g_ui_protocol_status RequestHandler::setActionListener(g_pid process, uint32_t component_id, uint32_t* out_listener_id) {

	Component* component = ComponentStore::get(component_id);
	if (component == 0) {
		return G_UI_PROTOCOL_FAIL;
	}

	ActionComponent* act = dynamic_cast<ActionComponent*>(component);
	if (act) {
		uint32_t listener_id = nextListenerId();
		act->setActionListener(process, listener_id);
		*out_listener_id = listener_id;
		return G_UI_PROTOCOL_SUCCESS;
	}
	return G_UI_PROTOCOL_FAIL;
}

/**
 *
 */
g_ui_protocol_status RequestHandler::setVisible(uint32_t component_id, bool visible) {

	Component* component = ComponentStore::get(component_id);
	if (component == 0) {
		return G_UI_PROTOCOL_FAIL;
	}

	component->setVisible(visible);
	return G_UI_PROTOCOL_SUCCESS;
}

/**
 *
 */
g_ui_protocol_status RequestHandler::setBounds(uint32_t component_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {

	Component* component = ComponentStore::get(component_id);
	if (component == 0) {
		return G_UI_PROTOCOL_FAIL;
	}

	component->setBounds(g_rectangle(x, y, width, height));
	return G_UI_PROTOCOL_SUCCESS;
}

/**
 *
 */
g_ui_protocol_status RequestHandler::getTitle(uint32_t component_id, std::string& out_title) {

	Component* component = ComponentStore::get(component_id);
	if (component == 0) {
		return G_UI_PROTOCOL_FAIL;
	}

	TitledComponent* tit = dynamic_cast<TitledComponent*>(component);
	if (tit) {
		out_title = tit->getTitle();
		return G_UI_PROTOCOL_SUCCESS;
	}

	return G_UI_PROTOCOL_FAIL;
}

/**
 *
 */
g_ui_protocol_status RequestHandler::setTitle(uint32_t component_id, char* title) {

	Component* component = ComponentStore::get(component_id);
	if (component == 0) {
		return G_UI_PROTOCOL_FAIL;
	}

	TitledComponent* tit = dynamic_cast<TitledComponent*>(component);
	if (tit) {
		tit->setTitle(title);
		return G_UI_PROTOCOL_SUCCESS;
	}

	return G_UI_PROTOCOL_FAIL;
}

/**
 *
 */
g_ui_protocol_status RequestHandler::createWindow(uint32_t* out_id) {

	// Create the window
	Window* win = new Window();
	win->setBounds(g_rectangle(10, 10, 100, 60));
	uint32_t ident = ComponentStore::add(win);

	WindowManager::getInstance()->addToScreen(win);

	*out_id = ident;
	return G_UI_PROTOCOL_SUCCESS;
}

/**
 *
 */
g_ui_protocol_status RequestHandler::createComponent(uint32_t component_type, uint32_t* out_id) {

	// Create the component
	Component* comp = 0;

	if (component_type == G_UI_COMPONENT_BUTTON) {
		comp = new Button();
		comp->setBounds(g_rectangle(0, 0, 100, 50));
	} else if (component_type == G_UI_COMPONENT_LABEL) {
		comp = new Label();
		comp->setBounds(g_rectangle(0, 0, 100, 50));
	} else if (component_type == G_UI_COMPONENT_TEXTFIELD) {
		comp = new TextField();
		comp->setBounds(g_rectangle(0, 0, 100, 50));
	}

	// Store the component
	if (comp != 0) {
		*out_id = ComponentStore::add(comp);
		return G_UI_PROTOCOL_SUCCESS;
	}

	return G_UI_PROTOCOL_FAIL;
}

/**
 *
 */
g_ui_protocol_status RequestHandler::addComponent(uint32_t parent_id, uint32_t child_id) {

	// Find both components
	if (parent_id == child_id) {
		return G_UI_PROTOCOL_FAIL;
	}

	Component* parent = ComponentStore::get(parent_id);
	if (parent == 0) {
		return G_UI_PROTOCOL_FAIL;
	}

	Component* child = ComponentStore::get(child_id);
	if (child == 0) {
		return G_UI_PROTOCOL_FAIL;
	}

	parent->addChild(child);
	return G_UI_PROTOCOL_SUCCESS;
}
