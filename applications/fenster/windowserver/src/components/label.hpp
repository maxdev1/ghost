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

#ifndef __WINDOWSERVER_COMPONENTS_LABEL__
#define __WINDOWSERVER_COMPONENTS_LABEL__

#include "components/component.hpp"
#include "components/titled_component.hpp"

#include <libwindow/font/font.hpp>
#include <libwindow/font/text_alignment.hpp>
#include <libwindow/color_argb.hpp>

class label_t : virtual public component_t, virtual public titled_component_t
{
    g_font* font;
    int fontSize;
    cairo_text_extents_t textExtents;
    cairo_font_extents_t fontExtents;

    std::string text;
    g_text_alignment alignment;
    g_color_argb color;

public:
    label_t();
    ~label_t() override = default;

    void paint() override;
    void update() override;

    virtual void setFont(g_font* font);

    virtual void setColor(g_color_argb color);
    virtual g_color_argb getColor();

    void setTitleInternal(std::string title) override;
    std::string getTitle() override;

    void setAlignment(g_text_alignment alignment);
    g_text_alignment getAlignment();

    bool setNumericProperty(int property, uint32_t value) override;
};

#endif
