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

#ifndef LIBWINDOW_CANVAS
#define LIBWINDOW_CANVAS

#include "libwindow/component.hpp"
#include "libwindow/listener/canvas_buffer_listener.hpp"

#include <cstdint>
#include <utility>
#include <cairo/cairo.h>

struct g_canvas_buffer_info
{
    uint8_t* buffer;
    uint16_t width;
    uint16_t height;

    cairo_surface_t* surface;
    cairo_t* context;
    int contextRefCount;
};

/**
 * A canvas is a simple component that offers a buffer to the creator for arbitrary painting. This buffer automatically
 * resizes when the bounds of the component change, so it is crucial to always keep track of which buffer to paint to.
 */
class g_canvas : public g_component
{
protected:
    g_canvas_buffer_info currentBuffer;
    g_user_mutex currentBufferLock;

    /**
     * Listener only for user purpose, so a client gets an event once the
     * buffer was changed.
     */
    g_canvas_buffer_listener* userListener;

    explicit g_canvas(uint32_t id);

public:
    static g_canvas* create();

    void acknowledgeNewBuffer(g_address address, uint16_t width, uint16_t height);

    void blit(const g_rectangle& rect);

    /**
     * Acquires the current buffer and lends it to the user for painting. The buffer must be released afterwards.
     */
    cairo_t* acquireGraphics();

    /**
     * Releases the current buffer.
     */
    void releaseGraphics();

    void setBufferListener(g_canvas_buffer_listener* l)
    {
        userListener = l;
    }

    void setBufferListener(g_canvas_buffer_listener_func func)
    {
        userListener = new g_canvas_buffer_listener_dispatcher(std::move(func));
    }
};

#endif
