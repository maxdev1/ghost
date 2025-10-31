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

#ifndef __WINDOWSERVER_COMPONENTS_TEXT_TEXTCOMPONENT__
#define __WINDOWSERVER_COMPONENTS_TEXT_TEXTCOMPONENT__

#include "components/component.hpp"
#include "components/text/move/caret_move_strategy.hpp"
#include <libwindow/metrics/range.hpp>

class text_component_t : virtual public component_t
{
protected:
    caret_move_strategy_t* caretMoveStrategy = nullptr;

public:
    ~text_component_t() override = default;

    virtual void setCursor(int position) = 0;
    virtual int getCursor() = 0;

    virtual void setMarker(int position) = 0;
    virtual int getMarker() = 0;

    virtual void setText(std::string text) = 0;
    virtual std::string getText() = 0;

    virtual g_range getSelectedRange() = 0;
};

#endif
