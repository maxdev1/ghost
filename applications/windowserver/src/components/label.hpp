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

#ifndef __LABEL__
#define __LABEL__

#include <components/component.hpp>
#include <components/titled_component.hpp>
#include <ghostuser/graphics/text/font.hpp>
#include <ghostuser/graphics/text/text_alignment.hpp>

/**
 *
 */
class label_t: public component_t, public titled_component_t {
private:
	g_font* font;
	int fontSize;
	cairo_text_extents_t lastExtents;

	std::string text;
	g_text_alignment alignment;
	g_color_argb color;

public:
	label_t();
	virtual ~label_t() {
	}

	virtual void paint();
	virtual void update();
	virtual bool handle(event_t& e);

	virtual void setFont(g_font* font);

	virtual void setColor(g_color_argb color);
	virtual g_color_argb getColor();

	virtual void setTitle(std::string title);
	virtual std::string getTitle();

	void setAlignment(g_text_alignment alignment);
	g_text_alignment getAlignment();
};

#endif
