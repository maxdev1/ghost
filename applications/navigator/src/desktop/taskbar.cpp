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

#include "taskbar.hpp"

#include <algorithm>
#include <cairo/cairo.h>
#include <libfenster/font/font_loader.hpp>
#include "helper.hpp"
#include "desktop_icons.hpp"

using namespace fenster;

namespace
{
    struct Lock
    {
        SYS_MUTEX_T mutex;
        explicit Lock(SYS_MUTEX_T mutex) : mutex(mutex) { platformAcquireMutex(mutex); }
        ~Lock() { platformReleaseMutex(mutex); }
    };
}

taskbar_t::taskbar_t(ComponentId id) : Component(id), Canvas(id), FocusableComponent(id)
{
    textLayoutBuffer = TextLayouter::initializeLayout();
}

taskbar_t::~taskbar_t()
{
    entries.clear();
    TextLayouter::destroyLayout(textLayoutBuffer);
    platformDestroyMutex(entriesLock);
}

taskbar_t* taskbar_t::create()
{
    auto instance = createCanvasComponent<taskbar_t>();
    if(instance) instance->init();
    return instance;
}

void taskbar_t::init()
{
    font = FontLoader::getDefault();
    setBufferListener([this]() { paint(); });
    addMouseListener([this](ComponentMouseEvent* e)
    {
        if(e->type == MouseEventType::Move) onMouseMove(e->position);
        else if(e->type == MouseEventType::Press && (e->buttons & MouseButton::Button1))
            onMouseLeftPress(e->position, e->clickCount);
        else if(e->type == MouseEventType::Drag) onMouseDrag(e->position);
        else if(e->type == MouseEventType::Release && !(e->buttons & MouseButton::Button1))
            onMouseRelease(e->position);
        else if(e->type == MouseEventType::DragRelease)
        {
            { Lock guard(entriesLock); pressedWindow = -1; pressedStart = pressedTaskbar = false; }
            paint();
        }
        else if(e->type == MouseEventType::Leave) onMouseLeave(e->position);
    });
    setFocusable(false);
    setDispatchesFocus(false);
}

void taskbar_t::onMouseMove(const Point& position)
{
    bool changed = false;
    {
        Lock guard(entriesLock);
        pointerPosition = position;
        pointerInside = true;
        bool hoveredStart = startBounds.contains(position);
        changed |= hoveredStart != startHovered;
        startHovered = hoveredStart;
        for(auto& entry : entries)
        {
            bool hovered = entry->listed && entry->onView.contains(position);
            changed |= entry->hovered != hovered;
            entry->hovered = hovered;
        }
    }
    if(changed) paint();
}

void taskbar_t::onMouseLeftPress(const Point& position, int clickCount)
{
    {
        Lock guard(entriesLock);
        pressedWindow = -1;
        pressedTaskbar = true;
        pressedStart = startBounds.contains(position);
        for(auto& entry : entries)
            if(entry->listed && entry->onView.contains(position))
            {
                pressedWindow = entry->window->getId();
                break;
            }
    }
    onMouseMove(position);
    paint();
}

void taskbar_t::onMouseDrag(const Point& position)
{
    onMouseMove(position);
}

void taskbar_t::onMouseRelease(const Point& position)
{
    std::shared_ptr<Window> window;
    std::function<void(bool)> startAction;
    bool showStart = false;
    {
        Lock guard(entriesLock);
        if(pressedStart && startBounds.contains(position)) {
            startAction = startMenuHandler; showStart = !startOpen;
        } else if(pressedTaskbar && !pressedStart && pressedWindow == -1 && position.y >= 0 && position.y < startBounds.y + startBounds.height) {
            startAction = startMenuHandler;
        }
        for(auto& entry : entries)
            if(entry->listed && entry->window->getId() == pressedWindow && entry->onView.contains(position))
            {
                window = entry->window;
                break;
            }
        pressedWindow = -1;
        pressedStart = pressedTaskbar = false;
    }
    if(startAction) startAction(showStart);
    // Never hold the model lock across synchronous server calls or painting.
    if(window)
    {
        uint32_t visible = 0, focused = 0;
        if(window->getNumericProperty(ComponentProperty::Visible, &visible))
        {
            window->getNumericProperty(ComponentProperty::Focused, &focused);
            if(visible && focused) window->setVisible(false);
            else if(visible || window->setVisible(true)) window->setFocused(true);
        }
    }
    onMouseMove(position);
    paint();
}

void taskbar_t::onMouseLeave(const Point& position)
{
    bool changed = false;
    {
        Lock guard(entriesLock);
        pointerInside = false;
        changed = startHovered;
        startHovered = false;
        for(auto& entry : entries)
        {
            changed |= entry->hovered;
            entry->hovered = false;
        }
    }
    if(changed) paint();
}

void taskbar_t::paint()
{
    const auto bounds = getBounds();
    auto cr = acquireGraphics();
    if(!cr) return;
    const int width = std::min(bounds.width, int(currentBuffer.width));
    const int height = std::min(bounds.height, int(currentBuffer.height));
    if(width <= 0 || height <= 0) { releaseGraphics(); return; }
    std::vector<taskbar_entry_t> snapshot;
    ComponentId pressed;
    bool hoveredStart, openedStart, downStart;
    {
        Lock guard(entriesLock);
        pressed = pressedWindow;
        startBounds = {8, 5, 44, std::max(0, height - 10)};
        hoveredStart = startHovered = pointerInside && startBounds.contains(pointerPosition);
        openedStart = startOpen; downStart = pressedStart && hoveredStart;
        const auto count = std::count_if(entries.begin(), entries.end(), [](const auto& entry) { return entry->listed; });
        const int available = std::max(0, width - 80);
        const int total = std::min(available, int(std::min<size_t>(count, available)) * 200);
        int index = 0;
        for(auto& entry : entries)
        {
            entry->onView = {};
            if(entry->listed && count && height > 12 && available > 0)
            {
                int left = 64 + int(int64_t(index) * total / count);
                int right = 64 + int(int64_t(++index) * total / count);
                int gap = right - left >= 12 ? 6 : 0;
                if(right > left) entry->onView = {left + gap / 2, 5, right - left - gap, height - 10};
            }
            entry->hovered = pointerInside && entry->onView.contains(pointerPosition);
            if(entry->onView.width > 0) snapshot.push_back(*entry);
        }
    }
    auto gradient = cairo_pattern_create_linear(0, 0, 0, height);
    cairo_pattern_add_color_stop_rgb(gradient, 0, .085, .115, .18);
    cairo_pattern_add_color_stop_rgb(gradient, 1, .055, .075, .12);
    cairo_set_source(cr, gradient);
    cairo_paint(cr);
    cairo_pattern_destroy(gradient);
    cairo_set_source_rgba(cr, .55, .70, .95, .24);
    cairo_rectangle(cr, 0, 0, width, 1); cairo_fill(cr);

    if(hoveredStart || openedStart || downStart) {
        cairo_set_source_rgb(cr, downStart ? .21 : .14, downStart ? .32 : .22, downStart ? .47 : .34);
        cairo_rounded_rectangle(cr, 8, 5, 44, std::max(0, height - 10), 8); cairo_fill(cr);
    }
    paintGhost(cr, 18, height / 2. - 12, 24);
    auto text = [&](const std::string& title, Rectangle box, double alpha)
    {
        if(!font || box.width <= 0 || box.height <= 0) return;
        cairo_save(cr);
        cairo_rectangle(cr, box.x, box.y, box.width, box.height); cairo_clip(cr);
        TextLayouter::layout(cr, title.c_str(), font, 13, {0, 0, box.width, box.height},
                             TextAlignment::LEFT, textLayoutBuffer, false);
        for(auto& glyph : textLayoutBuffer->positions)
        {
            if(glyph.line != 0 || glyph.controlChar != -1) continue;
            cairo_save(cr);
            cairo_set_source_rgba(cr, .90, .94, 1, alpha);
            TextLayouter::paintChar(cr, glyph, {box.x, box.y + (box.height - 17) / 2});
            cairo_restore(cr);
        }
        cairo_restore(cr);
    };
    for(const auto& entry : snapshot)
    {
        auto box = entry.onView;
        const bool active = entry.focused && entry.visible;
        const bool down = entry.hovered && entry.window->getId() == pressed;
        if(active) cairo_set_source_rgb(cr, .17, .26, .40);
        else if(entry.hovered) cairo_set_source_rgb(cr, .14, .20, .30);
        else cairo_set_source_rgba(cr, .30, .40, .56, .10);
        if(down) cairo_set_source_rgb(cr, .21, .32, .47);
        cairo_rounded_rectangle(cr, box.x, box.y, box.width, box.height, std::min(7., box.width / 2.));
        cairo_fill(cr);
        if(box.width >= 18)
        {
            cairo_set_source_rgba(cr, .46, .78, 1, active ? 1 : entry.visible ? .6 : .28);
            cairo_rounded_rectangle(cr, box.x + 9, box.y + box.height / 2 - 3, 6, 6, 2); cairo_fill(cr);
            auto title = entry.title.empty() ? std::string("Window") : entry.title;
            std::replace(title.begin(), title.end(), '\n', ' ');
            std::replace(title.begin(), title.end(), '\r', ' ');
            text(title, {box.x + 24, box.y, box.width - 32, box.height}, entry.visible ? 1 : .60);
        }
        if(active && box.width >= 24)
        {
            cairo_set_source_rgb(cr, .40, .78, .98);
            cairo_rounded_rectangle(cr, box.x + (box.width - 24) / 2, height - 4, 24, 2, 1); cairo_fill(cr);
        }
    }
    blit({0, 0, width, height});
    releaseGraphics();
}

void taskbar_t::setStartMenuHandler(std::function<void(bool)> handler)
{
    Lock guard(entriesLock); startMenuHandler = std::move(handler);
}

void taskbar_t::setStartMenuOpen(bool open)
{
    { Lock guard(entriesLock); startOpen = open; }
    paint();
}

void taskbar_t::handleDesktopEvent(WindowsEvent* event)
{
    std::shared_ptr<taskbar_entry_t> entry;
    {
        Lock guard(entriesLock);
        auto found = std::find_if(entries.begin(), entries.end(), [&](const auto& item) {
            return item->window->getId() == event->window_id;
        });
        if(found != entries.end()) entry = *found;
        if(!event->present && entry)
        {
            entry->present = false;
            if(pressedWindow == event->window_id) pressedWindow = -1;
            entries.erase(found);
        }
    }
    if(!event->present) { paint(); return; }
    if(!entry)
    {
        auto window = Window::attach(event->window_id);
        if(!window) return;
        entry = std::make_shared<taskbar_entry_t>();
        entry->window = std::shared_ptr<Window>(window, [](Window* window) { window->disposeLocal(); });
        { Lock guard(entriesLock); entries.push_back(entry); }
        std::weak_ptr<taskbar_entry_t> weak = entry;
        window->addTitleListener([this, weak](std::string title) {
            auto entry = weak.lock(); if(!entry) return;
            { Lock guard(entriesLock); if(!entry->present) return; entry->title = std::move(title); }
            paint();
        });
        window->addFocusListener([this, weak](bool focused) {
            auto entry = weak.lock(); if(!entry) return;
            { Lock guard(entriesLock); if(!entry->present) return; entry->focused = focused; }
            paint();
        });
        window->addVisibleListener([this, weak](bool visible) {
            auto entry = weak.lock(); if(!entry) return;
            { Lock guard(entriesLock); if(!entry->present) return; entry->visible = visible; }
            paint();
        });
        auto title = window->getTitle();
        uint32_t visible = 0, focused = 0;
        window->getNumericProperty(ComponentProperty::Visible, &visible);
        window->getNumericProperty(ComponentProperty::Focused, &focused);
        {
            Lock guard(entriesLock);
            entry->title = std::move(title); entry->visible = visible; entry->focused = focused;
        }
    }
    uint32_t listed = 0;
    entry->window->getNumericProperty(ComponentProperty::ShowInTaskbar, &listed);
    { Lock guard(entriesLock); entry->listed = listed != 0; }
    paint();
}
