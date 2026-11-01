/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef DESKTOP_ICONS_HPP
#define DESKTOP_ICONS_HPP
#include <cairo/cairo.h>
#include <initializer_list>

// A light outline and two eyes stay legible at taskbar sizes.
inline void paintGhost(cairo_t* cr, double x, double y, double size)
{
	cairo_save(cr);
	cairo_translate(cr, x, y);
	cairo_scale(cr, size / 24., size / 24.);
	cairo_move_to(cr, 4, 21);
	cairo_line_to(cr, 4, 11);
	cairo_curve_to(cr, 4, 5, 7, 2, 12, 2);
	cairo_curve_to(cr, 17, 2, 20, 5, 20, 11);
	cairo_line_to(cr, 20, 21);
	cairo_curve_to(cr, 18, 19, 17.5, 19, 16, 21);
	cairo_curve_to(cr, 14.5, 23, 13.5, 23, 12, 21);
	cairo_curve_to(cr, 10.5, 19, 9.5, 19, 8, 21);
	cairo_curve_to(cr, 6.5, 23, 5.5, 23, 4, 21);
	cairo_close_path(cr);
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_set_line_width(cr, 1.6);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
	cairo_stroke(cr);
	for(double eye: {9., 15.})
	{
		cairo_new_sub_path(cr);
		cairo_arc(cr, eye, 11, 1.15, 0, 6.28318530718);
	}
	cairo_fill(cr);
	cairo_restore(cr);
}
#endif
