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

#ifndef __WINDOWSERVER_COMPONENTS_CHECKBOX__
#define __WINDOWSERVER_COMPONENTS_CHECKBOX__

#include "components/component.hpp"
#include "components/label.hpp"

#define DEFAULT_BOX_SIZE 20
#define DEFAULT_BOX_TEXT_GAP 5

class checkbox_t : public component_t
{
    label_t label;
    bool checked;
    int boxSize;
    int boxTextGap;

    bool hovered;
    bool pressed;

public:
    checkbox_t();

    void layout() override;
    void paint() override;

    component_t* handleMouseEvent(mouse_event_t& e) override;
    void handleBoundChanged(const g_rectangle& oldBounds) override;

    label_t& getLabel()
    {
        return label;
    }
};

#endif
