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

#ifndef __WINDOWSERVER_COMPONENTS_FOCUSABLECOMPONENT__
#define __WINDOWSERVER_COMPONENTS_FOCUSABLECOMPONENT__
#include <stdint-gcc.h>

class component_t;

/**
 *
 */
class focusable_component_t
{
    component_t* self;

protected:
    bool focused = false;
    bool focusable = true;
    bool dispatchesFocus = true;

public:
    explicit focusable_component_t(component_t* self) : self(self)
    {
    }

    virtual ~focusable_component_t() = default;

    virtual bool isFocused() const
    {
        return focused;
    }

    virtual void setFocusedInternal(bool focused)
    {
        this->focused = focused;
    }

    virtual bool isFocusable() const
    {
        return isFocusableDefault() && focusable;
    }

    virtual bool isFocusableDefault() const
    {
        return false;
    }

    void setDispatchesFocus(bool dispatches)
    {
        this->dispatchesFocus = dispatchesFocus;
    }

    bool isDispatchesFocus()
    {
        return this->dispatchesFocus;
    }

    component_t* setFocused(bool focused);

    bool getNumericProperty(int property, uint32_t* out);
    bool setNumericProperty(int property, uint32_t value);
};

#endif
