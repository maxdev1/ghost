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

#include "tester.hpp"
#include <ghost.h>
#include <assert.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <libvbedriver/vbedriver.hpp>
#include <libps2driver/ps2driver.hpp>

struct g_rectangle
{
public:
	int x;
	int y;
	int width;
	int height;
};

typedef uint32_t g_color_argb;

g_vbe_mode_info video_mode_information;

/**
 *
 */
void blit(g_rectangle invalid, g_rectangle sourceSize, g_color_argb *source)
{

	uint16_t bpp = video_mode_information.bpp;
	uint8_t *position = ((uint8_t *)video_mode_information.lfb) + (invalid.y * video_mode_information.bpsl);

	uint32_t right = invalid.x + invalid.width;
	uint32_t bottom = invalid.y + invalid.height;

	if (bpp == 32)
	{
		for (int y = invalid.y; y < bottom; y++)
		{
			uint32_t *position4 = (uint32_t *)position;
			for (int x = invalid.x; x < right; x++)
			{
				position4[x] = source[y * sourceSize.width + x];
			}
			position += video_mode_information.bpsl;
		}
	}
	else if (bpp == 24)
	{
		for (int y = invalid.y; y < bottom; y++)
		{
			for (int x = invalid.x; x < right; x++)
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

g_rectangle lastMousePos;
g_rectangle mousePos;
int32_t cursorX = 100;
int32_t cursorY = 100;

uint8_t drawing = 0;
int mousepacks = 0;

uint8_t waitingForData = 1;
char lastChar = 0;

static g_fd keyboardRead = 0;
static g_fd mouseRead = 0;

void keyboardReceiverThread()
{

	uint8_t val;
	for (;;)
	{
		if (g_read(keyboardRead, &val, 1) == 1)
		{
			lastChar = val;
			waitingForData = 0;
		}
	}
}

void mouseReceiverThread()
{

	g_ps2_mouse_packet packet;

	for (;;)
	{
		if (g_read(mouseRead, &packet, sizeof(g_ps2_mouse_packet)) == sizeof(g_ps2_mouse_packet))
		{
			mousepacks++;
			cursorX += packet.x;
			cursorY += packet.y;

			if (cursorX > video_mode_information.resX - 10)
			{
				cursorX = video_mode_information.resX - 10;
			}
			if (cursorX < 0)
			{
				cursorX = 0;
			}
			if (cursorY > video_mode_information.resY - 10)
			{
				cursorY = video_mode_information.resY - 10;
			}
			if (cursorY < 0)
			{
				cursorY = 0;
			}
			waitingForData = 0;
		}
	}
}

int main(int argc, char **argv)
{
	// Get PS2 input from PS2 driver
	klog("registering at PS2 driver...");
	if (!ps2DriverInitialize(&keyboardRead, &mouseRead))
	{
		klog("failed to register at PS2 driver");
		return -1;
	}
	klog("init ps2 threads... %i, %i", keyboardRead, mouseRead);
	g_create_thread((void *)keyboardReceiverThread);
	g_create_thread((void *)mouseReceiverThread);
	klog("done");

	// Init VBE
	klog("calling video driver to set mode...");
	vbeDriverSetMode(1024, 768, 32, video_mode_information);

	while (video_mode_information.bpp == 0)
	{
		klog("failed to initialize video... retrying in 3 seconds");
		g_sleep(3000);
		vbeDriverSetMode(1024, 768, 32, video_mode_information);
	}
	klog("video mode set: %ix%i@%i, lfb: %x", video_mode_information.resX, video_mode_information.resY, video_mode_information.bpp, video_mode_information.lfb);

	g_color_argb *source = (g_color_argb *)malloc(1024 * 768 * 4);


	for (uint16_t y = 0; y < video_mode_information.resY; y++)
	{
		for (uint16_t x = 0; x < video_mode_information.resX; x++)
		{
			source[y * video_mode_information.resX + x] = 0xFFFFFFFF;
		}
	}

	g_rectangle sourceSize;
	sourceSize.x = 0;
	sourceSize.y = 0;
	sourceSize.width = video_mode_information.resX;
	sourceSize.height = video_mode_information.resY;
	blit(sourceSize, sourceSize, source);

	uint8_t blitlock = 0;
	for (;;)
	{
		g_atomic_lock(&blitlock);
		mousePos.x = cursorX;
		mousePos.y = cursorY;
		mousePos.width = 10;
		mousePos.height = 10;

		for (uint16_t y = lastMousePos.y; y < lastMousePos.y + 10; y++)
		{
			for (uint16_t x = lastMousePos.x; x < lastMousePos.x + 10; x++)
			{
				if (y < sourceSize.height && x < sourceSize.width)
				{
					source[y * video_mode_information.resX + x] = 0xFFFFFFFF;
				}
			}
		}
		for (uint16_t y = mousePos.y; y < mousePos.y + 10; y++)
		{
			for (uint16_t x = mousePos.x; x < mousePos.x + 10; x++)
			{
				if (y < sourceSize.height && x < sourceSize.width)
				{
					source[y * video_mode_information.resX + x] = 0;
				}
			}
		}

		blit(lastMousePos, sourceSize, source);
		blit(mousePos, sourceSize, source);

		for (uint16_t y = 50; y < 100; y++)
		{
			for (uint16_t x = 50; x < 50 + 255 * 2; x++)
			{
				source[y * video_mode_information.resX + x] = 0xFFFFFFFF;
			}
		}
		for (uint16_t y = 50; y < 100; y++)
		{
			for (uint16_t x = 50; x < 50 + lastChar * 2; x++)
			{
				source[y * video_mode_information.resX + x] = 0;
			}
		}
		g_rectangle barSize;
		barSize.x = 50;
		barSize.y = 50;
		barSize.width = 255 * 2;
		barSize.height = 50;
		blit(barSize, sourceSize, source);

		blitlock = 0;

		lastMousePos = mousePos;

		g_atomic_block(&waitingForData);
		waitingForData = 1;
	}
}
