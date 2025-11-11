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

#ifndef DESKTOP_TASKBAR
#define DESKTOP_TASKBAR

#include <libfenster/components/canvas.hpp>
#include <libfenster/components/selection.hpp>
#include <libfenster/components/window.hpp>
#include <libfenster/font/text_layouter.hpp>
#include <vector>

using namespace fenster;

struct taskbar_entry_t
{
    Window* window;
    std::string title;
    bool focused;
    bool hovered;
    bool visible;
    Rectangle onView;
};

class taskbar_t : public Canvas
{
    g_user_mutex entriesLock = g_mutex_initialize_r(true);
    std::vector<taskbar_entry_t*> entries;
    LayoutedText* textLayoutBuffer;

protected:
    void init();

    void onMouseMove(const Point& position);

    void onMouseLeftPress(const Point& position, int clickCount);
    void onMouseDrag(const Point& position);
    void onMouseRelease(const Point& position);
    void onMouseLeave(const Point& position);

public:
    explicit taskbar_t(ComponentId id);

    ~taskbar_t() override = default;
    static taskbar_t* create();

    void handleDesktopEvent(WindowsEvent* event);

    virtual void paint();
};

#endif
