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

#include <headless_screen.hpp>
#include <string.h>
#include <ghostuser/utils/Utils.hpp>
#include <ghost.h>

static uint8_t* output_video_direct = (uint8_t*) VIDEO_MEMORY;
static uint32_t screenIdCounter = 0;

/**
 *
 */
headless_screen_t::headless_screen_t() {
	id = screenIdCounter++;
	output_buffer = new uint8_t[SCREEN_WIDTH * SCREEN_HEIGHT * 2];
	output_current = output_buffer;

	offset = 0;
	activeProcessId = 0;
	lock = false;
	clean();
}

/**
 *
 */
void headless_screen_t::clean() {
	g_atomic_lock(&lock);
	for (uint32_t off = 0; off < SCREEN_HEIGHT * SCREEN_WIDTH * 2; off += 2) {
		output_buffer[off] = ' ';
		output_buffer[off + 1] = SC_COLOR(SC_BLACK, SC_WHITE);
	}
	offset = 0;
	lock = false;
}

/**
 *
 */
void headless_screen_t::activate() {
	g_atomic_lock(&lock);
	memcpy((uint8_t*) VIDEO_MEMORY, output_buffer,
	SCREEN_HEIGHT * SCREEN_WIDTH * 2);
	output_current = output_video_direct;
	lock = false;
}

/**
 *
 */
void headless_screen_t::deactivate() {
	g_atomic_lock(&lock);
	memcpy(output_buffer, (uint8_t*) VIDEO_MEMORY,
	SCREEN_HEIGHT * SCREEN_WIDTH * 2);
	output_current = output_buffer;
	lock = false;
}

/**
 *
 */
void headless_screen_t::moveVisualCursor(int x, int y) {
	g_atomic_lock(&lock);
	uint16_t position = (y * SCREEN_WIDTH) + x;

	g_utils::outportByte(0x3D4, 0x0F);
	g_utils::outportByte(0x3D5, (uint8_t) (position & 0xFF));
	g_utils::outportByte(0x3D4, 0x0E);
	g_utils::outportByte(0x3D5, (uint8_t) ((position >> 8) & 0xFF));
	lock = false;
}

/**
 *
 */
void headless_screen_t::moveCursor(int x, int y) {

	g_atomic_lock(&lock);

	offset = (y * SCREEN_WIDTH * 2) + x * 2;
	if (offset < 0) {
		offset = 0;
	} else if (offset > (SCREEN_WIDTH * 2 * SCREEN_HEIGHT)) {
		offset = SCREEN_WIDTH * 2 * SCREEN_HEIGHT;
	}

	lock = false;
	updateCursor();
}

/**
 *
 */
void headless_screen_t::updateCursor() {
	moveVisualCursor((offset % (SCREEN_WIDTH * 2)) / 2,
			offset / (SCREEN_WIDTH * 2));
}

/**
 *
 */
void headless_screen_t::writeChar(char c) {
	g_atomic_lock(&lock);
	if (c == '\n') {
		offset += SCREEN_WIDTH * 2;
		offset -= offset % (SCREEN_WIDTH * 2);
	} else {
		output_current[offset++] = c;
		output_current[offset++] = (uint8_t) SC_COLOR(colorBackground,
				colorForeground);
	}
	lock = false;

	normalize();
	updateCursor();
}

/**
 *
 */
void headless_screen_t::backspace() {
	g_atomic_lock(&lock);
	offset -= 2;
	output_current[offset++] = ' ';
	++offset; // keep color
	offset -= 2;
	lock = false;

	updateCursor();
}

/**
 *
 */
void headless_screen_t::normalize() {

	g_atomic_lock(&lock);

	if (offset >= SCREEN_WIDTH * SCREEN_HEIGHT * 2) {
		offset -= SCREEN_WIDTH * 2;

		uint32_t lineBytes = SCREEN_WIDTH * 2;
		uint32_t screenSize = SCREEN_HEIGHT * lineBytes;

		memcpy(output_current, &output_current[SCREEN_WIDTH * 2],
				screenSize - lineBytes);

		for (uint32_t i = 0; i < SCREEN_WIDTH * 2; i += 2) {
			output_current[screenSize - lineBytes + i] = ' ';
			output_current[screenSize - lineBytes + i + 1] = SC_BLACK;
		}
	}

	lock = false;
}

/**
 *
 */
g_key_info headless_screen_t::readInput(bool* cancelCondition) {
	return g_keyboard::readKey(cancelCondition);
}

/**
 *
 */
int headless_screen_t::getCursorX() {
	return (offset % (SCREEN_WIDTH * 2)) / 2;
}

/**
 *
 */
int headless_screen_t::getCursorY() {
	return offset / (SCREEN_WIDTH * 2);
}
