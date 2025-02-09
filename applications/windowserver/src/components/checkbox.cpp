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

#include "components/checkbox.hpp"
#include "events/mouse_event.hpp"

checkbox_t::checkbox_t() :
	checked(false), boxSize(DEFAULT_BOX_SIZE), boxTextGap(DEFAULT_BOX_TEXT_GAP), hovered(false), pressed(false)
{
	addChild(&label, COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL);
}

void checkbox_t::handleBoundChanged(const g_rectangle& oldBounds)
{
	g_rectangle unpositioned = getBounds();
	unpositioned.x = boxSize + boxTextGap;
	unpositioned.y = 0;
	this->label.setBounds(unpositioned);
}

void checkbox_t::layout()
{
	g_dimension preferredSize = label.getPreferredSize();
	if(preferredSize.height < boxSize + boxTextGap)
	{
		preferredSize.height = boxSize + boxTextGap;
	}
	preferredSize.width += preferredSize.height;
	setPreferredSize(preferredSize);
}

void checkbox_t::paint()
{
	auto cr = graphics.acquireContext();
	if(!cr)
		return;

	auto background = (pressed ? RGB(240, 240, 240) : (hovered ? RGB(245, 245, 255) : RGB(255, 255, 255)));
	cairo_set_source_rgba(cr, G_COLOR_ARGB_TO_FPARAMS(background));
	cairo_rectangle(cr, 0, 0, boxSize, boxSize);
	cairo_fill(cr);

	auto border = ((hovered || pressed) ? RGB(140, 140, 150) : RGB(160, 160, 170));
	cairo_set_source_rgba(cr, G_COLOR_ARGB_TO_FPARAMS(border));
	cairo_rectangle(cr, 0.5, 0.5, boxSize, boxSize);
	cairo_set_line_width(cr, 1.0);
	cairo_stroke(cr);

	if(checked)
	{
		int pad = 3;
		cairo_set_source_rgba(cr, G_COLOR_ARGB_TO_FPARAMS(RGB(25, 125, 255)));
		cairo_rectangle(cr, 1 + pad, 1.5 + pad, boxSize - pad * 2 - 1, boxSize - pad * 2 - 1.5);
		cairo_fill(cr);
	}

	graphics.releaseContext();
}

component_t* checkbox_t::handleMouseEvent(mouse_event_t& me)
{
	if(me.type == G_MOUSE_EVENT_ENTER)
	{
		hovered = true;
		markFor(COMPONENT_REQUIREMENT_PAINT);
	}
	else if(me.type == G_MOUSE_EVENT_LEAVE)
	{
		hovered = false;
		markFor(COMPONENT_REQUIREMENT_PAINT);
	}
	else if(me.type == G_MOUSE_EVENT_PRESS)
	{
		pressed = true;
		markFor(COMPONENT_REQUIREMENT_PAINT);
	}
	else if(me.type == G_MOUSE_EVENT_RELEASE || me.type == G_MOUSE_EVENT_DRAG_RELEASE)
	{
		pressed = false;

		g_rectangle minbounds = getBounds();
		minbounds.x = 0;
		minbounds.y = 0;
		if(me.type == G_MOUSE_EVENT_RELEASE && minbounds.contains(me.position))
		{
			checked = !checked;
		}

		markFor(COMPONENT_REQUIREMENT_PAINT);
	}
	return this;
}
