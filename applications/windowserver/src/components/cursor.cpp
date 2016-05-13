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

#include <components/cursor.hpp>

#include <ghost.h>
#include <events/mouse_event.hpp>
#include <ghostuser/utils/property_file_parser.hpp>
#include <ghostuser/utils/logger.hpp>

static std::map<std::string, cursor_configuration> cursorConfigurations;
static cursor_configuration* currentConfiguration = 0;

g_point cursor_t::position;
g_point cursor_t::nextPosition;

mouse_button_t cursor_t::pressedButtons = MOUSE_EVENT_NONE;
mouse_button_t cursor_t::nextPressedButtons = MOUSE_EVENT_NONE;
component_t* cursor_t::draggedComponent = 0;
component_t* cursor_t::hoveredComponent = 0;
component_t* cursor_t::focusedComponent = 0;

/**
 *
 */
void cursor_t::set(std::string name) {

	if (cursorConfigurations.count(name) > 0) {
		currentConfiguration = &cursorConfigurations[name];
	} else if (cursorConfigurations.count("default") > 0) {
		currentConfiguration = &cursorConfigurations["default"];
	} else {
		g_logger::log("could neither load '" + name + "' cursor nor 'default' cursor");
	}

}

/**
 *
 */
bool cursor_t::load(std::string cursorPath) {

	// Open config file
	std::ifstream in(cursorPath + "/cursor.cfg");
	if (!in.good()) {
		return false;
	}
	g_property_file_parser props(in);
	auto content = props.getProperties();

	// Read required params
	std::string name = content["name"];
	std::string hitpoint_x = content["hitpoint.x"];
	std::string hitpoint_y = content["hitpoint.y"];
	std::string image = content["image"];

	if (name.empty() || hitpoint_x.empty() || hitpoint_y.empty() || image.empty()) {
		return false;
	}

	// Convert hitpoint
	std::stringstream stx;
	stx << hitpoint_x;
	int hitpointX;
	stx >> hitpointX;

	std::stringstream sty;
	sty << hitpoint_y;
	int hitpointY;
	sty >> hitpointY;

	std::string cursorImagePath = (cursorPath + "/" + image);

	// check if file exists
	FILE* cursorImageFile = fopen(cursorImagePath.c_str(), "r");
	if (cursorImageFile == NULL) {
		return false;
	}
	fclose(cursorImageFile);

	// load cursor
	cursor_configuration pack;
	pack.surface = cairo_image_surface_create_from_png(cursorImagePath.c_str());
	if (pack.surface == nullptr) {
		klog("failed to load cursor image at '%s' for configuration '%s'", cursorImagePath.c_str(), cursorPath.c_str());
		return false;
	}

	pack.hitpoint = g_point(hitpointX, hitpointY);
	pack.size = g_dimension(cairo_image_surface_get_width(pack.surface), cairo_image_surface_get_height(pack.surface));
	cursorConfigurations[name] = pack;

	g_logger::log("created cursor '" + name + "' with hitpoint %i/%i", hitpointX, hitpointY);

	return true;
}

/**
 *
 */
void cursor_t::paint(g_graphics* global) {

	auto cr = global->getContext();
	cairo_reset_clip(cr);

	if (currentConfiguration) {
		// draw cursor image
		cairo_set_source_surface(cr, currentConfiguration->surface, position.x - currentConfiguration->hitpoint.x,
				position.y - currentConfiguration->hitpoint.y);
		cairo_paint(cr);

	} else {
		// draw fallback cursor
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_rectangle(cr, position.x, position.y, FALLBACK_CURSOR_SIZE, FALLBACK_CURSOR_SIZE);
		cairo_fill(cr);
	}
}

/**
 *
 */
g_rectangle cursor_t::getArea() {

	// get area for current cursor
	if (currentConfiguration) {
		return g_rectangle(position.x - currentConfiguration->hitpoint.x, position.y - currentConfiguration->hitpoint.y, currentConfiguration->size.width,
				currentConfiguration->size.height);
	}

	// fallback cursor is just a square
	return g_rectangle(position.x, position.y, FALLBACK_CURSOR_SIZE, FALLBACK_CURSOR_SIZE);
}
