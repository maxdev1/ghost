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

#include "input_receiver.hpp"
#include <ghost.h>
#include <ghostuser/io/keyboard.hpp>
#include <ghostuser/io/mouse.hpp>

/**
 *
 */
void input_receiver_t::initialize() {
	g_create_thread((void*) input_receiver_t::startReceiveMouseEvents);
	g_create_thread((void*) input_receiver_t::startReceiveKeyEvents);
}

/**
 *
 */
void input_receiver_t::startReceiveKeyEvents() {
	g_task_register_id("windowserver/keyReceiver");

	event_processor_t* event_queue = windowserver_t::instance()->event_processor;

	g_key_info info;
	while (true) {
		info = g_keyboard::readKey();
		event_queue->queueKeyEvent(info);
	}
}

/**
 *
 */
void input_receiver_t::startReceiveMouseEvents() {
	g_task_register_id("windowserver/mouseReceiver");

	event_processor_t* event_queue = windowserver_t::instance()->event_processor;

	g_mouse_info info;
	while (true) {
		info = g_mouse::readMouse();
		event_queue->queueMouseEvent(info);
	}
}
