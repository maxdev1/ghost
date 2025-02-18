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

#ifndef DESKTOP_BACKGROUND
#define DESKTOP_BACKGROUND

#include <taskbar.hpp>

#include "item.hpp"
#include "item_organizer.hpp"

#include <libwindow/canvas.hpp>
#include <libwindow/selection.hpp>
#include <vector>

class background_t : virtual public g_canvas
{
protected:
    void init();

    taskbar_t* taskbar;
    std::vector<item_t*> items;
    void onMouseMove(const g_point& position);
    bool dragItems;
    g_point selectionStart;
    g_selection* selection = nullptr;
    item_organizer_t* organizer = nullptr;

    void onMouseLeftPress(const g_point& position, int clickCount);
    void onMouseDrag(const g_point& position);
    void onMouseRelease(const g_point& position);

public:
    explicit background_t(g_ui_component_id id):
        g_component(id),
        g_canvas(id), dragItems(false)
    {
    }

    ~background_t() override = default;
    static background_t* create();

    void addTaskbar(taskbar_t* taskbar);
    virtual void paint();

    void organize();
    void addItem(item_t* item);
};

#endif
