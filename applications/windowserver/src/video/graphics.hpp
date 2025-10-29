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
#include <libwindow/metrics/dimension.hpp>
#include <libwindow/metrics/rectangle.hpp>
#include "platform/platform.hpp"

/**
 * The graphics class is a utility that internally holds a cairo surface and has
 * the ability to resize it when required.
 */
class graphics_t
{
    cairo_t* context = nullptr;
    cairo_surface_t* surface = nullptr;
    SYS_MUTEX_T lock = platformInitializeMutex(true);

    int contextRefCount = 0;
    int averageFactor = 10;

public:
    int width;
    int height;

    explicit graphics_t(uint16_t width = 0, uint16_t height = 0);

    void resize(int width, int height, bool averaged = true);

    cairo_surface_t* getSurface() const
    {
        return surface;
    }

    void setAverageFactor(int factor)
    {
        this->averageFactor = factor;
    }

    cairo_t* acquireContext();

    void releaseContext();

    void blitTo(graphics_t* target, const g_rectangle& clip, const g_point& position);
};

#endif
