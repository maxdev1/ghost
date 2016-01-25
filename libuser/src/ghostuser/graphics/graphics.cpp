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

#include <ghostuser/graphics/graphics.hpp>
#include <string.h>
#include <malloc.h>

#define DEBUG_DIRTY false

/**
 *
 */
g_graphics::g_graphics(bool transparentBackground, g_color_argb* externalBuffer, uint16_t width, uint16_t height) :
		transparentBackground(transparentBackground), buffer(externalBuffer), width(width), height(height) {

	hasExternalBuffer = (externalBuffer != nullptr);
	resize(0, 0);
}

/**
 *
 */
g_color_argb g_graphics::add(g_color_argb a, g_color_argb b) {

	uint8_t firstA = (a >> 24) & 0xFF;
	uint8_t firstR = (a >> 16) & 0xFF;
	uint8_t firstG = (a >> 8) & 0xFF;
	uint8_t firstB = a & 0xFF;

	uint8_t secondA = (b >> 24) & 0xFF;
	uint8_t secondR = (b >> 16) & 0xFF;
	uint8_t secondG = (b >> 8) & 0xFF;
	uint8_t secondB = b & 0xFF;

	float secondStrength = ((float) secondA) / 255.0f;
	uint8_t resultA = (uint8_t) (firstA + (secondA * (255 - firstA) / 255));
	uint8_t resultR = (uint8_t) (secondStrength * secondR + (1.0f - secondStrength) * (float) firstR);
	uint8_t resultG = (uint8_t) (secondStrength * secondG + (1.0f - secondStrength) * (float) firstG);
	uint8_t resultB = (uint8_t) (secondStrength * secondB + (1.0f - secondStrength) * (float) firstB);

	return (resultA << 24) | (resultR << 16) | (resultG << 8) | (resultB);
}

/**
 *
 */
void g_graphics::resize(int newWidth, int newHeight) {

	// when there is an external buffer, don't do this
	if (hasExternalBuffer) {
		return;
	}

	if (newWidth < 0 || newHeight < 0) {
		return;
	}

	// create new buffer
	g_color_argb* newBuffer = new g_color_argb[newWidth * newHeight];

	if (buffer) {
		delete buffer;
	}

	// set new values
	width = newWidth;
	height = newHeight;
	buffer = newBuffer;
}

/**
 *
 */
void g_graphics::clear() {
	for (int p = 0; p < width * height; p++) {
		if (transparentBackground) {
			buffer[p] = 0;
		} else {
			buffer[p] = RGB(255, 255, 255);
		}
	}
}

/**
 *
 */
void g_graphics::paintPixel(int x, int y, g_color_argb argb) {

	if (x >= 0 && y >= 0 && x < width && y < height) {
		buffer[y * width + x] = add(buffer[y * width + x], argb);
	}
}

/**
 *
 */
void g_graphics::putPixel(int x, int y, g_color_argb argb) {
	if (x >= 0 && y >= 0 && x < width && y < height) {
		buffer[y * width + x] = argb;
	}
}

/**
 *
 */
g_color_argb g_graphics::getPixel(int x, int y) {

	if (x >= 0 && y >= 0 && x < width && y < height) {
		return buffer[y * width + x];
	}
	return ARGB(0, 0, 0, 0);
}

/**
 *
 */
void g_graphics::blitTo(g_color_argb* out, const g_rectangle& outBounds, const g_rectangle& absoluteClip, const g_point& offset) {

	int absX;
	int absY;

	for (int y = 0; y < height; y++) {
		absY = (y + offset.y);

		for (int x = 0; x < width; x++) {
			absX = (x + offset.x);

			if (absX >= absoluteClip.x && absY >= absoluteClip.y && absX < absoluteClip.x + absoluteClip.width && absY < absoluteClip.y + absoluteClip.height) {
				uint32_t absOff = absY * outBounds.width + absX;

				if (transparentBackground) {
					out[absOff] = add(out[absOff], buffer[y * width + x]);
				} else {
					out[absOff] = buffer[y * width + x];
				}
			}
		}
	}
}
