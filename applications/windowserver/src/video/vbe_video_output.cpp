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

#include "vbe_video_output.hpp"
#include <libvbedriver/vbedriver.hpp>
#include <stdio.h>

bool g_vbe_video_output::initializeWithSettings(uint32_t width, uint32_t height, uint32_t bits)
{
    while(!vbeDriverSetMode(width, height, bits, video_mode_information))
    {
        klog("failed to initialize video... retrying in 3 seconds");
        g_sleep(3000);
    }
    return true;
}

void g_vbe_video_output::blit(g_rectangle invalid, g_rectangle sourceSize, g_color_argb* source)
{

    uint16_t bpp = video_mode_information.bpp;
    uint8_t* position = ((uint8_t*) video_mode_information.lfb) + (invalid.y * video_mode_information.bpsl);

    uint32_t right = invalid.x + invalid.width;
    uint32_t bottom = invalid.y + invalid.height;

    if(bpp == 32)
    {
        for(int y = invalid.y; y < bottom; y++)
        {
            uint32_t* position4 = (uint32_t*) position;
            for(int x = invalid.x; x < right; x++)
            {
                position4[x] = source[y * sourceSize.width + x];
            }
            position += video_mode_information.bpsl;
        }
    }
    else if(bpp == 24)
    {
        for(int y = invalid.y; y < bottom; y++)
        {
            for(int x = invalid.x; x < right; x++)
            {
                g_color_argb color = source[y * sourceSize.width + x];
                position[x * 3] = color & 0xFF;
                position[x * 3 + 1] = (color >> 8) & 0xFF;
                position[x * 3 + 2] = (color >> 16) & 0xFF;
            }
            position += video_mode_information.bpsl;
        }
    }
}

g_dimension g_vbe_video_output::getResolution()
{
    return g_dimension(video_mode_information.resX, video_mode_information.resY);
}
