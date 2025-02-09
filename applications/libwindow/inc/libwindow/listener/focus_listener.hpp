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

#ifndef LIBWINDOW_FOCUSLISTENER
#define LIBWINDOW_FOCUSLISTENER

#include "listener.hpp"
#include "../interface.hpp"
#include <bits/std_function.h>

class g_component;

typedef std::function<void(bool)> g_focus_listener_func;

class g_focus_listener : public g_listener
{
public:
	void process(g_ui_component_event_header* header) override
	{
		auto event = (g_ui_component_focus_event*) header;
		handleFocusChanged(event->now_focused);
	}

	virtual void handleFocusChanged(bool nowFocused) = 0;
};

class g_focus_listener_dispatcher : public g_focus_listener
{
	g_focus_listener_func func;

public:
	explicit g_focus_listener_dispatcher(g_focus_listener_func func):
		func(std::move(func))
	{
	}

	void handleFocusChanged(bool nowFocused) override
	{
		func(nowFocused);
	}
};

#endif
