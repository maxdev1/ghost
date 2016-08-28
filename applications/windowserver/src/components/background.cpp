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

#include <components/background.hpp>

cairo_surface_t* surface;
/**
 *
 */
background_t::background_t(g_rectangle bounds) {
	setBounds(bounds);

	surface = cairo_image_surface_create_from_png("/system/graphics/wallpaper.png");
}

/**
 *
 */
void background_t::paint() {

	cairo_t* cr = graphics.getContext();
	auto bounds = getBounds();
	cairo_set_source_surface(cr, surface, bounds.x, bounds.y);
	cairo_rectangle(cr, bounds.x, bounds.y, bounds.width, bounds.height);
	cairo_fill(cr);
}

/**
 *
 */
bool background_t::handle(event_t& e) {
	return true;
}
