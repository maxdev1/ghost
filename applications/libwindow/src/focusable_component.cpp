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

#include <utility>

#include "libwindow/focusable_component.hpp"
#include "libwindow/properties.hpp"
#include "libwindow/listener/focus_listener.hpp"

bool g_focusable_component::isFocused()
{
	uint32_t out;
	getNumericProperty(G_UI_PROPERTY_FOCUSED, &out);
	return out == 1;
}

bool g_focusable_component::setFocused(bool focused)
{
	return setNumericProperty(G_UI_PROPERTY_FOCUSED, focused ? 1 : 0);
}

void g_focusable_component::addFocusListener(std::function<void(bool)> func)
{
	this->addListener(G_UI_COMPONENT_EVENT_TYPE_FOCUS, new g_focus_listener_dispatcher(std::move(func)));
}
