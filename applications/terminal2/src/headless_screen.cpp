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
#include <ghostuser/utils/utils.hpp>
#include <ghost.h>
#include <stdio.h>

static uint32_t screenIdCounter = 0;

/**
 *
 */
headless_screen_t::headless_screen_t() {
	id = screenIdCounter++;
	output_current = (uint8_t*) VIDEO_MEMORY;

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
		output_current[off] = ' ';
		output_current[off + 1] = SC_COLOR(SC_BLACK, SC_WHITE);
	}
	offset = 0;
	lock = false;
}

/**
 *
 */
void headless_screen_t::updateVisualCursor() {

	if (cursorVisible) {
		int x = (offset % (SCREEN_WIDTH * 2)) / 2;
		int y = offset / (SCREEN_WIDTH * 2);

		g_atomic_lock(&lock);
		uint16_t position = (y * SCREEN_WIDTH) + x;

		g_utils::outportByte(0x3D4, 0x0F);
		g_utils::outportByte(0x3D5, (uint8_t) (position & 0xFF));
		g_utils::outportByte(0x3D4, 0x0E);
		g_utils::outportByte(0x3D5, (uint8_t) ((position >> 8) & 0xFF));
		lock = false;
	}
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

	updateVisualCursor();
}

/**
 *
 */
void headless_screen_t::writeChar(char c) {

	// Print to the screen
	g_atomic_lock(&lock);

	if (c == '\n') {
		offset += SCREEN_WIDTH * 2;
		offset -= offset % (SCREEN_WIDTH * 2);
	} else {
		output_current[offset++] = c;
		output_current[offset++] = (uint8_t) SC_COLOR(colorBackground, colorForeground);
	}

	lock = false;

	// Ensure valid offset
	normalize();
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

		memcpy(output_current, &output_current[SCREEN_WIDTH * 2], screenSize - lineBytes);

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
void headless_screen_t::setCursorVisible(bool visible) {

	cursorVisible = visible;

	if (visible) {
		updateVisualCursor();

	} else {
		g_atomic_lock(&lock);
		// effectively hides the cursor
		g_utils::outportByte(0x3D4, 0x0F);
		g_utils::outportByte(0x3D5, (uint8_t) (-1 & 0xFF));
		g_utils::outportByte(0x3D4, 0x0E);
		g_utils::outportByte(0x3D5, (uint8_t) ((-1 >> 8) & 0xFF));
		lock = false;
	}

}

/**
 *
 */
void headless_screen_t::setScrollAreaScreen() {
	scrollAreaScreen = true;
}

/**
 *
 */
void headless_screen_t::setScrollArea(int start, int end) {
	scrollAreaScreen = false;
	g_atomic_lock(&lock);
	scrollAreaStart = start > SCREEN_HEIGHT ? SCREEN_HEIGHT : start;
	scrollAreaEnd = end > SCREEN_HEIGHT ? SCREEN_HEIGHT : end;
	lock = false;
}

/**
 *
 */
void headless_screen_t::scroll(int value) {

	int scrollStart = scrollAreaScreen ? 0 : scrollAreaStart;
	int scrollEnd = scrollAreaScreen ? SCREEN_HEIGHT : scrollAreaEnd;

	g_atomic_lock(&lock);

	if (value > 0) {
		for (int i = scrollEnd - 2; i >= scrollStart; i--) {
			memcpy(&output_current[(i + value) * SCREEN_WIDTH * 2], &output_current[i * SCREEN_WIDTH * 2], SCREEN_WIDTH * 2);
		}
	} else {
		for (int i = scrollStart; i < scrollEnd; i++) {
			memcpy(&output_current[i * SCREEN_WIDTH * 2], &output_current[(i + value) * SCREEN_WIDTH * 2], SCREEN_WIDTH * 2);
		}
	}

	lock = false;
}

/**
 *
 */
g_key_info headless_screen_t::readInput() {
	return g_keyboard::readKey();
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

/**
 *
 */
int headless_screen_t::getWidth() {
	return SCREEN_WIDTH;
}

/**
 *
 */
int headless_screen_t::getHeight() {
	return SCREEN_HEIGHT;
}
