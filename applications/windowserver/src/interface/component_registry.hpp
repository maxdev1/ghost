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

#ifndef __WINDOWSERVER_INTERFACE_COMPONENTREGISTRY__
#define __WINDOWSERVER_INTERFACE_COMPONENTREGISTRY__

#include <libwindow/interface.hpp>

#include "components/component.hpp"

class component_registry_t
{
public:
    static g_ui_component_id add(component_t* component);
    static void bind(g_pid process, component_t* component);
    static component_t* get(g_ui_component_id id);

    static void removeComponent(g_pid pid, g_ui_component_id id);
    static void cleanupProcess(g_pid pid);

private:
    static void removeProcessComponents(g_pid process, component_t* component,
                                          std::list<component_t*>& removedComponents);
};

#endif
