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

#ifndef CURSOR_HPP_
#define CURSOR_HPP_

#include <components/Component.hpp>
#include <events/MouseEvent.hpp>
#include <ghostuser/graphics/painter.hpp>
#include <ghostuser/graphics/metrics/Point.hpp>
#include <ghostuser/graphics/images/Image.hpp>

/**
 *
 */
struct CursorPack {
	g_image image;
	g_point hitpoint;
};

/**
 *
 */
class Cursor {
public:
	static g_point position;

	static MouseButton pressedButtons;
	static Component* draggedComponent;
	static Component* hoveredComponent;
	static Component* focusedComponent;

	/**
	 *
	 */
	static void paint(g_painter* global);

	/**
	 *
	 */
	static g_rectangle getArea();

	/**
	 *
	 */
	static void set(std::string name);

	/**
	 *
	 */
	static bool load(std::string cursorPath);

};

#endif
