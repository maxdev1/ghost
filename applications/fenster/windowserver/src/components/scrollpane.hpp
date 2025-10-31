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
#include "components/scrollbar.hpp"

class scrollpane_t : virtual public component_t, public scroll_handler_t
{
    component_t* content = nullptr;
    g_point scrollPosition = g_point(0, 0);
    scrollbar_t verticalScrollbar = scrollbar_t(scrollbar_orientation_t::VERTICAL);
    scrollbar_t horizontalScrollbar = scrollbar_t(scrollbar_orientation_t::HORIZONTAL);

    bool fixedWidth = false;
    bool fixedHeight = false;

    bool showHbar = false;
    bool showVbar = false;

    virtual void updateContent();

protected:
    bool hasGraphics() const override
    {
        return false;
    }

public:
    scrollpane_t();

    virtual g_point getPosition() const
    {
        return scrollPosition;
    }

    void layout() override;

    component_t* handleMouseEvent(mouse_event_t& event) override;
    void handleScroll(scrollbar_t* bar) override;

    virtual void setContent(component_t* content);

    virtual component_t* getContent() const
    {
        return content;
    }

    g_dimension calculateViewport(g_dimension contentPrefSize);

    void setFixedWidth(bool fix)
    {
        this->fixedWidth = fix;
    }

    void setFixedHeight(bool fix)
    {
        this->fixedHeight = fix;
    }
};

#endif
