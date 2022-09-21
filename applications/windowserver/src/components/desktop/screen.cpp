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

#include "components/desktop/screen.hpp"
#include "components/cursor.hpp"
#include "windowserver.hpp"

void screen_t::markDirty(g_rectangle rect)
{

    // Mark area as invalid
    if(invalid.x == 0 && invalid.y == 0 && invalid.width == 0 && invalid.height == 0)
    {
        invalid = rect;
    }
    else
    {
        int top = rect.getTop() < invalid.getTop() ? rect.getTop() : invalid.getTop();
        int left = rect.getLeft() < invalid.getLeft() ? rect.getLeft() : invalid.getLeft();
        int bottom = rect.getBottom() > invalid.getBottom() ? rect.getBottom() : invalid.getBottom();
        int right = rect.getRight() > invalid.getRight() ? rect.getRight() : invalid.getRight();

        invalid = g_rectangle(left, top, right - left, bottom - top);
    }

    // Fix invalid area
    if(invalid.x < 0)
    {
        invalid.width += invalid.x;
        invalid.x = 0;
    }
    if(invalid.y < 0)
    {
        invalid.height += invalid.y;
        invalid.y = 0;
    }
    if(invalid.x + invalid.width > getBounds().width)
    {
        invalid.width = getBounds().width - invalid.x;
    }
    if(invalid.y + invalid.height > getBounds().height)
    {
        invalid.height = getBounds().height - invalid.y;
    }
}

bool screen_t::handleMouseEvent(mouse_event_t& e)
{
    if(e.type == G_MOUSE_EVENT_DRAG)
    {
        if(pressing)
        {
            g_rectangle selection(pressPoint.x, pressPoint.y, e.position.x - pressPoint.x, e.position.y - pressPoint.y);
            windowserver_t::instance()->background->showSelection(selection);
        }
        return true;
    }
    else if(e.type == G_MOUSE_EVENT_DRAG_RELEASE)
    {
        if(pressing)
        {
            pressing = false;
            windowserver_t::instance()->background->hideSelection();
            return true;
        }
    }

    if(component_t::handleMouseEvent(e))
        return true;

    if(e.type == G_MOUSE_EVENT_PRESS)
    {
        pressing = true;
        pressPoint = e.position;
    }

    cursor_t::set("default");
    return true;
}