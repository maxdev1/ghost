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

#ifndef GHOSTLIBRARY_GRAPHICS_GRAPHICS
#define GHOSTLIBRARY_GRAPHICS_GRAPHICS

#include <ghostuser/graphics/color_argb.hpp>
#include <ghostuser/graphics/metrics/dimension.hpp>
#include <ghostuser/graphics/metrics/rectangle.hpp>
#include <stdint.h>

/**
 *
 */
class g_graphics {
private:
	int width;
	int height;
	g_color_argb* buffer;
	bool transparentBackground;
	bool hasExternalBuffer;

	inline g_color_argb add(g_color_argb a, g_color_argb b);

public:
	/**
	 * Creates a graphics object. This is a class that allows painting on a buffer.
	 * If an <code>externalBuffer</code> is provided, no internal buffer will be
	 * automatically created.
	 *
	 * @param transparentBackground whether the graphics buffer has a transparent background
	 *
	 * @param externalBuffer an optional external buffer to use instead of an internal one
	 * @param width of the externalBuffer
	 * @param height of the externalBuffer
	 */
	g_graphics(bool transparentBackground = false, g_color_argb* externalBuffer = nullptr, uint16_t width = 0, uint16_t height = 0);

	/**
	 *
	 */
	g_color_argb* getBuffer() {
		return buffer;
	}

	/**
	 *
	 */
	void resize(int width, int height);

	/**
	 *
	 */
	int getWidth() {
		return width;
	}

	/**
	 *
	 */
	int getHeight() {
		return height;
	}

	/**
	 * Adds the pixel
	 */
	void paintPixel(int x, int y, g_color_argb argb);

	/**
	 * Sets the pixel ignoring alphas
	 */
	void putPixel(int x, int y, g_color_argb argb);

	/**
	 * Returns the pixel
	 */
	g_color_argb getPixel(int x, int y);

	/**
	 *
	 */
	void clear();

	/**
	 *
	 */
	void blitTo(g_color_argb* out, const g_rectangle& outBounds, const g_rectangle& absoluteClip, const g_point& offset);
};

#endif
