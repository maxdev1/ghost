/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef LIBWINDOW_COMPONENT
#define LIBWINDOW_COMPONENT

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "bounding_component.hpp"
#include "component_registry.hpp"
#include "interface.hpp"
#include "listener/mouse_listener.hpp"
#include "listener/visible_listener.hpp"
#include "metrics/rectangle.hpp"
#include "ui.hpp"
#include "color_argb.hpp"

/**
 * Template class for all components. Implements the basic component functionalities such as creation and bounds handling.
 */
class g_component : public g_bounding_component
{
protected:
    g_ui_component_id id;
    bool destroyed = false;

    g_user_mutex listenersLock = g_mutex_initialize_r(true);
    std::unordered_map<g_ui_component_event_type, std::vector<g_listener*>> listeners;

    ~g_component() override;

    template <typename COMPONENT_TYPE, g_ui_component_type COMPONENT_CONSTANT>
    static COMPONENT_TYPE* createComponent()
    {
        if (!g_ui_initialized)
            return nullptr;

        g_message_transaction tx = g_get_message_tx_id();
        g_ui_create_component_request request;
        request.header.id = G_UI_PROTOCOL_CREATE_COMPONENT;
        request.type = COMPONENT_CONSTANT;
        g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_create_component_request), tx);
	    // g_yield_t(g_ui_delegate_tid);

        size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_create_component_response);
        uint8_t buffer[bufferSize];

        COMPONENT_TYPE* component = nullptr;
        if (g_receive_message_t(buffer, bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
        {
            auto response = (g_ui_create_component_response*)G_MESSAGE_CONTENT(buffer);

            if (response->status == G_UI_PROTOCOL_SUCCESS)
            {
                component = attachComponent<COMPONENT_TYPE>(response->id);
            }
        }
        return component;
    }

    template <typename COMPONENT_TYPE>
    static COMPONENT_TYPE* attachComponent(g_ui_component_id id)
    {
        if (!g_ui_initialized)
            return nullptr;

        auto component = new COMPONENT_TYPE(id);
        g_component_registry::add(component);
        return component;
    }

public:
    explicit g_component(g_ui_component_id id) : id(id), g_bounding_component(this)
    {
    }

    g_ui_component_id getId() const
    {
        return id;
    }

    void destroy();
    bool addChild(g_component* c);

    bool setBounds(const g_rectangle& rect);
    g_rectangle getBounds();

    bool isVisible();
    bool setVisible(bool visible);
    bool setBackground(g_color_argb argb);

    bool setFocusable(bool focusable);
    bool isFocusable();
    bool setDispatchesFocus(bool d);
    bool isDispatchesFocus();

    bool setPreferredSize(g_dimension size);

    bool setNumericProperty(int property, uint32_t value);
    bool getNumericProperty(int property, uint32_t* out);

    bool addListener(g_ui_component_event_type eventType, g_listener* listener);
    bool addMouseListener(g_mouse_listener* listener);
    bool addMouseListener(g_mouse_listener_func listener);
    bool addVisibleListener(g_visible_listener* listener);
    bool addVisibleListener(g_visible_listener_func listener);

    bool setFlexOrientation(bool horizontal);
    bool setFlexComponentInfo(g_component* child, float grow, float shrink, int basis);
    bool setFlexPadding(g_insets padding);
    bool setFlexGap(int gap);

    void handle(g_ui_component_event_header* header);

    bool setLayout(g_ui_layout_manager layout);
};

#endif
