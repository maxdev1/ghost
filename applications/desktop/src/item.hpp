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

#include <libfenster/components/canvas.hpp>
#include <libfenster/components/label.hpp>
#include <cairo/cairo.h>

using namespace fenster;

class item_t : virtual public Canvas
{
protected:
    cairo_surface_t* iconSurface = nullptr;
    Label* label = nullptr;
    std::string application;

    void init(std::string name, std::string icon, std::string application);

public:
    explicit item_t(uint32_t id);

    bool hover = false;
    bool selected = false;
    Point dragOffset;

    static item_t* create(std::string name, std::string icon, std::string application);

    ~item_t() override = default;
    virtual void paint();
    void onDoubleClick();
};

#endif
