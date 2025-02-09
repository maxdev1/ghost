/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef LIBWINDOW_KEYLISTENER
#define LIBWINDOW_KEYLISTENER

#include "listener.hpp"
#include "../interface.hpp"

#include <libinput/keyboard/keyboard.hpp>
#include <bits/std_function.h>

struct g_key_event
{
	g_key_info_basic info;
};

typedef std::function<void(g_key_event&)> g_key_listener_func;

class g_key_listener : public g_listener
{
public:
	void process(g_ui_component_event_header* header) override
	{
		auto event = (g_ui_component_key_event*) header;

		g_key_event e;
		e.info = event->key_info;
		handleKeyEvent(e);
	}

	virtual void handleKeyEvent(g_key_event& e) = 0;
};

class g_key_listener_dispatcher : public g_key_listener
{
	g_key_listener_func func;

public:
	explicit g_key_listener_dispatcher(g_key_listener_func func):
		func(std::move(func))
	{
	}

	void handleKeyEvent(g_key_event& e) override
	{
		func(e);
	}
};

#endif
