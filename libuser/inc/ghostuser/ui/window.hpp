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

#ifndef GHOSTLIBRARY_UI_WINDOW
#define GHOSTLIBRARY_UI_WINDOW

#include <ghostuser/ui/component.hpp>
#include <ghostuser/ui/listener.hpp>
#include <ghostuser/ui/titled_component.hpp>
#include <cstdint>
#include <functional>

/**
 *
 */
class g_window: public g_component, public g_titled_component {
protected:
	g_window(uint32_t id) :
			g_component(id), g_titled_component(id) {
	}

public:
	static g_window* create();

	bool isResizable();
	void setResizable(bool resizable);

	bool onClose(std::function<void()> func);
};

#endif
