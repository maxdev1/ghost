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

#include "component_registry.hpp"
#include <map>

static std::map<g_ui_component_id, component_t*> components;
static std::map<g_pid, std::map<g_ui_component_id, component_t*>> components_by_process;
static g_ui_component_id next_id = 1;

/**
 *
 */
g_ui_component_id component_registry_t::add(g_pid process, component_t* component) {
	g_ui_component_id id = next_id++;
	components[id] = component;
	components_by_process[process][id] = component;
	component->id = id;
	return id;
}

/**
 *
 */
component_t* component_registry_t::get(g_ui_component_id id) {
	if (components.count(id) > 0) {
		return components[id];
	}
	return 0;
}

/**
 *
 */
std::map<g_ui_component_id, component_t*>* component_registry_t::get_process_map(g_pid pid) {
	if (components_by_process.count(pid) > 0) {
		return &components_by_process[pid];
	}
	return nullptr;
}

/**
 *
 */
void component_registry_t::remove_component(g_pid pid, g_ui_component_id id) {

	if (components.count(id) > 0) {
		if (components_by_process.count(pid) > 0) {
			components_by_process[pid].erase(components_by_process[pid].find(id));
		}
		components.erase(components.find(id));
	}
}

/**
 *
 */
void component_registry_t::remove_process_map(g_pid pid) {
	components_by_process.erase(components_by_process.find(pid));
}
