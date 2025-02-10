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

#ifndef __WINDOWSERVER_COMPONENTS_WINDOW__
#define __WINDOWSERVER_COMPONENTS_WINDOW__

#include "components/component.hpp"
#include "components/label.hpp"
#include "components/panel.hpp"
#include "components/titled_component.hpp"

#include <bits/std_function.h>
#include <libwindow/color_argb.hpp>

#include "focusable_component.hpp"

/**
 * constants for border sizes
 */
#define DEFAULT_BORDER_WIDTH 7
#define DEFAULT_CORNER_SIZE 15

/**
 * modes used when resizing windows
 */
enum window_resize_mode_t
{
    RESIZE_MODE_NONE,
    RESIZE_MODE_MOVE,
    RESIZE_MODE_TOP,
    RESIZE_MODE_TOP_RIGHT,
    RESIZE_MODE_RIGHT,
    RESIZE_MODE_BOTTOM_RIGHT,
    RESIZE_MODE_BOTTOM,
    RESIZE_MODE_BOTTOM_LEFT,
    RESIZE_MODE_LEFT,
    RESIZE_MODE_TOP_LEFT
};

/**
 *
 */
class window_t :
    virtual public component_t,
    virtual public titled_component_t
{
    int borderWidth;
    int cornerSize;
    g_color_argb backgroundColor;
    bool resizable;
    bool focused;

    label_t label;
    panel_t panel;

    bool crossPressed;
    bool crossHovered;

    g_point pressPoint;
    g_rectangle pressBounds;
    window_resize_mode_t resizeMode;

    int shadowSize;
    int padding;
    g_rectangle crossBounds;

    std::function<void()> titleChangedListener{};

public:
    window_t();

    ~window_t() override = default;

    panel_t* getPanel()
    {
        return &panel;
    }

    void setBackground(g_color_argb color)
    {
        backgroundColor = color;
        markFor(COMPONENT_REQUIREMENT_PAINT);
    }

    g_color_argb getBackground() const
    {
        return backgroundColor;
    }

    bool isWindow() override
    {
        return true;
    }

    void addChild(component_t* component,
                  component_child_reference_type_t type = COMPONENT_CHILD_REFERENCE_TYPE_DEFAULT) override;

    void layout() override;
    void paint() override;

    component_t* handleMouseEvent(mouse_event_t& e) override;

    bool isFocusable() const override
    {
        return true;
    }

    bool isFocused() const override;
    void setFocusedInternal(bool focused) override;

    bool getNumericProperty(int property, uint32_t* out) override;
    bool setNumericProperty(int property, uint32_t value) override;

    void setTitleInternal(std::string title) override;
    std::string getTitle() override;

    void onTitleChanged(const std::function<void()>& callback)
    {
        titleChangedListener = callback;
    }

    virtual void close();

    void setLayoutManager(layout_manager_t* layoutManager) override;
};

#endif
