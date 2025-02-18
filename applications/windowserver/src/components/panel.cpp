/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "components/panel.hpp"
#include <libwindow/properties.hpp>

void panel_t::paint()
{
	if(!hasGraphics())
		return;

	auto cr = graphics.acquireContext();
	if(!cr)
		return;

	cairo_set_source_rgba(cr, ARGB_FR_FROM(background), ARGB_FG_FROM(background), ARGB_FB_FROM(background),
	                      ARGB_FA_FROM(background));
	cairo_rectangle(cr, 0, 0, getBounds().width, getBounds().height);
	cairo_fill(cr);

	graphics.releaseContext();
}

void panel_t::setBackground(g_color_argb color)
{
	background = color;
	markFor(COMPONENT_REQUIREMENT_PAINT);
}

g_color_argb panel_t::getBackground()
{
	return background;
}

bool panel_t::getNumericProperty(int property, uint32_t* out)
{
	if(property == G_UI_PROPERTY_BACKGROUND)
	{
		*out = background;
		return true;
	}

	return component_t::getNumericProperty(property, out);
}

bool panel_t::setNumericProperty(int property, uint32_t value)
{
	if(property == G_UI_PROPERTY_BACKGROUND)
	{
		setBackground(value);
		return true;
	}
	return component_t::setNumericProperty(property, value);
}
