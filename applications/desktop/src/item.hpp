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

#ifndef DESKTOP_ITEM
#define DESKTOP_ITEM

#include <libwindow/canvas.hpp>
#include <libwindow/label.hpp>
#include <cairo/cairo.h>

class item : public g_canvas
{
protected:
    cairo_surface_t* iconSurface = nullptr;
    g_label* label = nullptr;
    std::string application;
    explicit item(uint32_t id);

    void init(std::string name, std::string icon, std::string application);

public:
    bool hover = false;
    bool selected = false;
    uint64_t lastPress = 0;
    g_point dragOffset;

    static item* create(std::string name, std::string icon, std::string application);

    ~item() override = default;
    virtual void paint();
    void onDoubleClick();
};

#endif
