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

#ifndef __TEXT_FIELD__
#define __TEXT_FIELD__

#include "components/text/fonts/font.hpp"
#include "components/text/fonts/text_layouter.hpp"
#include "components/text/text_component.hpp"
#include "components/titled_component.hpp"
#include "input/keyboard/keyboard.hpp"

#include <libwindow/metrics/insets.hpp>
#include <list>
#include <string>

enum class text_field_visual_status_t : uint8_t
{
    NORMAL,
    HOVERED
};

class text_field_t : public text_component_t, public titled_component_t
{
  private:
    std::string text;
    text_field_visual_status_t visualStatus;
    bool focused;
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
    text_field_t();
    virtual ~text_field_t();

    /**
     *
     */
    virtual void update();

    /**
     *
     */
    virtual void paint();

    virtual bool handleKeyEvent(key_event_t& e);
    virtual bool handleMouseEvent(mouse_event_t& e);
    virtual bool handleFocusEvent(focus_event_t& e);

    /**
     *
     */
    virtual void setText(std::string text);

    /**
     *
     */
    virtual std::string getText()
    {
        return text;
    }

    /**
     *
     */
    virtual void setTitle(std::string title)
    {
        setText(title);
    }

    /**
     *
     */
    virtual std::string getTitle()
    {
        return getText();
    }

    /**
     *
     */
    virtual void setCursor(int pos);

    /**
     *
     */
    virtual int getCursor()
    {
        return cursor;
    }

    /**
     *
     */
    virtual void setMarker(int pos);

    /**
     *
     */
    virtual int getMarker()
    {
        return marker;
    }

    /**
     *
     */
    void backspace(g_key_info& info);

    /**
     *
     */
    void insert(std::string text);

    /**
     *
     */
    int viewToPosition(g_point p);

    /**
     *
     */
    g_rectangle glyphToView(g_positioned_glyph& g);

    /**
     *
     */
    int positionToUnscrolledCursorX(int pos);

    /**
     *
     */
    g_rectangle positionToCursorBounds(int pos);

    /**
     *
     */
    void setFont(g_font* f);

    /**
     *
     */
    virtual g_range getSelectedRange();

    /**
     *
     */
    virtual bool getNumericProperty(int property, uint32_t* out);

    /**
     *
     */
    virtual bool setNumericProperty(int property, uint32_t value);
};

#endif
