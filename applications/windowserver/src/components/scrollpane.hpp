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

#ifndef __WINDOWSERVER_COMPONENTS_SCROLLPANE__
#define __WINDOWSERVER_COMPONENTS_SCROLLPANE__

#include "components/component.hpp"
#include "components/panel.hpp"
#include "components/scrollbar.hpp"

class scrollpane_t : public component_t, public scroll_handler_t
{
  private:
    component_t* viewPort;
    g_point scrollPosition;
    scrollbar_t verticalScrollbar;
    scrollbar_t horizontalScrollbar;

  public:
    scrollpane_t() : scrollPosition(g_point(0, 0)), viewPort(nullptr),
                     verticalScrollbar(scrollbar_orientation_t::VERTICAL),
                     horizontalScrollbar(scrollbar_orientation_t::HORIZONTAL)
    {
    }

    virtual g_point getPosition() const
    {
        return scrollPosition;
    }

    virtual void layout();

    virtual void setPosition(g_point& position);

    virtual void setViewPort(component_t* content);

    virtual component_t* getViewPort() const
    {
        return viewPort;
    }

    virtual void handleScroll(scrollbar_t* bar);
};

#endif
