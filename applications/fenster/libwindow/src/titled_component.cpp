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

#include "libwindow/titled_component.hpp"
#include "libwindow/listener/title_listener.hpp"
#include "libwindow/properties.hpp"

bool g_titled_component::setTitle(std::string title)
{
	return setStringProperty(G_UI_PROPERTY_TITLE, title);
}

std::string g_titled_component::getTitle()
{
	std::string out;
	getStringProperty(G_UI_PROPERTY_TITLE, out);
	return out;
}

void g_titled_component::addTitleListener(std::function<void(std::string)> func)
{
	this->addListener(G_UI_COMPONENT_EVENT_TYPE_TITLE, new g_title_listener_dispatcher(std::move(func)));
}
