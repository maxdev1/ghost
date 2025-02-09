/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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
#include "components/cursor.hpp"
#include <libinput/keyboard/keyboard.hpp>
#include <libinput/mouse/mouse.hpp>

#include <ghost.h>
#include <libps2driver/ps2driver.hpp>

static g_fd keyboardIn;
static g_fd mouseIn;

void input_receiver_t::initialize()
{
	g_tid keyEventThread = g_create_task((void*) startReceiveKeyEvents);
	g_tid mouseEventThread = g_create_task((void*) startReceiveMouseEvents);
	ps2DriverInitialize(&keyboardIn, &mouseIn, keyEventThread, mouseEventThread);
}

void input_receiver_t::startReceiveKeyEvents()
{
	g_task_register_id("windowserver/key-receiver");

	event_processor_t* event_queue = windowserver_t::instance()->eventProcessor;

	while(true)
	{
		g_key_info key = g_keyboard::readKey(keyboardIn);

		if(key.ctrl && key.key == "KEY_Q" && key.pressed)
		{
			windowserver_t::setDebug(!windowserver_t::isDebug());
			continue;
		}

		event_queue->bufferKeyEvent(key);

		windowserver_t::instance()->requestUpdate();
	}
}

void input_receiver_t::startReceiveMouseEvents()
{
	g_task_register_id("windowserver/mouse-receiver");

	windowserver_t* instance = windowserver_t::instance();
	g_dimension resolution = instance->videoOutput->getResolution();

	while(true)
	{
		g_mouse_info info = g_mouse::readMouse(mouseIn);

		cursor_t::nextPosition.x += info.x;
		cursor_t::nextPosition.y += info.y;

		if(cursor_t::nextPosition.x < 0)
		{
			cursor_t::nextPosition.x = 0;
		}
		if(cursor_t::nextPosition.x > resolution.width - 2)
		{
			cursor_t::nextPosition.x = resolution.width - 2;
		}
		if(cursor_t::nextPosition.y < 0)
		{
			cursor_t::nextPosition.y = 0;
		}
		if(cursor_t::nextPosition.y > resolution.height - 2)
		{
			cursor_t::nextPosition.y = resolution.height - 2;
		}

		cursor_t::nextPressedButtons = G_MOUSE_BUTTON_NONE;
		if(info.button1)
		{
			cursor_t::nextPressedButtons |= G_MOUSE_BUTTON_1;
		}
		if(info.button2)
		{
			cursor_t::nextPressedButtons |= G_MOUSE_BUTTON_2;
		}
		if(info.button3)
		{
			cursor_t::nextPressedButtons |= G_MOUSE_BUTTON_3;
		}

		windowserver_t::instance()->requestUpdate();
	}
}
