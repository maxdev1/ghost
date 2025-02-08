/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __WINDOWSERVER_VIDEOOUTPUT__
#define __WINDOWSERVER_VIDEOOUTPUT__

#include <libwindow/color_argb.hpp>
#include <libwindow/metrics/dimension.hpp>
#include <libwindow/metrics/rectangle.hpp>

/**
 * The video output is the abstract interface that the window server uses to
 * initialize a video mode and put output on the screen.
 */
class g_video_output
{
public:
    virtual ~g_video_output() = default;

    /**
     * Initializes the video mode implementation.
     *
     * @return whether initialization was successful
     */
    virtual bool initialize() = 0;

    /**
     * Writes the invalid rectangle within the source image to the screen.
     *
     * @param invalid
     * 		rectangle that is invalid
     * @param sourceSize
     * 		absolute size of the source
     * @param source
     * 		source buffer
     */
    virtual void blit(g_rectangle invalid, g_rectangle sourceSize, g_color_argb* source) = 0;

    /**
     * Returns the initialized resolution.
     */
    virtual g_dimension getResolution() = 0;
};

#endif
