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

#include "generic_video_output.hpp"
#include <libvideo/videodriver.hpp>
#include <cstdio>
#include <immintrin.h>

bool g_generic_video_output::initializeWithSettings(uint32_t width, uint32_t height, uint32_t bits)
{
	int tries = 3;
	while(!videoDriverSetMode(driverTid, deviceId, width, height, bits, videoModeInformation))
	{
		klog("failed to initialize generic video... retrying");
		if(tries-- == 0)
			return false;
		g_sleep(1000);
	}
	return true;
}

void g_generic_video_output::blit(g_rectangle invalid, g_rectangle sourceSize, g_color_argb* source)
{
	uint16_t bpp = videoModeInformation.bpp;
	uint8_t* position = ((uint8_t*) videoModeInformation.lfb) + (invalid.y * videoModeInformation.bpsl);

	int right = invalid.x + invalid.width;
	int bottom = invalid.y + invalid.height;

	if(bpp == 32)
	{
		for(int y = invalid.y; y < bottom; y++)
		{
			auto sourceRow = &source[y * sourceSize.width];

			auto position4 = (uint32_t*) position;
			int x = invalid.x;
			for(; x < right - 4; x += 4)
			{
				__m128i data = _mm_loadu_si128((__m128i*) &sourceRow[x]);
				_mm_storeu_si128((__m128i*) &position4[x], data);
			}
			for(; x < right; x++)
			{
				position4[x] = sourceRow[x];
			}
			position += videoModeInformation.bpsl;
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
			position += videoModeInformation.bpsl;
		}
	}

	if(videoModeInformation.explicit_update)
	{
		videoDriverUpdate(driverTid, deviceId, invalid.x, invalid.y, invalid.width, invalid.height);
		g_yield();
	}
}


g_dimension g_generic_video_output::getResolution()
{
	return {videoModeInformation.resX, videoModeInformation.resY};
}
