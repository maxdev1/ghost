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

#ifndef __COMPONENT_REGISTRY__
#define __COMPONENT_REGISTRY__

#include <ghostuser/tasking/thread.hpp>
#include <components/component.hpp>
#include <ghostuser/ui/interface_specification.hpp>

/**
 *
 */
class component_registry_t {
public:
	static g_ui_component_id add(g_pid process, component_t* component);
	static component_t* get(g_ui_component_id id);
	static std::map<g_ui_component_id, component_t*>* get_process_map(g_pid pid);

	static void remove_component(g_pid pid, g_ui_component_id id);
	static void remove_process_map(g_pid pid);
};

#endif
