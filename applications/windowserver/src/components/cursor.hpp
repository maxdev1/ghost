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

#ifndef __CURSOR__
#define __CURSOR__

#include <components/component.hpp>
#include <events/mouse_event.hpp>
#include <ghostuser/graphics/painter.hpp>
#include <ghostuser/graphics/metrics/point.hpp>
#include <ghostuser/graphics/images/image.hpp>

#include <string.h>
#include <cstdio>
#include <sstream>
#include <map>
#include <fstream>
/**
 *
 */
struct cursor_configuration {
	g_image image;
	g_point hitpoint;
};

/**
 *
 */
class cursor_t {
public:
	static g_point position;
	static g_point nextPosition;
	static mouse_button_t pressedButtons;
	static mouse_button_t nextPressedButtons;

	static component_t* draggedComponent;
	static component_t* hoveredComponent;
	static component_t* focusedComponent;

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
