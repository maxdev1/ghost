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

#ifndef __GENERIC_VIDEO_OUTPUT__
#define __GENERIC_VIDEO_OUTPUT__

#include "configuration_based_video_output.hpp"
#include <libvideo/videodriver.hpp>
#include <ghost/tasks/types.h>

class g_generic_video_output : public g_configuration_based_video_output
{
    g_video_mode_info videoModeInformation{};

    g_tid driverTid;
    g_device_id deviceId;

public:
    explicit g_generic_video_output(g_tid driverTid, g_device_id deviceId): driverTid(driverTid), deviceId(deviceId)
    {
    }

    bool initializeWithSettings(uint32_t width, uint32_t height, uint32_t bits) override;
    void blit(g_rectangle invalid, g_rectangle sourceSize, g_color_argb* source) override;
    g_dimension getResolution() override;
};

#endif
