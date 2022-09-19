/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "layout/grid_layout_manager.hpp"
#include "components/component.hpp"

#include <typeinfo>
#include <vector>

/**
 *
 */
grid_layout_manager_t::grid_layout_manager_t(int columns, int rows) : columns(columns), rows(rows), padding(g_insets(0, 0, 0, 0)), horizontalCellSpace(0), verticalCellSpace(0)
{
}

/**
 *
 */
void grid_layout_manager_t::layout()
{

    if(component == 0)
    {
        return;
    }

    std::vector<component_child_reference_t>& children = component->getChildren();

    g_rectangle usedBounds = component->getBounds();
    usedBounds.x = padding.left;
    usedBounds.y = padding.top;
    usedBounds.width -= padding.left + padding.right;
    usedBounds.height -= padding.top + padding.bottom;

    int x = usedBounds.x;
    int y = usedBounds.y;
    int rowHeight = 0;

    int widthPerComponent = (columns > 0) ? (usedBounds.width / columns) : usedBounds.width;

    for(auto& ref : children)
    {
        component_t* c = ref.component;

        int usedHeight = (rows > 0) ? (usedBounds.height / rows) : c->getPreferredSize().height;

        if(x + widthPerComponent > usedBounds.width)
        {
            x = usedBounds.x;
            y += rowHeight;
            rowHeight = 0;
        }

        c->setBounds(g_rectangle(x, y, widthPerComponent, usedHeight));
        x += widthPerComponent;

        if(usedHeight > rowHeight)
        {
            rowHeight = usedHeight;
        }
    }
}
