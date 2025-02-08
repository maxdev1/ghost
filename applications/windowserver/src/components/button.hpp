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

#ifndef __WINDOWSERVER_COMPONENTS_BUTTON__
#define __WINDOWSERVER_COMPONENTS_BUTTON__

#include "components/action_component.hpp"
#include "components/button_state.hpp"
#include "components/component.hpp"
#include "components/label.hpp"
#include "components/titled_component.hpp"

#include <libwindow/metrics/insets.hpp>
#include <string>

class button_t : public component_t, public titled_component_t, public action_component_t
{
    button_state_t state;
    label_t label;
    g_insets insets;
    bool enabled;

public:
    button_t();
    ~button_t() override = default;

    void update() override;
    void layout() override;
    void paint() override;

    component_t* handleMouseEvent(mouse_event_t& e) override;
    component_t* handleFocusEvent(focus_event_t& e) override;

    void setTitle(std::string title) override;
    std::string getTitle() override;

    bool getNumericProperty(int property, uint32_t* out) override;
    bool setNumericProperty(int property, uint32_t value) override;

    virtual void setEnabled(bool enabled);

    virtual bool isEnabled() const
    {
        return enabled;
    }

    label_t& getLabel()
    {
        return label;
    }
};

#endif
