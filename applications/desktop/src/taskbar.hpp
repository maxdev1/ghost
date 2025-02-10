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

#include <libwindow/canvas.hpp>
#include <libwindow/selection.hpp>
#include <libwindow/window.hpp>
#include <libfont/text_layouter.hpp>
#include <vector>

struct taskbar_entry
{
    g_window* window;
    std::string title;
    bool focused;
    bool hovered;
    g_rectangle onView;
};

class taskbar : public g_canvas
{
    g_user_mutex entriesLock = g_mutex_initialize_r(true);
    std::vector<taskbar_entry*> entries;
    g_layouted_text* textLayoutBuffer;

protected:
    void init();

    void onMouseMove(const g_point& position);

    void onMouseLeftPress(const g_point& position, int clickCount);
    void onMouseDrag(const g_point& position);
    void onMouseRelease(const g_point& position);
    void onMouseLeave(const g_point& position);

public:
    explicit taskbar(g_ui_component_id id);

    ~taskbar() override = default;
    static taskbar* create();

    void handleDesktopEvent(g_ui_windows_event* event);

    virtual void paint();
};

#endif
