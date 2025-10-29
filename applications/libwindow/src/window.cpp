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

#include <functional>
#include <utility>

#include "libwindow/properties.hpp"
#include "libwindow/window.hpp"

class g_window_close_dispatcher : public g_listener
{
	std::function<void()> func;

public:
	explicit g_window_close_dispatcher(std::function<void()> func) :
		func(std::move(func))
	{
	}

	void process(g_ui_component_event_header* header) override
	{
		func();
	}
};

g_window* g_window::create()
{
	return createComponent<g_window, G_UI_COMPONENT_TYPE_WINDOW>();
}

g_window* g_window::attach(g_ui_component_id id)
{
	return attachComponent<g_window>(id);
}

bool g_window::isResizable()
{
	uint32_t value;
	getNumericProperty(G_UI_PROPERTY_RESIZABLE, &value);
	return value;
}

void g_window::setResizable(bool resizable)
{
	setNumericProperty(G_UI_PROPERTY_RESIZABLE, resizable);
}

bool g_window::onClose(std::function<void()> func)
{
	return addListener(G_UI_COMPONENT_EVENT_TYPE_CLOSE, new g_window_close_dispatcher(std::move(func)));
}
