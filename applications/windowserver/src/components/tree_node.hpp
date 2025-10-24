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

#ifndef __WINDOWSERVER_COMPONENTS_TREE_NODE__
#define __WINDOWSERVER_COMPONENTS_TREE_NODE__

#include "components/component.hpp"
#include "components/label.hpp"

class tree_node_t : virtual public component_t, virtual public titled_component_t
{
    label_t label;
    bool open = false;

public:
    tree_node_t();
    ~tree_node_t() override = default;

    void update() override;
    void layout() override;

    void setTitleInternal(std::string title) override;
    std::string getTitle() override;

    component_t* handleMouseEvent(mouse_event_t& event) override;
};


#endif
