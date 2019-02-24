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

#include <ghost.h>
#include <ghostuser/io/mouse.hpp>
#include <ghostuser/io/ps2.hpp>
#include <ghostuser/io/ps2_driver_constants.hpp>
#include <ghostuser/utils/logger.hpp>

/**
 *
 */
g_mouse_info g_mouse::readMouse() {

	if (!g_ps2_is_registered) {
		if (!g_ps2::registerSelf()) {
			return g_mouse_info();
		}
	}

	// wait until incoming data is here (and the driver unsets the atom)
	g_atomic_block(&g_ps2_area->mouse.buffer_empty_lock);

	g_mouse_info e;
	e.x = g_ps2_area->mouse.move_x;
	e.y = g_ps2_area->mouse.move_y;
	e.button1 = (g_ps2_area->mouse.flags & (1 << 0));
	e.button2 = (g_ps2_area->mouse.flags & (1 << 1));
	e.button3 = (g_ps2_area->mouse.flags & (1 << 2));

	g_ps2_area->mouse.move_x = 0;
	g_ps2_area->mouse.move_y = 0;

	g_ps2_area->mouse.buffer_empty_lock = true;

	return e;
}
