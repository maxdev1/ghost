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

#include "headless_screen.hpp"
#include <ghost.h>
#include <ghost/io.h>
#include <libps2driver/ps2driver.hpp>
#include <string.h>

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define VIDEO_MEMORY 0xB8000

static uint8_t* output = (uint8_t*) VIDEO_MEMORY;
static g_fd keyboardIn;
static g_fd mouseIn;

headless_screen_t::headless_screen_t()
{
	offset = 0;
}

bool headless_screen_t::initialize()
{
	lock = g_atomic_initialize();

	enableCursor();
	clean();

	return ps2DriverInitialize(&keyboardIn, &mouseIn);
}

void headless_screen_t::clean()
{
	g_atomic_lock(lock);
	for(uint32_t off = 0; off < SCREEN_HEIGHT * SCREEN_WIDTH * 2; off += 2)
	{
		output[off] = ' ';
		output[off + 1] = SC_COLOR(SC_BLACK, SC_WHITE);
	}
	offset = 0;
	g_atomic_unlock(lock);
}

void headless_screen_t::enableCursor()
{
	ioOutportByte(0x3D4, 0x0A);
	ioOutportByte(0x3D5, (ioInportByte(0x3D5) & 0xC0) | 0);
	ioOutportByte(0x3D4, 0x0B);
	ioOutportByte(0x3D5, (ioInportByte(0x3D5) & 0xE0) | SCREEN_HEIGHT);
}

void headless_screen_t::updateCursor()
{
	g_atomic_lock(lock);

	uint16_t position = (getCursorY() * SCREEN_WIDTH) + getCursorX();
	ioOutportByte(0x3D4, 0x0F);
	ioOutportByte(0x3D5, (uint8_t) (position & 0xFF));
	ioOutportByte(0x3D4, 0x0E);
	ioOutportByte(0x3D5, (uint8_t) ((position >> 8) & 0xFF));

	g_atomic_unlock(lock);
}

void headless_screen_t::setCursor(int x, int y)
{
	g_atomic_lock(lock);

	offset = (y * SCREEN_WIDTH * 2) + x * 2;
	if(offset < 0)
	{
		offset = 0;
	}
	else if(offset > (SCREEN_WIDTH * 2 * SCREEN_HEIGHT))
	{
		offset = SCREEN_WIDTH * 2 * SCREEN_HEIGHT;
	}

	g_atomic_unlock(lock);
	updateCursor();
}

void headless_screen_t::write(char c)
{
	g_atomic_lock(lock);
	if(c == '\n')
	{
		offset += SCREEN_WIDTH * 2;
		offset -= offset % (SCREEN_WIDTH * 2);
	}
	else
	{
		output[offset++] = c;
		output[offset++] = (uint8_t) SC_COLOR(colorBackground,
											  colorForeground);
	}
	normalize();
	g_atomic_unlock(lock);

	updateCursor();
}

void headless_screen_t::backspace()
{
	g_atomic_lock(lock);
	offset -= 2;
	output[offset++] = ' ';
	++offset; // keep color
	offset -= 2;
	g_atomic_unlock(lock);

	updateCursor();
}

void headless_screen_t::normalize()
{
	if(offset >= SCREEN_WIDTH * SCREEN_HEIGHT * 2)
	{
		offset -= SCREEN_WIDTH * 2;

		uint32_t lineBytes = SCREEN_WIDTH * 2;
		uint32_t screenSize = SCREEN_HEIGHT * lineBytes;

		memcpy(output, &output[SCREEN_WIDTH * 2],
			   screenSize - lineBytes);

		for(uint32_t i = 0; i < SCREEN_WIDTH * 2; i += 2)
		{
			output[screenSize - lineBytes + i] = ' ';
			output[screenSize - lineBytes + i + 1] = SC_COLOR(SC_BLACK, SC_WHITE);
		}
	}
}

g_key_info headless_screen_t::readInput()
{
	return g_keyboard::readKey(keyboardIn);
}

int headless_screen_t::getCursorX()
{
	return (offset % (SCREEN_WIDTH * 2)) / 2;
}

int headless_screen_t::getCursorY()
{
	return offset / (SCREEN_WIDTH * 2);
}

int headless_screen_t::getWidth()
{
	return SCREEN_WIDTH;
}

int headless_screen_t::getHeight()
{
	return SCREEN_HEIGHT;
}
