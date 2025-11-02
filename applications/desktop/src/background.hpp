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

#include <libfenster/canvas.hpp>
#include <libfenster/selection.hpp>
#include <vector>

using namespace fenster;

class background_t : virtual public Canvas
{
protected:
    void init();

    taskbar_t* taskbar;
    std::vector<item_t*> items;
    void onMouseMove(const Point& position);
    bool dragItems;
    Point selectionStart;
    Selection* selection = nullptr;
    item_organizer_t* organizer = nullptr;

    void onMouseLeftPress(const Point& position, int clickCount);
    void onMouseDrag(const Point& position);
    void onMouseRelease(const Point& position);

public:
    explicit background_t(ComponentId id):
        Component(id),
        Canvas(id), dragItems(false),
        FocusableComponent(id)
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
