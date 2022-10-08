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

#ifndef __WINDOWSERVER_COMPONENTS_TEXT_MOVE_CARETMOVESTRATEGY__
#define __WINDOWSERVER_COMPONENTS_TEXT_MOVE_CARETMOVESTRATEGY__

#include "components/text/caret_direction.hpp"

#include <libinput/keyboard/keyboard.hpp>

class text_component_t;

class caret_move_strategy_t
{
  public:
    virtual ~caret_move_strategy_t()
    {
    }

    virtual void moveCaret(text_component_t* component, caret_direction_t direction, g_key_info& keyInfo) = 0;

    virtual int calculateSkip(std::string text, int position, caret_direction_t direction) = 0;
};

#endif
