/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef LIBWINDOW_PROPERTIES
#define LIBWINDOW_PROPERTIES

/**
 * Properties may have a different meaning for each component. They are
 * used to simplify configuring components from a client application.
 */

#define G_UI_PROPERTY_MOVABLE			1
#define G_UI_PROPERTY_RESIZABLE			2
#define G_UI_PROPERTY_SECURE			3
#define G_UI_PROPERTY_ENABLED			4
#define G_UI_PROPERTY_LAYOUT_MANAGER	5
#define G_UI_PROPERTY_BACKGROUND        6
#define G_UI_PROPERTY_COLOR             7
#define G_UI_PROPERTY_ALIGNMENT         8

#endif
