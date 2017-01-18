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

#ifndef __WINDOW__
#define __WINDOW__

#include <components/component.hpp>
#include <components/label.hpp>
#include <components/titled_component.hpp>
#include <components/panel.hpp>

/**
 * constants for border sizes
 */
#define DEFAULT_BORDER_WIDTH		7
#define DEFAULT_CORNER_SIZE			15

/**
 * modes used when resizing windows
 */
enum window_resize_mode_t {
	RESIZE_MODE_NONE,
	RESIZE_MODE_MOVE,
	RESIZE_MODE_TOP,
	RESIZE_MODE_TOP_RIGHT,
	RESIZE_MODE_RIGHT,
	RESIZE_MODE_BOTTOM_RIGHT,
	RESIZE_MODE_BOTTOM,
	RESIZE_MODE_BOTTOM_LEFT,
	RESIZE_MODE_LEFT,
	RESIZE_MODE_TOP_LEFT
};

/**
 *
 */
class window_t: public component_t, public titled_component_t {
private:
	int borderWidth;
	int cornerSize;
	g_color_argb backgroundColor;
	bool resizable;

	label_t label;
	panel_t panel;

	bool crossPressed;
	bool crossHovered;
	bool focused;

	int shadowSize;
	g_rectangle crossBounds;

public:
	window_t();

	virtual ~window_t() {
	}

	/**
	 *
	 */
	panel_t* getPanel() {
		return &panel;
	}

	/**
	 *
	 */
	void setBackground(g_color_argb color) {
		backgroundColor = color;
		markFor(COMPONENT_REQUIREMENT_PAINT);
	}

	/**
	 *
	 */
	g_color_argb getBackground() {
		return backgroundColor;
	}

	/**
	 *
	 */
	virtual void addChild(component_t* component);

	/**
	 *
	 */
	virtual void layout();

	/**
	 *
	 */
	virtual void paint();

	/**
	 *
	 */
	virtual void handleBoundChange(g_rectangle oldBounds);

	/**
	 *
	 */
	virtual bool handle(event_t& e);

	/**
	 *
	 */
	virtual bool getNumericProperty(int property, uint32_t* out);

	/**
	 *
	 */
	virtual bool setNumericProperty(int property, uint32_t value);

	/**
	 *
	 */
	virtual void setTitle(std::string title);

	/**
	 *
	 */
	virtual std::string getTitle();

	/**
	 *
	 */
	virtual void close();

	/**
	 *
	 */
	virtual void setLayoutManager(layout_manager_t* layoutManager);

};

#endif
