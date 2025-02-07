/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __WINDOWSERVER_VIDEO_GRAPHICS__
#define __WINDOWSERVER_VIDEO_GRAPHICS__

#include <cairo/cairo.h>
#include <libwindow/color_argb.hpp>
#include <libwindow/metrics/dimension.hpp>
#include <libwindow/metrics/rectangle.hpp>
#include <stdint.h>
#include <ghost/mutex.h>

class graphics_t
{
    cairo_t* context = nullptr;
    cairo_surface_t* surface = nullptr;
    g_user_mutex lock = g_mutex_initialize_r(true);

    int contextRefCount = 0;

    int averageFactor = 10;

public:
    int width;
    int height;

    /**
     * Creates a graphics object. This is a class that holds a surface.
     * If an <code>externalBuffer</code> is provided, no internal buffer will be
     * automatically created.
     *
     * @param width of the externalBuffer
     * @param height of the externalBuffer
     */
    graphics_t(uint16_t width = 0, uint16_t height = 0);

    void resize(int width, int height, bool averaged = true, bool force = false);

    void setAverageFactor(int factor)
    {
        this->averageFactor = factor;
    }

    cairo_t* acquireContext();

    void releaseContext();

    cairo_surface_t* getSurface()
    {
        return surface;
    }

    void blitTo(graphics_t* target, const g_rectangle& clip, const g_point& position);
};

#endif
