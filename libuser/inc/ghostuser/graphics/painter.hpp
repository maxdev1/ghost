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

#ifndef GHOSTLIBRARY_GRAPHICS_PAINTER
#define GHOSTLIBRARY_GRAPHICS_PAINTER

#include <ghostuser/graphics/graphics.hpp>
#include <ghostuser/graphics/images/image.hpp>
#include <ghostuser/graphics/metrics/dimension.hpp>
#include <ghostuser/graphics/metrics/rectangle.hpp>
#include <ghostuser/graphics/polygon.hpp>
#include <vector>
#include <string>

/**
 *
 */
class g_painter {
private:
	g_graphics& graphics;
	g_color_argb color;

public:
	g_painter(g_graphics& graphics) :
			graphics(graphics), color(RGB(0, 0, 0)) {
	}

	void setColor(g_color_argb newColor) {
		color = newColor;
	}

	g_color_argb getColor() {
		return color;
	}

	void fill(g_rectangle r);
	void draw(g_rectangle r);

	void fill(g_polygon& p);

	void drawLine(g_point a, g_point b);

	void blur(double intensity);

	void drawImage(int px, int py, g_image* image);
	void drawBitmap(int px, int py, g_color_argb* bitmap, int width, int height);
	void drawColoredBitmap(int px, int py, g_color_argb* bitmap, g_color_argb color, int width, int height);
};

#endif
