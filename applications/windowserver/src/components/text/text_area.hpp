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

#ifndef __TEXT_AREA__
#define __TEXT_AREA__

#include "components/text/text_component.hpp"
#include "components/titled_component.hpp"

#include <libfont/font.hpp>
#include <libfont/text_layouter.hpp>
#include <libwindow/metrics/insets.hpp>
#include <libwindow/color_argb.hpp>
#include <string>

enum class text_area_visual_status_t : uint8_t
{
    NORMAL,
    HOVERED
};

class text_area_t : virtual public text_component_t, virtual public titled_component_t
{
    std::string text;
    text_area_visual_status_t visualStatus;
    bool secure;

    g_font* font;

    int scrollX;
    int fontSize;
    g_color_argb textColor;
    g_insets insets;

    int cursor;
    int marker;

    g_layouted_text* viewModel;

    bool shiftDown = false;

    void loadDefaultFont();
    void applyScroll();

public:
    text_area_t();
    ~text_area_t() override = default;

    void update() override;
    void paint() override;

    component_t* handleKeyEvent(key_event_t& e) override;
    component_t* handleMouseEvent(mouse_event_t& e) override;

    bool getNumericProperty(int property, uint32_t* out) override;
    bool setNumericProperty(int property, uint32_t value) override;

    void setText(std::string text) override;
    std::string getText() override;

    bool isFocusableDefault() const override
    {
        return true;
    }
    void setFocusedInternal(bool focused) override;

    virtual void setSecure(bool secure);

    virtual bool isSecure()
    {
        return secure;
    }

    void setTitleInternal(std::string title) override
    {
        setText(title);
    }

    std::string getTitle() override
    {
        return getText();
    }

    void setCursor(int pos) override;

    int getCursor() override
    {
        return cursor;
    }

    void setMarker(int pos) override;

    int getMarker() override
    {
        return marker;
    }

    g_range getSelectedRange() override;

    void backspace(g_key_info& info);

    void insert(std::string text);
    int viewToPosition(g_point p);
    g_rectangle glyphToView(g_positioned_glyph& g);
    g_point positionToUnscrolledCursorPoint(int pos);
    g_rectangle positionToCursorBounds(int pos);
    void setFont(g_font* f);
};

#endif
